/**
 * @file      sidplay_driver.cxx
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/09/03
 * @brief     sidplay input plugin for dcplaya
 * @version   $Id: sidplay_driver.cxx,v 1.13 2002-12-06 14:41:35 ben Exp $
 */

/* generated config include */
// #include "config.h"

// #include <kos/fs.h>
// #include <kos/fs_romdisk.h>

//#include <dc/fmath.h>

extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kos/fs.h>
};

#include "player.h"
#include "emucfg.h"

extern "C" {
#include "inp_driver.h"
#include "fifo.h"
#include "playa.h"
#include "gzip.h"
#include "sysdebug.h"
};

extern "C" {
int time()
{
  return 0;
}
}

static int disk_info(playa_info_t *info, sidTune * sidtune);

static void DisplayConfig(emuConfig * c)
{
  dbglog(DBG_DEBUG,"--------------------------------------------------\n");
  dbglog(DBG_DEBUG,"CONFIG:\n");
  dbglog(DBG_DEBUG,"--------------------------------------------------\n");
  dbglog(DBG_DEBUG,"frq: %d\n", c->frequency);
  dbglog(DBG_DEBUG,"bit: %d\n", c->bitsPerSample);
  dbglog(DBG_DEBUG,"chn: %d\n", c->channels);
  dbglog(DBG_DEBUG,"fmt: %d\n", c->sampleFormat);
  dbglog(DBG_DEBUG,"vol: %x\n", c->volumeControl);
  dbglog(DBG_DEBUG,"--------------------------------------------------\n");
}

static emuEngine * engine;     // sidplay emulator engine
static emuConfig config;       // sidplay emulator config
static sidTune * tune;

static ubyte * sidbuffer;
static int sidbuffer_len;

static int auto_next_track = 1;

static unsigned int minMs;      // Min time (1024th of second) for music
static unsigned int maxMs;      // Max time (1024th of second) for music
static unsigned int zeroMs;     // Time of successive zero for end detect.

static unsigned int splCnt;     // Sample counter
static unsigned int splGoalMin; // Mimimum sample for music stop
static unsigned int splGoal;    // Sample goal (end of track)
static unsigned int zeroCnt;    // Number of successive zero sample recieved
static unsigned int zeroGoal;   // Number of zero samples to detect end.

static int init(any_driver_t *d)
{
  int err = 0;

  sidbuffer = 0;
  sidbuffer_len = 0;
  minMs = 6<<10;       /* All tracks hav 6 seconds time minimum */
  maxMs = (60*8)<<10;  /* All tracks have 8 minutes time maximum */
  zeroMs = 6<<10;      /* Successive zero time for end detection */
  tune = 0;
  auto_next_track = 1;
  engine = new emuEngine;
  if (!engine) {
    err = __LINE__;
    goto error;
  }
  if (!engine->getStatus()) {
    err = __LINE__;
    goto error;
  }
  // Get default config
  engine->getConfig(config);
  // Apply our changes
  config.frequency = 44100;
  config.channels = SIDEMU_MONO;
  config.bitsPerSample = SIDEMU_16BIT;
  config.sampleFormat = SIDEMU_SIGNED_PCM;
  config.emulateFilter = true; //false; //
  // Set new config
  engine->setConfig(config);
  if (!engine->getStatus()) {
    err = __LINE__;
    goto error;
  }
  DisplayConfig(&config);

 error:
  if (err) {
    if (engine) {
      delete engine;
      engine = 0;
    }
  }

  dbglog(DBG_DEBUG, ">> %s : error line [%d]\n", __FUNCTION__ , err);
  return err;
}

static int stop(void)
{
  if (tune) {
    delete tune;
    tune = 0;
  }

  if (sidbuffer) {
    delete sidbuffer;
    sidbuffer = 0;
  }
  sidbuffer_len = 0;

  dbglog(DBG_DEBUG, "sidplay: STOP\n");
  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  stop();
  if (engine) {
    delete engine;
    engine = 0;
  }
  return 0;
}

