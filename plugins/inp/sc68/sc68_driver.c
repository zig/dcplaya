/**
 * @file      dreamcast68.c
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/02/08
 * @brief     sc68 for dreamcast - main for kos 1.1.x
 * @version   $Id: sc68_driver.c,v 1.2 2002-09-04 18:54:11 ben Exp $
 */

/* generated config include */
#include "config.h"

#include <kos/fs.h>
#include <kos/fs_romdisk.h>

//#include <dc/fmath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* sc68 includes */
#include "file68/SC68error.h"
#include "file68/SC68utils.h"
#include "file68/SC68file.h"
#include "file68/SC68os_general.h"

#include "sc68app/SC68.h"
#include "sc68app/SC68os_interface.h"
#include "sc68app/SC68play.h"
#include "sc68app/SC68delay.h"
#include "sc68app/SC68mixer.h"

/* 68000 emulator includes */
#include "emu68/emu68.h"
#include "emu68/ioplug68.h"
#include "io68/io68.h"

#include "inp_driver.h"
#include "fifo.h"


/* 512 Kb 68K memory buffer */
#define MEMORY68 (1<<19)

/* Browse info update when disk is loaded. */
#define UPDATE_BROWSE_INFO 0

SC68app_t app; /**< sc68 application context. */

extern char * playa_make_time_str(unsigned int);

/** Display to output debug statcked error messages.
 */
static void spool_error_message(void)
{
  char *s;

  if (s = SC68error_get(), s) {
    dbglog(DBG_DEBUG, "sc68 : Stacked Error Message:\n");
    do {
      dbglog(DBG_DEBUG, "  -->  %s\n", s);
    } while (s = SC68error_get(), s);
  } else {
    dbglog(DBG_DEBUG, "sc68 : No stacked error message\n");
  }
}

/** Initialize 68000 emulator.
 */
static int init_68K(reg68_t * r, u32 sz)
{
  if (EMU68_init(sz) < 0) {
    return SC68error_add(EMU68error_get());
  }
  *r = reg68;
  r->sr = 0x2000;
  r->a[7] = sz - 4;
  if (MW_init() < 0)
    return SC68error_add("Microwire emulator init failed");
  if (MFP_init() < 0)
    return SC68error_add("MFP emulator init failed");
  if (YM_init() < 0)
    return SC68error_add("Yamaha 2149 emulator init failed");
  if (PL_init() < 0)
    return SC68error_add("Paula emulator init failed");
  return 0;
}

/** Set sc68 application to default values. */
static void set_sc68app_default(SC68app_t * app)
{
  memset(app, 0, sizeof(*app));
  app->cur_track = -1;
  SC68config_default(&app->config);
}

static int init(any_driver_t *d)
{
  int err = 0;
  int frq;

  dbglog(DBG_DEBUG, "sc68 : Init [%s]\n", d->name);

  /* check whether EMU68 is compiled without debug option */
  if (EMU68_sizeof_reg68() != sizeof(reg68_t)) {
    SC68error_add
      ("Internal : Bad EMU68 version : should not define SC68DEBUG ...");
    err = __LINE__;
    goto error;
  }

  /* Set default configuration */
  set_sc68app_default(&app);

  /* Copy config parameters to app */
  SC68app_configure(&app);

  frq = (int) app.mix.real_frq;

  /* 68K & IO-Emulators init */
  if (init_68K(&app.reg68, MEMORY68)) {
    err = __LINE__;
    goto error;
  }
  dbglog(DBG_DEBUG, "sc68: 68K emulator init : OK\n");

  /* IO replay frequency init */
  app.mix.real_frq = 44100;
  YM_set_replay_frq(app.mix.real_frq);
  MW_set_replay_frq((int) app.mix.real_frq);
  PL_set_replay_frq((int) app.mix.real_frq);
  app.mix.buf = YM_get_buffer();

 error:
  spool_error_message();
  dbglog(DBG_DEBUG, ">> %s : error line [%d]\n", __FUNCTION__ , err);
  return err;
}

static int stop(void)
{
  dbglog(DBG_DEBUG, "sc68: STOP, Eject disk\n");

  SC68stop(&app);
  SC68eject(&app);
  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  stop();
  return 0;
}

extern uint8 sc68disk[], romdisk[];

static int start(const char *fn, decoder_info_t *info)
{
  int err = 0;
  disk68_t *disk = 0;
  music68_t *mus = 0;


  /* $$$ Hack romdisk */
  
  fs_romdisk_shutdown();
  fs_romdisk_init(sc68disk);
  

  disk = SC68app_load_diskfile((char *)fn, 0);
  if (!disk) {
    stop();
    err = -1;
    goto error;
  }
  
  if (err = SC68stop(&app), err < 0) {
    goto error;
  }
  if (err = SC68load(&app, disk), err < 0) {
    goto error;
  }

  /* Patch for tao_tsd, use 25Khz version ! */
  if (app.cur_disk) {
    int i;
    file_t fd;

    for (i=0; i<app.cur_disk->nb_six; ++i) {
      char *replay = app.cur_disk->mus[i].replay;
      if (replay && !strcmp("tao_tsd", replay)) {
        app.cur_disk->mus[i].replay = "tao_tsd25";
        dbglog(DBG_DEBUG, "sc68 : Patched #%d for tao_tsd25\n", i+1);
      } 
    }

    fd = fs_open(fn, O_RDONLY);
    if (fd) {
      info->bytes = fs_total(fd);
      fs_close(fd);
    } else {
      info->bytes = 0; /* $$$ Safety net !!! */
    }
    info->bps    =  (info->bytes << 3) / (mus->time ? mus->time : 5*60);
    info->desc   = "sc68 file";
    info->frq    = (int) app.mix.real_frq;
    info->bits   = 16;
    info->stereo = 1;
    info->time   = app.cur_disk->time * 1000;
  } else {
    err = -1;
    goto error;
  }
  err = 0;

 error:
  spool_error_message();
  
  fs_romdisk_shutdown();
  fs_romdisk_init(romdisk);
  
  return err;
}

