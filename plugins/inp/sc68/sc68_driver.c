/**
 * @file      dreamcast68.c
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/02/08
 * @brief     sc68 for dreamcast - main for kos 1.1.x
 * @version   $Id: sc68_driver.c,v 1.7 2003-03-10 22:55:33 ben Exp $
 */

#include "dcplaya/config.h"

#include <kos/fs.h>
#include "fs_rz.h"

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

#include "sysdebug.h"

/* 512 Kb 68K memory buffer */
#define MEMORY68 (1<<19)

/* Browse info update when disk is loaded. */
#define UPDATE_BROWSE_INFO 0

SC68app_t app; /**< sc68 application context. */

extern uint8 sc68newdisk[];

static int disk_info(playa_info_t *info, disk68_t *d);

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

static void shutdown_68K()
{
  EMU68_kill();
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

/*   SDDEBUG("sc68 : Init [%s]\n", d->name); */

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
/*   SDDEBUG("sc68: 68K emulator init : OK\n"); */

  /* IO replay frequency init */
  app.mix.real_frq = 44100;
  YM_set_replay_frq(app.mix.real_frq);
  MW_set_replay_frq((int) app.mix.real_frq);
  PL_set_replay_frq((int) app.mix.real_frq);
  app.mix.buf = YM_get_buffer();

  SDDEBUG("[sc68] : create sc68 romdisk\n");
  fs_rz_init(sc68newdisk);

 error:
  if (err) {
    spool_error_message();
    SDERROR("[%s] : error line [%d]\n", __FUNCTION__ , err);
    shutdown_68K();
  }

  return -!!err;
}

static int stop(void)
{
/*   dbglog(DBG_DEBUG, "sc68: STOP, Eject disk\n"); */
  SC68stop(&app);
  SC68eject(&app);
  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  stop();
  SDDEBUG("[sc68] : shutdown 68k\n");
  shutdown_68K();
  SDDEBUG("[sc68] : removing sc68 romdisk\n");
  fs_rz_shutdown();
  return 0;
}

static int start(const char *fn, int track, playa_info_t *info)
{
  int err = 0;
  disk68_t *disk = 0;

/*   SDDEBUG("%s(%s)\n", __FUNCTION__, fn); */
  
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

    for (i=0; i<app.cur_disk->nb_six; ++i) {
      char *replay = app.cur_disk->mus[i].replay;
      if (replay && !strcmp("tao_tsd", replay)) {
        app.cur_disk->mus[i].replay = "tao_tsd25";
/*         SDDEBUG( "sc68 : Patched #%d for tao_tsd25\n", i+1); */
      } 
    }

    if (err = SC68track(&app, track), err < 0) {
      goto error;
    }

    if (err = SC68play(&app), err < 0) {
      goto error;
    }

  } else {
    err = -1;
    goto error;
  }
  err = 0;

 error:
  if (err) {
    SC68eject(&app);
  }
  disk_info(info, app.cur_disk);
  spool_error_message();
  
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
/*     dbglog(DBG_DEBUG, "sc68 : END OF TRACK\n"); */
    if (app.cur_track + 1 >= app.cur_disk->nb_six) {
/*       dbglog(DBG_DEBUG, "sc68 : NO MORE TRACK\n"); */
      status |= INP_DECODE_END;
    } else {
      SC68track(&app, app.cur_track + 1);
      status |= INP_DECODE_INFO;
    }
  }

  return status;
}

static int decoder(playa_info_t * info)
{
  int status;
  int n;
        
  status = FillYM();
  if (status < 0) {
    spool_error_message();
    return INP_DECODE_ERROR;
  }

  if (status & INP_DECODE_INFO) {
    disk_info(info, 0);
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

static char *make_author(music68_t *mus, char *tmp)
{
  if(mus->author == mus->composer || !strcmp(mus->author, mus->composer)) {
    tmp = mus->author;
  } else {
    sprintf(tmp, "%s / %s", mus->author, mus->composer);
  }
  return tmp;
}

static char * make_desc(music68_t *mus, char *tmp)
{
  sprintf(tmp, "%s - %s",
	  mus->flags.amiga
	  ? "Amiga"
	  : (mus->flags.ste ? "Atari STE" : "Atari STF"),
	  mus->replay ? mus->replay : "internal replay");
  return tmp;
}

static char * make_track(disk68_t *d, char * tmp)
{
  if (app.cur_track < 0) {
    sprintf(tmp,"%02d", d->nb_six);
  } else {
    sprintf(tmp,"%02d/%02d", app.cur_track+1, d->nb_six);
  }
  return tmp;
}

static int update_info(playa_info_t *info, disk68_t *d, music68_t *mus,
		       char *tmp)
{
/*   SDDEBUG("update info [%p] [%p]\n", d, mus); */
  info->update_mask = 0;
  if (!d) {
    return -1;
  }
  if (!mus) {
    mus = d->mus + d->default_six;
  }

/*   SDDEBUG("update info\n"); */
  playa_info_time(info, mus->time << 10);
  playa_info_desc(info, make_desc(mus,tmp));
  playa_info_artist(info, make_author(mus,tmp));
  playa_info_track(info, make_track(d,tmp));
  playa_info_title(info, mus->name);

  return 0;
}

static int disk_info(playa_info_t *info, disk68_t *d)
{
  char tmp[256];
  music68_t *mus = 0;

/*   SDDEBUG("disk info [%p]\n", d); */

  if (!d) {
    return update_info(info, app.cur_disk, app.cur_mus, tmp);
  }
/*   SDDEBUG("disk info...\n"); */

  if (mus = app.cur_mus, !mus) {
    mus = d->mus + d->default_six;
  }

  update_info(info, d, mus, tmp);

  playa_info_bits(info,1);
  playa_info_stereo(info,1);
  playa_info_frq(info, (int)app.mix.real_frq);
  playa_info_bps(info,0);
  playa_info_bytes(info,0);
  playa_info_album(info,d->name);
  playa_info_year(info,0);
  playa_info_genre(info,"chip-tune");
  playa_info_comments(info,0);

  return 0;
}

static int file_info(playa_info_t *info, const char *fname)
{
  int err = -1;
  disk68_t *d;

  d = SC68file_load((char *)fname);
  if (d) {
    err = disk_info(info, d);
    free(d);
  }
  return err;
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
    "Benjamin Gerard",    /**< Driver authors */
    "Atari & Amiga "      /**< Description */
    "music player",
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /**< User Id */
  ".sc68\0.sc6\0",        /**< Extension list */

  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(sc68_driver)