static int load_sid(const char *fn)
{
  if (sidbuffer) {
    delete sidbuffer;
    sidbuffer = 0;
  }

  sidbuffer = (ubyte *)gzip_load(fn, &sidbuffer_len);
  if (!sidbuffer) {
    goto error;
  }

  return 0;

 error:
  if (sidbuffer) {
    delete sidbuffer;
    sidbuffer = 0;
  }
  sidbuffer_len = 0;
  return -1;
}

static int start_track(int track, playa_info_t *info)
{
  sidTuneInfo sidinfo;
  unsigned int ms;
  const int sht = 5;

  playa_info_t infotmp;

  if (!tune || !tune->getInfo(sidinfo)) {
	return -1;
  }
  
  if (track == 0) {
    track = sidinfo.startSong;
  } else if (track == -1) {
	track = sidinfo.currentSong+1;
  }
  if (track < 1 || track > sidinfo.songs) {
	return 0;
  }

  if (!sidEmuInitializeSong(*engine, *tune, track)) {
	return -1;
  }

  if (!info) {
	info = &infotmp;
  }
  disk_info(info, tune);
	  
  ms = info->info[PLAYA_INFO_TIME].v;
  zeroGoal = zeroCnt = splCnt = 0;

  if (!ms) {
	ms = maxMs;
	splGoalMin = (config.frequency * (minMs>>5))  >> sht;
	splGoal    = (config.frequency * (maxMs>>5))  >> sht;
	zeroGoal   = (config.frequency * (zeroMs>>5)) >> sht;
  } else {
	splGoal = (config.frequency * (ms >> 5)) >> sht;
  }
  
//   if (zeroGoal) {
//     SDWARNING("Music has no time info. Auto detect parameters:\n"
// 			  " Min  sample:%u\n"
// 			  " Max  sample:%u\n"
// 			  " Zero sample:%u\n",splGoalMin, splGoal, zeroGoal);
//   } else {
//     SDWARNING("Music has time. Max sample:%u\n", splGoal);
//   }

  return track;
}

static int start(const char *fn, int track, playa_info_t *info)
{
  int err = 0;

  if (!tune) {
    tune = new sidTune(0,0);
  }
  if (!tune) {
    err = __LINE__;
    goto error;
  }
   if (load_sid(fn) < 0) {
    err = __LINE__;
    goto error;
  }
  if (!tune->load(sidbuffer, sidbuffer_len)) {
    err = __LINE__;
    goto error;
  }
  /* Sid track are 1 based. */
  track = start_track(track+1, info);
  err = track < 0;

 error:
  return -err;
}

static int decoder(playa_info_t *info)
{
  int status = 0;
  int buffer[512];
  int n;

  /* Check fir valid sid engine and sid tune objects  */
  if (!engine || !tune) {
    SDERROR("sidplay: null object [%p %p]\n", engine, tune);
    return INP_DECODE_ERROR;
  }

  /* Get fifo free space (in bytes) */
  n = fifo_free() << 1;
  if (n < 0) {
    SDERROR("sidplay: fifo error\n");
    return INP_DECODE_ERROR;
  }

  if (n == 0) {
    return 0;
  }

  /* Not to much please. */
  if (n > (int)sizeof(buffer)) {
    n = sizeof(buffer);
  }

  /* Run emulator, fill buffer */
  sidEmuFillBuffer(*engine, *tune, buffer, n);
  if (!engine->getStatus()) {
    SDERROR("sidplay: status error\n");
    return INP_DECODE_ERROR;
  }

  /* Get it back to sample */
  n >>= 1;

  if (fifo_write_mono((short *)buffer, n) != n) {
    /* This should not happen since we check the fifo above and no other
       thread fill it. */
    SDERROR("sidplay: write error\n");
    return INP_DECODE_ERROR;
  }

  status = INP_DECODE_CONT;
  splCnt += n;

  /* Auto detect end */
  if (zeroGoal) {
	int i;
	for (i=0; i<(n>>1) && !buffer[i]; ++i);
	if (i == (n>>1) && !( (n&1) && ((short*)buffer)[n-1])) {
		zeroCnt += n;
// 		SDDEBUG("Idle: %u %u\n", n, zeroCnt);
		if (zeroCnt >= zeroGoal && splCnt >= splGoalMin) {
		  /* End detected */
// 		  SDDEBUG("sid: End detected @%u\n",splCnt);
		  splCnt = splGoal;
		}
	} else {
	  zeroCnt = 0;
	}
  }

  if (splCnt >= splGoal) {
//     SDDEBUG("sidplay: reach end [%u > %u]\n",splCnt, splGoal);
	int track;
	track = start_track(-1, info);
	if (track < 0) {
	  status = INP_DECODE_ERROR;
	} else if (!track) {
	  status |= INP_DECODE_END ;
	} else {
	  status |= INP_DECODE_INFO;
	}
  }

  return status;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}

