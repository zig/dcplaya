/** @ingroup dcplaya_inp_driver
 *  @file    nsf_driver.c
 *  @author  benjamin gerard 
 *  @date    2003/04/08
 *
 *  $Id: nsf_driver.c,v 1.4 2003-05-01 22:34:19 benjihan Exp $
 */ 

/* Adapted from nosefart main_linux.c */

#include <kos.h>
#include <string.h>

#include "dcplaya/config.h"
#include "inp_driver.h"
#include "pcm_buffer.h"
#include "fifo.h"
#include "sysdebug.h"

#include "gzip.h"
#include "types.h"
#include "nsf.h"

static nsf_t *nsf = 0;
static const int freq = 44100;
static const int bits = 16;

static const int pcm_stereo = 0; /* stereo does not work !! */
static short * pcm_ptr;
static int pcm_count;

static int dataSize;

static unsigned int minMs;      // Min time (1024th of second) for music
static unsigned int maxMs;      // Max time (1024th of second) for music
static unsigned int zeroMs;     // Time of successive zero for end detect.

static unsigned int splCnt;     // Sample counter
static unsigned int splGoalMin; // Mimimum sample for music stop
static unsigned int splGoal;    // Sample goal (end of track)
static unsigned int zeroCnt;    // Number of successive zero sample recieved
static unsigned int zeroGoal;   // Number of zero samples to detect end.
static uint16 oldspl;

static char * make_desc(nsf_t * nsf, char *tmp)
{
  sprintf(tmp,"Nintendo Famicom %d hz", (int)nsf->playback_rate);
  return tmp;
}

static char * make_track(nsf_t * nsf, char * tmp)
{
  sprintf(tmp,"%02d/%02d", nsf->current_song, nsf->num_songs);
  return tmp;
}

static char * make_comment(nsf_t * nsf, char * tmp)
{
  if (!nsf->copyright[0]) {
    return 0;
  }
  sprintf(tmp, "Copyright: %s", nsf->copyright);
  return tmp;
}

static char * make_title(nsf_t * nsf, char * tmp)
{
  if (!nsf->song_name[0]) {
    return 0;
  }
  sprintf(tmp, "%s #%d", nsf->song_name, nsf->current_song);
  return tmp;
}

static unsigned int make_time(nsf_t * nsf)
{
  unsigned int t = 0;
  int track = nsf->current_song;

  SDDEBUG("nsf: make_time #%d\n", track);

  if (!nsf->song_frames) {
    SDDEBUG("nsf: NO TIME INFO\n");
  } else if (!nsf->song_frames[track]) {
    SDDEBUG("nsf: NO TIME INFO FOR THIS TRACK\n");
  }

  if (nsf->song_frames
      && track >= 0
      && track <= nsf->num_songs
      && (t = nsf->song_frames[track], t)) {
    unsigned long long ms = (unsigned long long)t << 10;
    ms /= nsf->playback_rate ? nsf->playback_rate : 60;
    t = ms;
  }

  return t;
}

static int nsf_update_info(playa_info_t *info, nsf_t * nsf, char * tmp)
{
  playa_info_time(info, make_time(nsf));
  playa_info_title(info, make_title(nsf,tmp));
  playa_info_track(info, make_track(nsf,tmp));

  return 0;
}

static int nsf_info(playa_info_t *info, nsf_t * nsf)
{
  char tmp[512];

  if (!info || !nsf) {
    return -1;
  }
  nsf_update_info(info, nsf, tmp);
  playa_info_desc(info, make_desc(nsf,tmp));
  playa_info_bits(info,1);
  playa_info_stereo(info,pcm_stereo);
  playa_info_frq(info, freq);
  playa_info_bps(info,0);
  playa_info_bytes(info,0);
  playa_info_album(info, nsf->song_name);
  playa_info_artist(info, nsf->artist_name);
  playa_info_year(info,0);
  playa_info_genre(info,"game-tune");
  playa_info_comments(info, make_comment(nsf, tmp));

  return 0;
}

