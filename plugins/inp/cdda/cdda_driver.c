/** @ingroup dcplaya_inp_driver
 *  @file    cdda_driver.c
 *  @author  benjamin gerard 
 *  @date    2003/01/01 
 *
 *  $Id: cdda_driver.c,v 1.5 2003-03-10 22:55:33 ben Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <dc/cdrom.h>

#include "dcplaya/config.h"
#include "inp_driver.h"
#include "fifo.h"
#include "sysdebug.h"

static uint32 sample_cnt;
static const int zero_size = (44100 / 60);
static short * zero;
static audio_track_t audio_track;

static int info(playa_info_t *info, const char *fname);

static unsigned int time_of_track(const audio_track_t * at) {
  return (at->samples * 11025u) >> 8;
}

static int init(any_driver_t * driver)
{
  SDDEBUG("%s('%s')\n", __FUNCTION__, driver->name);
  zero = 0;
  memset(&audio_track,0,sizeof(audio_track));
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


/* Start playback (implies song load) */
static int start(const char *fn, int track, playa_info_t *inf)
{
  int err;
  int status;
  audio_track_t at;

  SDDEBUG("%s('%s', %d)\n", __FUNCTION__, fn, track);

  status = cdrom_check();
  switch (status & 255) {
  case CDROM_BUSY:
  case CDROM_PAUSED:
  case CDROM_STANDBY:
  case CDROM_PLAYING:
  case CDROM_SEEKING:
  case CDROM_SCANING:
    break;
    
  case CDROM_OPEN:  case CDROM_NODISK: default:
    SDERROR("cdda : No disk.\n");
    return -1;
    break;
  }

  if (track < 0) {
    track = tracknum(fn);
  } else {
    ++track;
  }

  /* Get audio track info. */
  at.number = -1;
  if (cdrom_audio_track(track, &at)) {
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
    status = cdrom_check();
    SDERROR("cdda : GDR command PLAY failed [%s,%s].\n", 
	    cdrom_statusstr(status), cdrom_drivestr(status));
    audio_track.number = 0;
    goto error;
  }
  audio_track = at;

  info(inf, 0);
  sample_cnt = 0;
  return 0;

 error:
  return -1;

}

static int cdrom_cdda_stop(void)
{
  return cdrom_spin_down();
}

/* Stop playback (implies song unload) */
static int stop(void)
{
  if (zero) {
    free(zero);
    zero = 0;
  }
  audio_track.number = 0;
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
  int n, status;

  if (audio_track.number <= 0) {
    return INP_DECODE_ERROR;
  }

  status = cdrom_check();

  switch (status & 255) {
  case CDROM_BUSY:
  case CDROM_PAUSED:
  case CDROM_STANDBY:
  case CDROM_SEEKING:
  case CDROM_SCANING:
/*     SDERROR("cdda : CDROM [%s,%s].\n", */
/* 	    cdrom_statusstr(status), */
/* 	    cdrom_drivestr(status)); */
    return 0;
  case CDROM_PLAYING:
    break;

  case CDROM_OPEN:  case CDROM_NODISK:
  default:
    SDERROR("cdda : CDROM [%s,%s].\n",
	    cdrom_statusstr(status),
	    cdrom_drivestr(status));
    return INP_DECODE_ERROR;
    break;
  }

  n = fifo_write_mono(zero, zero_size);
  if (n < 0) {
    return INP_DECODE_ERROR;
  }
  sample_cnt += n;

  return (-(n > 0) & INP_DECODE_CONT)
    | (-(sample_cnt >= audio_track.samples) & INP_DECODE_END);
}

static int make_info(playa_info_t *info, audio_track_t * track, int tracks)
{
  char tmp[64];

  if (!track) {
    return -1;
  }

  playa_info_time(info, time_of_track(track));
  sprintf(tmp, "CD Audio Track #%02u", (int)track->number);
  playa_info_title(info, tmp);
  if (tracks <= 0) {
    sprintf(tmp,"%02d", (int)track->number);
  } else {
    sprintf(tmp,"%02d/%02d",  (int)track->number, tracks);
  }
  playa_info_track(info,tmp);
  playa_info_bytes(info, track->samples<<2);

  return 0;
}

static int info(playa_info_t *info, const char *fname)
{
  playa_info_bits(info,1);
  playa_info_stereo(info,1);
  playa_info_frq(info, 44100);
  playa_info_bps(info, 44100<<5);

  if (!fname) {
    if (audio_track.number) {
      return make_info(info, &audio_track, cdrom_audio_tracks());
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
  ".cda\0.cdda\0",        /* Extension list */
  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(cdda_driver)