static int update_info(playa_info_t *info, sidTuneInfo & sidinfo, char *tmp)
{
  sidTune * sidtune;

  info->update_mask = 0;
  if (sidtune = tune, !sidtune) {
    return -1;
  }
  if (!sidtune->getInfo(sidinfo)) {
    return -1;
  }

  playa_info_time(info, sidinfo.lengthInSeconds<<10);

  sprintf(tmp,"%02d/%02d", sidinfo.currentSong, sidinfo.songs);
  playa_info_track(info, tmp);
  playa_info_title(info, sidinfo.nameString);
  playa_info_comments(info, sidinfo.copyrightString);

  return 0;
}

static int disk_info(playa_info_t *info, sidTune * sidtune)
{
  char tmp[256];
  sidTuneInfo sidinfo;

  if (!sidtune) {
    return update_info(info, sidinfo, tmp);
  }
  if (!sidtune->getInfo(sidinfo)) {
    return -1;
  }

//   SDDEBUG("disk_info(%p)\n",sidtune);

//$$$ ben: Make stream channel low frq !! Add another variable for replay frq
    //(sidinfo.clock & SIDTUNE_CLOCK_PAL) ? 50 : 60;

  playa_info_bits(info, config.bitsPerSample >> 4);
  playa_info_stereo(info, config.channels-1);
  playa_info_frq(info, config.frequency);
//   playa_info_time(info, (5*60) << 10);
  playa_info_bps(info, 0);
  playa_info_bytes(info, 0);
  
  playa_info_desc(info, (char *)sidinfo.formatString);
  playa_info_artist(info, sidinfo.authorString);
  playa_info_album(info, sidinfo.nameString);
  sprintf(tmp,"%02d/%02d", sidinfo.currentSong, sidinfo.songs);
  playa_info_track(info, tmp);
  playa_info_title(info, sidinfo.nameString);
  playa_info_year(info, 0);
  playa_info_genre(info, "chip-tune");
  playa_info_comments(info, sidinfo.copyrightString);

  return 0;
}

static int file_info(playa_info_t *info, const char *fname)
{
  info->update_mask = 0;
  return 0;
}

static int info(playa_info_t *info, const char *fname)
{
  if (fname) {
    return file_info(info, fname);
  } else {
    return disk_info(info, 0);
  }
}


inp_driver_t sidplay_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /**< Next driver (see any_driver.h)  */
    INP_DRIVER,           /**< Driver type                     */      
    0x0100,               /**< Driver version                  */
    "sid",                /**< Driver name                     */
    "Benjamin Gerard",    /**< Driver authors                  */
    "C64 music player",   /**< Description                     */
    0,                    /**< DLL handler                     */
    init,                 /**< Driver init                     */
    shutdown,             /**< Driver shutdown                 */
    options,              /**< Driver options                  */
  },
  
  /* Input driver specific */
  0,                      /**< User Id                         */
  ".sid\0"                /**< Extension list                  */
  ".sid.gz\0"
  ".psid\0"
  ".psid.gz\0"
  ".c64\0"
  ".c64.gz\0",

  start,
  stop,
  decoder,
  info,
};

extern "C" {

EXPORT_DRIVER(sidplay_driver)

};