static int FillYM(void)
{
  int status = INP_DECODE_CONT;
  const unsigned int sign = SC68MIXER_CHANGE_SIGN;
  const unsigned int cycle_this_pass = app.mix.cycleperpass;

  /* Data in YM mix buffer ? */
  if (app.mix.buflen) {
    return status;
  }

  EMU68_level_and_interrupt(cycle_this_pass);

  /* YM ??? */
  if (app.cur_mus->flags.ym) {
    app.mix.buflen = YM_mix(cycle_this_pass);
  } else {
    app.mix.buflen = app.mix.stdbuflen;
  }

  app.mix.buf = YM_get_buffer();

  /* MW ??? */
  if (app.cur_mus->flags.ste) {
    MW_mix(app.mix.buf, app.reg68.mem, app.mix.buflen);
    if (sign) {
      SC68mixer_stereo_16_LR(app.mix.buf, app.mix.buf,
			     app.mix.buflen, sign);
    }
  }
  /* Paula ??? */
  else if (app.cur_mus->flags.amiga) {
    PL_mix(app.mix.buf, app.reg68.mem,
	   app.reg68.mem + app.reg68.memsz, app.mix.buflen);

    SC68mixer_stereo_16_LR(app.mix.buf, app.mix.buf,
			   app.mix.buflen, SC68MIXER_CHANGE_SIGN);

    SC68mixer_blend_LR(app.mix.buf, app.mix.buf, app.mix.buflen,
		       app.mix.amiga_blend, sign);
  }
  /* Do stereo my self */
  else {
    SC68mixer_dup_L_to_R(app.mix.buf, app.mix.buf, app.mix.buflen, sign);
  }

  /* Advance time */
  SC68app_advance_time(&app);

  /* Reach end of track */
  if (app.time.elapsed > app.time.track) {
    dbglog(DBG_DEBUG, "sc68 : END OF TRACK\n");
    if (app.cur_track + 1 >= app.cur_disk->nb_six) {
      dbglog(DBG_DEBUG, "sc68 : NO MORE TRACK\n");
      status |= INP_DECODE_END;
    } else {
      SC68track(&app, app.cur_track + 1);
      status |= INP_DECODE_INFO;
    }
  }

  return status;
}

static int decoder(decoder_info_t *info)
{
  int status;
  int n;
        
  status = FillYM();
  if (status < 0) {
    spool_error_message();
    return INP_DECODE_ERROR;
  }

  n = 0;
  if (app.mix.buflen > 0) {
    n = fifo_write((int *)app.mix.buf, app.mix.buflen);
    if (n > 0) {
      app.mix.buf += n;
      app.mix.buflen -= n;
    }
  }

  if (!n) {
    /* Nothing has been written. fifo must be full ! */
    status &= ~INP_DECODE_CONT;
  }

  return (n < 0) ? INP_DECODE_ERROR : status;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}


static int disk_info(playa_info_t *info, disk68_t *d)
{
  char tmp[256];
  music68_t *mus = 0;

  if (!d) {
    d = app.cur_disk;
    mus = app.cur_mus;
    if (!d) {
      return -1;
    }
  }

  if (!mus) {
    mus = d->mus + d-> default_six;
  }

  sprintf(tmp, "%s - %s replay @ %d Hz",
	  mus->flags.amiga
	  ? "Amiga"
	  : (mus->flags.ste ? "Atari STE" : "Atari STF"),
	  mus->replay ? mus->replay : "internal",
	  mus->frq);
  info->time = playa_make_time_str(mus->time * 1000);
  info->format = strdup(tmp);
  info->artist = strdup(mus->author);
  info->album  = strdup(d->name);
  if (app.cur_track < 0) {
    sprintf(tmp,"%02d", d->nb_six);
  } else {
    sprintf(tmp,"%02d/%02d", app.cur_track+1, d->nb_six);
  }
  info->track  = strdup(tmp);
  info->title = strdup(mus->name);
  info->year = 0;
  info->genre = strdup("chip-tune");
  info->comments = 0;
  return 0;
}

static int file_info(playa_info_t *info, const char *fname)
{
  disk68_t *d;

  d = SC68file_load((char *)fname);
  if (d) {
    disk_info(info, d);
    free(d);
  }
  return d ? 0 : -1;
}

static int info(playa_info_t *info, const char *fname)
{
  if (fname) {
    return file_info(info, fname);
  } else {
    return disk_info(info, 0);
  }
}

static inp_driver_t sc68_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /**< Next driver (see any_driver.h)  */
    INP_DRIVER,           /**< Driver type */      
    0x0100,               /**< Driver version */
    "sc68",               /**< Driver name */
    "Benjamin Gerard\0",  /**< Driver authors */
    "Atari & Amiga "      /**< Description */
    "music player",
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /**< User Id */
  ".sc68\0.sc6\0",        /**< EXtension list */

  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(sc68_driver)
