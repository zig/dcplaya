/**
 * @file      sidplay_driver.cxx
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/09/03
 * @brief     sidplay input plugin for dcplay
 * @version   $Id: sidplay_driver.cxx,v 1.3 2002-09-13 01:21:33 ben Exp $
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

#include "file_wrapper.h"
};

#include "player.h"
#include "emucfg.h"

extern "C" {
#include "inp_driver.h"
#include "fifo.h"
#include "playa.h"
};

extern "C" {
int time()
{
  return 0;
}
}

void * operator new (unsigned int bytes) {
  return malloc(bytes);
}

void operator delete (void *addr) {
  free (addr);
}

void * operator new[] (unsigned int bytes) {
  return malloc(bytes);
}

void operator delete[] (void *addr) {
  free (addr);
}

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

static unsigned int splCnt;    // Sample counter
static unsigned int splGoal;   // Sample goal (end of track)

static int init(any_driver_t *d)
{
  int err = 0;

  sidbuffer = 0;
  sidbuffer_len = 0;

  tune = 0;
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
  int fd = 0;

  if (sidbuffer) {
    delete sidbuffer;
    sidbuffer = 0;
  }

  fd = fs_open(fn, O_RDONLY);
  if (!fd) {
    goto error;
  }

  sidbuffer_len = fs_total(fd);
  if (sidbuffer_len < 128) {
    goto error;
  }

  sidbuffer = new ubyte [sidbuffer_len];
  if (!sidbuffer) {
    goto error;
  }

  if (fs_read(fd, sidbuffer, sidbuffer_len) != (unsigned int)sidbuffer_len) {
    goto error;
  }

  fs_close(fd);
  return 0;

 error:
  if (fd) {
    fs_close(fd);
  }
  if (sidbuffer) {
    delete sidbuffer;
    sidbuffer = 0;
  }
  sidbuffer_len = 0;
  return -1;
}

static int start(const char *fn, decoder_info_t *info)
{
  int err = 0;
  sidTuneInfo sidinfo;

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

  tune->getInfo(sidinfo);
  if (!sidEmuInitializeSong(*engine, *tune, sidinfo.startSong)) {
    err = __LINE__;
    goto error;
  }

  info->bytes = sidbuffer_len;

  {
    int secs;
    static char desc[256];
    sprintf(desc, "%s - %s", sidinfo.formatString, sidinfo.speedString);

    secs = 5*60;
    info->bps    =  (info->bytes << 3) / secs;
    info->desc   = desc;
    info->frq    = config.frequency;
    info->bits   = config.bitsPerSample;
    info->stereo = config.channels-1;
    info->time   = secs * 1000;

    splCnt  = 0;
    splGoal = secs * info->frq;
  }

  err = 0;

 error:
  return err;
}

static int decoder(decoder_info_t *info)
{
  int status = 0;
  int buffer[512];
  int n;

  /* Check fir valid sid engine and sid tune objects  */
  if (!engine || !tune) {
    return INP_DECODE_ERROR;
  }

  /* Get fifo free space (in bytes) */
  n = fifo_free() << 1;
  if (n < 0) {
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
    return INP_DECODE_ERROR;
  }

  /* Get it back to sample */
  n >>= 1;

  if (fifo_write_mono((short *)buffer, n) != n) {
    /* This should not happen since we check the fifo above and no other
       thread fill it. */
    return INP_DECODE_ERROR;
  }

  status = INP_DECODE_CONT;

  splCnt += n;
  if (splCnt >= splGoal) {
    status |= INP_DECODE_END ;
  }

  return status;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}


static int disk_info(playa_info_t *info, sidTune * sidtune)
{
  char tmp[64];
  sidTuneInfo sidinfo;

  if (!sidtune) {
    sidtune = tune;
  }
  if (!sidtune) {
    return -1;
  }


  if (!sidtune->getInfo(sidinfo)) {
    return -1;
  }

  info->format = strdup(sidinfo.formatString);
  info->time   = playa_make_time_str(5*60*1000);

  info->artist = strdup(sidinfo.authorString);
  info->album  = strdup(sidinfo.nameString);
  sprintf(tmp,"%02d/%02d", sidinfo.currentSong, sidinfo.songs);
  info->track  = strdup(tmp);
  info->title = strdup(sidinfo.nameString);
  info->year = 0;
  info->genre = strdup("chip-tune");
  info->comments = 0;
  return 0;
}

static int file_info(playa_info_t *info, const char *fname)
{
  info->format = 0;
  info->time   = 0;

  info->artist = 0;
  info->album  = 0;
  info->track  = 0;
  info->title = 0;
  info->year = 0;
  info->genre = 0;
  info->comments = 0;
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
    INP_DRIVER,           /**< Driver type */      
    0x0100,               /**< Driver version */
    "sidplay",            /**< Driver name */
    "Benjamin Gerard\0",  /**< Driver authors */
    "C64 music player",   /**< Description */
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /**< User Id */
  ".sid\0.psid\0.c64\0",  /**< EXtension list */

  start,
  stop,
  decoder,
  info,
};

extern "C" {

EXPORT_DRIVER(sidplay_driver)

};