/* Load an NSF file. */
static nsf_t * load_nsf_file(const char * filename)
{
  nsf_t * nsf = 0;

#if 1
  int buffer_len;
  void * buffer = 0;
  buffer = gzip_load(filename, &buffer_len);
  SDDEBUG("LOAD:%p %d\n",buffer,buffer_len);
  if (buffer) {
    nsf = nsf_load(0/*(char *)filename*/, buffer, buffer_len);
    free(buffer);
  }
#else
  nsf = nsf_load((char *)filename,0,0);
#endif

  if (!nsf) {
    SDERROR("Error opening \"%s\"\n", filename);
  }
  return nsf;
}

/* start track, display which it is, and what channels are enabled */
static int nsf_setupsong(void)
{
  if (!nsf) {
    return -1;
  }
  dataSize = freq / nsf->playback_rate;

  SDDEBUG("nsf_setupsong : "
	  "nsf-length=%d nsf-rate:%d nsf-track:%d data-size:%d\n",
	  nsf->length,nsf->playback_rate, nsf->current_song, dataSize);

  nsf_setfilter(nsf, NSF_FILTER_NONE);
  nsf_playtrack(nsf, nsf->current_song, freq, bits, pcm_stereo);
  return nsf->current_song;
}

static int set_track(int track, playa_info_t * info)
{
  char tmp[512];

  if (!nsf) return -1;

  if (track == 0) {
    SDDEBUG("SETTING DEFAULT TRACK\n");
    /* Currently setting track 1, so we can hear all tracks... */
    track = 1; /*nsf->start_song;*/
  } else if (track == -1) {
    SDDEBUG("SETTING NEXT TRACK\n");
    track = nsf->current_song+1;
  }
  if (track < 1 || track > nsf->num_songs) {
    return 0;
  }
  nsf->current_song = track;
  if (info) {
    nsf_update_info(info, nsf, tmp);
  }

  return track;
}

/* Make some noise, run the show */
static int play(int track, playa_info_t * info)
{
  int err;
  unsigned int ms;
  const int sht = 5;

  track = set_track(track, info);
  if (track <= 0) {
    return track;
  }

  ms = info->info[PLAYA_INFO_TIME].v;
  zeroGoal = zeroCnt = splCnt = 0;
  oldspl = 0;
  if (!ms) {
    ms = maxMs;
    splGoalMin = (freq * (minMs>>5))  >> sht;
    splGoal    = (freq * (maxMs>>5))  >> sht;
    zeroGoal   = (freq * (zeroMs>>5)) >> sht;
  } else {
    splGoal = (freq * (ms >> 5)) >> sht;
  }

  SDDEBUG("TRACK    :%d\n"
	  "GoalMin  :%d\n"
	  "SplGoal  :%d\n"
	  "ZeroGoal :%d\n"
	  ,track,splGoalMin,splGoal,zeroGoal);

  err = nsf_setupsong();
  if (err > 0) {
/*     nsf_info(info, nsf); */
  } else {
    err = -1;
  }
  return err;
}

/* free what we've allocated */
static void close_nsf_file(void)
{
  if (nsf) {
    nsf_free(&nsf);
    nsf = 0;
  }
}

/************************************************************************/

static int init(any_driver_t * driver)
{
  SDDEBUG("%s('%s')\n", __FUNCTION__, driver->name);

  nsf_init();
  nsf = 0;
  minMs = 6<<10;       /* All tracks have 6 seconds time minimum */
  maxMs = (60*8)<<10;  /* All tracks have 8 minutes time maximum */
  zeroMs = 6<<10;      /* Successive zero time for end detection */
  return 0;
}

/* Stop playback (implies song unload) */
static int stop(void)
{
  SDDEBUG("%s()\n", __FUNCTION__);
  close_nsf_file();
  pcm_buffer_shutdown();
  pcm_count = 0;
  pcm_ptr = 0;
  return 0;
}

