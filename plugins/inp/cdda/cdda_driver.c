/** @ingroup dcplaya_inp_driver
 *  @file    cdda_driver.c
 *  @author  benjamin gerard 
 *  @date    2003/01/01 
 *
 *  $Id: cdda_driver.c,v 1.1 2003-01-02 11:50:17 ben Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <dc/cdrom.h>

#include "inp_driver.h"
#include "fifo.h"
#include "sysdebug.h"

/* Disk types :
 * $00 CDDA
 * $10 CDROM
 * $20 CDROM/XA
 * $30 CDI $80 GDROM
 */

typedef struct {
  uint32 number;        /**< Track number.     */
  uint32 lba_start;     /**< Starting address. */
  uint32 lba_end;       /**< Ending address.   */
  uint32 time_ms;       /**< Time in ms.       */
} audio_track_t;

typedef struct {
  int n;                       /**< Number of audio track. */
  audio_track_t * cur_track;   /**< Cuurent playing track. */
  audio_track_t track[1];      /**< Tracks.                */
} audio_cd_t;

static const int zero_size = (44100 / 60);
static short * zero;
static audio_cd_t * cdda;

static uint32 entry, entryend;

static int info(playa_info_t *info, const char *fname);

static int init(any_driver_t * driver)
{
  SDDEBUG("%s('%s')\n", __FUNCTION__, driver->name);
  zero = 0;
  entry = entryend = 0;
  cdda = 0;
  return 0;
}

static int tracknum(const char *name)
{
  int v = 0;
  if (name) {
    int c;
    while (c=*name++, (c && !(c>='0' && c<='9')))
      ;
    while (c>='0' && c<='9') {
      v = v * 10 + c - '0';
      c=*name++;
    }
  }
  return v;
}

/* ORBITAL
:: cdda_driver.c:56: TOC START,END,DUNNO 01010000, 01080000, 0104F2FE
:: cdda_driver.c:59: TOC ENTRY #1 : 01000096
:: cdda_driver.c:59: TOC ENTRY #2 : 0100B83F
:: cdda_driver.c:59: TOC ENTRY #3 : 010127AA
:: cdda_driver.c:59: TOC ENTRY #4 : 01019976
:: cdda_driver.c:59: TOC ENTRY #5 : 0102030A
:: cdda_driver.c:59: TOC ENTRY #6 : 0102B18A
:: cdda_driver.c:59: TOC ENTRY #7 : 01034A5A
:: cdda_driver.c:76: TOC ENTRY #8 : 010406A7
*/

/* DCLOAD
:: cdda_driver.c:68: TOC START,END,DUNNO 01010000, 41020000, 41002F7A
:: cdda_driver.c:71: TOC ENTRY #1 : 01000096
*/

/** CD TOC HELP
 *
 * CDDA has a start,end,dunno CTRL == 0
 * CDXA has a start==0,end==4,dunno==4 CTRL
 * CROM has a start==4,end==4,dunno==4 CTRL
 *
 * DUNNO LBA looks like disk total size, this is probably a number of sector
 */

static audio_cd_t * cdda_content(void)
{
  CDROM_TOC toc;
  int first, last, i, cnt;
  audio_cd_t * cdda = 0;

  memset(&toc,0,sizeof(toc));
  cdrom_reinit();
  if (cdrom_read_toc(&toc,0)) {
    SDERROR("Error reading CD TOC.\n");
    goto error;
  }

  first = TOC_TRACK(toc.first);
  last = TOC_TRACK(toc.last);

  /* Count audio tracks. */
  /* $$$ Here I will only scan track control code, ignoring the disk type. */
  for (i=first, cnt=0; i<=last; ++i) {
    cnt += TOC_CTRL(toc.entry[i - 1]) == 0;
  }

  if (!cnt) {
    /* No audio track. */
    SDERROR("Can not found an audio track on this disk.\n");
    goto error;
  }

  /* Allocation */
  cdda = malloc(sizeof(*cdda) + (cnt-1) * sizeof(cdda->track));
  if (!cdda) {
    SDERROR("audio CD TOC allcation failed.\n");
    goto error;
  }

  /* FILL cdda */
  for (i=first, cnt=0; i<=last; ++i) {
    uint32 e = toc.entry[i - 1];
    if (TOC_CTRL(e) == 0) {
      uint32 end;
      audio_track_t * track = cdda->track + i - 1;
      int sectors;

      end = (i == last) ? toc.dunno : toc.entry[i];

      track->lba_start = TOC_LBA(e);
      track->lba_end = TOC_LBA(end);
      sectors = track->lba_end - track->lba_start;
      if (sectors <= 0) {
	SDWARNING("Weird LBA for track %d: [%08x - %08x], skipping.\n",
		  i,e,end);
	continue;
      }
      /* $$$ From my calculation there is about audio 2348-2350 bytes
	 per sector */
      track->time_ms = ((unsigned int)sectors * 1365u + 99u) / 100u;
      track->number = ++cnt;

      SDDEBUG("track #%02d [%06x-%06x] [%d] [%02u:%02u].\n",
	      track->number, track->lba_start, track->lba_end, sectors,
	      (track->time_ms>>10)/60u, (track->time_ms>>10)%60u);
    }
  }
  cdda->n = cnt;

  return cdda;

 error:
  if (cdda) free(cdda);
  return 0;
}

/* Start playback (implies song load) */
static int start(const char *fn, int track, playa_info_t *inf)
{
  int err;

  SDDEBUG("%s('%s', %d)\n", __FUNCTION__, fn, track);

  cdda = cdda_content();
  if (!cdda) {
    goto error;
  }

  if (track < 0) {
    track = tracknum(fn);
  } else {
    ++track;
  }
  SDDEBUG("CDDA start track [%d]\n",track);

  if (track < 1 || track > cdda->n) {
    SDERROR("cdda : track #%02d out of range.\n", track);
    goto error;
  }

  /* Create empty buffer for fake streaming. */
  if (!zero) zero = malloc(2 * zero_size);
  if (!zero) {
    SDERROR("cdda : zero buffer malloc failed.\n");
    goto error;
  }
  memset(zero,0,2 * zero_size);

  /* Run PLAY track command */
  err = cdrom_cdda_play(track, track, 0, CDDA_TRACKS);
  if (err) {
    SDERROR("cdda : GDR command PLAY failed [%d].\n", err);
    goto error;
  }
  cdda->cur_track = cdda->track + track - 1;
  info(inf, 0);
  return 0;

 error:
  if (cdda) {
    free (cdda);
    cdda = 0;
  }
  entry = entryend = 0;
  return -1;

}

static int cdrom_cdda_stop(void)
{
/*   $$$ ben : does not work. */
/*   return cdrom_cdda_play(0, 0, 0, CDDA_TRACKS); */
/*   return cdrom_reinit(); */
  entry = entryend = 0;
  return cdrom_spin_down();
}

/* Stop playback (implies song unload) */
static int stop(void)
{
  if (zero) {
    free(zero);
    zero = 0;
  }
  if (cdda) {
    free(cdda);
    cdda = 0;
  }
  return cdrom_cdda_stop();
}

/* Shutdown the player */
static int shutdown(any_driver_t * driver)
{
  SDDEBUG("%s('%s')\n", __FUNCTION__, driver->name);
  stop();
  return 0;
}

static int decoder(playa_info_t * info)
{
  int n;

#if 0
  static int buffer[2048*8/4];
  static int in = 0;
  static int off = 0;
  if (!in) {
    int err;
    int n = entryend - entry;
    if (n<=0) {
      return INP_DECODE_END;
    }
    if (n > 8) n = 8;

    err = cdrom_read_sectors(buffer, entry, n);
    if (err) {
      SDDEBUG("err = %d\n",err);
      return INP_DECODE_ERROR;
    }

    entry += n;
    in = n * 2048/4;
    off = 0;
  }

  n = fifo_write(buffer+off, in);
  if (n < 0) {
    return INP_DECODE_ERROR;
  }
  off += n;
  in -= n;
#else
  n = fifo_write_mono(zero, zero_size);
  if (n < 0) {
    return INP_DECODE_ERROR;
  }

#endif
  return -(n > 0) & INP_DECODE_CONT;
}

static int make_info(playa_info_t *info, audio_track_t * track, int tracks)
{
  char tmp[64];

  if (!track) {
    return -1;
  }
  playa_info_time(info, track->time_ms);
  sprintf(tmp, "CD Audio Track #%02u", (int)track->number);
  playa_info_title(info, tmp);
  if (!tracks) {
    sprintf(tmp,"%02d", (int)track->number);
  } else {
    sprintf(tmp,"%02d/%02d",  (int)track->number, tracks);
  }
  playa_info_track(info,tmp);
  playa_info_bytes(info, (track->lba_end - track->lba_start) * 2349);
  playa_info_bytes(info, (track->lba_end - track->lba_start) * 2349);

  return 0;
}

static int info(playa_info_t *info, const char *fname)
{
  playa_info_bits(info,1);
  playa_info_stereo(info,1);
  playa_info_frq(info, 44100);
  playa_info_bps(info, 44100<<5);

  if (!fname) {
    if (cdda) {
      return make_info(info, cdda->cur_track, cdda->n);
    }
  }
  return -1;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}


static inp_driver_t cdda_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    INP_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "cdda",               /* Driver name */
    "Benjamin Gerard",    /* Driver authors */
    "Audio CD",           /**< Description */
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /* User Id */
  ".cdda\0.cda\0",        /* Extension list */
  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(cdda_driver)