static int info(playa_info_t *info, const char *fname);

/* Start playback (implies song load) */
int start(const char *fn, int track, playa_info_t *inf)
{
  int err = 0;
  SDDEBUG("%s('%s', %d)\n", __FUNCTION__, fn, track);

  if (fn) {
    stop();
    nsf = load_nsf_file(fn);
  }
  err = nsf_info(inf, nsf);
  err = err || play(track+1, inf) < 0;

  if (!err) {
    const int size = (dataSize + 32) << pcm_stereo;
    err = pcm_buffer_init(size , 0);
    pcm_count = 0;
    pcm_ptr = pcm_buffer;
  }

  if (err) {
    stop();
    err = -1;
  }
  return err;
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
  int n = 0, status;
  int16 * buffer;

  if (!nsf) {
    return INP_DECODE_ERROR;
  }

  /* No more pcm : decode next nsf frame
   * No more frame : it is the end
   */
  if (!pcm_count) {
    nsf_frame(nsf);
    apu_process(pcm_buffer, dataSize);
    pcm_ptr = pcm_buffer;
    pcm_count = dataSize;
  }

  buffer = pcm_ptr;
      
  if (pcm_count > 0) {
    /* If we have some pcm , send them to fifo */
    if (pcm_stereo) {
      n = fifo_write((int *)pcm_ptr, pcm_count);
    } else {
      n = fifo_write_mono(pcm_ptr, pcm_count);
    }
    
    if (n > 0) {
      pcm_ptr   += (n<<pcm_stereo);
      pcm_count -= n;
    } else if (n < 0) {
      return INP_DECODE_ERROR;
    }
  }

  status = -(n > 0) & INP_DECODE_CONT;
  splCnt += n;

  /* Auto detect end */
  if (zeroGoal && n > 0) {
    int i;
    uint16 cs, os;
    cs = os = oldspl;
    for (i=0; i<n; ++i) {
      cs = buffer[i];
      if (cs != os) {
	break;
      }
      os = cs;
    }
    oldspl = buffer[n-1];

    if (i == n && cs == os) {
      zeroCnt += n;
      //      SDDEBUG("Idle: %d %u %u %u\n", cs, n, zeroCnt, zeroGoal);
      if (zeroCnt >= zeroGoal) {
	SDDEBUG("Idle: cnt:%u min:%u goal:%u\n",zeroCnt,splGoalMin,zeroGoal);
	if(splCnt >= splGoalMin) {
	  /* End detected */
	  SDDEBUG("sid: End detected @%u\n",splCnt);
	  splCnt = splGoal;
	}
      }
    } else {
      zeroCnt = 0;
    }
  }

  if (splCnt >= splGoal) {
    SDDEBUG("nsf: reach end [%u > %u]\n",splCnt, splGoal);
    int track;
    track = play(-1, info);
    if (track < 0) {
      status = INP_DECODE_ERROR;
    } else if (!track) {
      status |= INP_DECODE_END;
    } else {
      status |= INP_DECODE_INFO;
    }
  }

  return status;
}

static int info(playa_info_t *info, const char *fname)
{
  int err;

  if (!fname) {
    err = nsf_info(info, nsf);
  } else {
    nsf_t * nsf = load_nsf_file(fname);
    nsf->current_song = 0; /* Get total time. */
    err = nsf_info(info, nsf);
    nsf_free(&nsf);
  }
  return err;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}

static inp_driver_t driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    INP_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "nsf",                /* Driver name */
    "Benjamin Gerard, "   /* Driver authors */
    "Matthew Conte, "
    "Matthew Strait",
    "Nintendo Famicom "
    "music player",      /**< Description */
    0,                   /**< DLL handler */
    init,                /**< Driver init */
    shutdown,            /**< Driver shutdown */
    options,             /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /* User Id */
  ".nsf\0.nsf.gz\0",      /* EXtension list */

  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(driver)

