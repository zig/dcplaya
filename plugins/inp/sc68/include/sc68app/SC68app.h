/**
 * @ingroup   sc68app_devel
 * @file      SC68app.h
 * @author    Ben(jamin) Gerard<ben@sashipa.com>
 * @date      1999/05/15
 * @brief     sc68 application main
 * @version   $Id: SC68app.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *                         sc68 - application main
 *         Copyright (C) 2001 Ben(jamin) Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef _SC68APP_H_
#define _SC68APP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/emu68.h"
#include "file68/SC68file.h"
#include "sc68app/SC68config.h"

#define SC68APP_DEFAULT_MAX_PASS 5

/** sc68 application context structure.
 */
typedef struct
{
  volatile char ready;  /**< Application ready flag */

  int version;          /**< version number */

  /* Play Bar */
  char play;            /**< Playing flag */
  char pause;           /**< Pause flag */
  char ffwd;            /**< Fast forward flag */
  char refresh_me;      /**< GUI need to be refresh */ 

  /* Loop mode */
  char eot_action;  /**< End Of Track action; 0:none 1:stop 2:next-track 3:restart */
  char eod_action;  /**< End Of Track action; 0:none 1:stop 2:next-disk 3:restart */
  char eot;         /**< End Of Track */
  char eod;         /**< End Of Disk */

  char play_at_load;   /**< launch play at disk load 0:Off 1:On */
  unsigned skip_time;  /**< track below this time are skipped */
  int force_track;     /**< forced track : -1 for default */

  int browse_info;    /**< Generate browse info flags */

  disk68_t   *cur_disk;		/**< Loaded SC68 file */
  music68_t  *cur_mus;		/**< Current music (played )*/
  music68_t  *dis_mus;    /**< Current displayed music */
  int        cur_track;   /**< Current track (music number) */
  u32        playaddr;    /**< play address in 68K memory */

  /** Time info structure. */
  struct
  {
    unsigned track,            /**< Total time for this track */
             selection;        /**< Total time for selected tracks */
    unsigned rem;              /**< Error diffusion cycle -> sec */
    unsigned rem_ms;           /**< Error diffusion cycle -> ms */
    unsigned elapsed;          /**< Current track elapsed time */
    unsigned elapsed_ms;       /**< Current track elapsed ms */
    unsigned sel_elapsed;      /**< Selection elapsed time */

    unsigned total;            /**< seconds elapsed since first play */
    int mode;                  /**< bit0: elapsed/remaind, bit1: track/selection */
    unsigned def;              /**< Default time for unknown track */
  } time;

  /** Delay info structure. */
  struct
  {
    int time;         /**< Delay Time (in mix buffer len factor), minus for divide ! */
    int strength;     /**< 0..256 */
    int onoff;        /**< 0:desactif delay processing */
    int lr;           /**< 1:actif LR spacializer mode instead of echo chamber */
    int buffer_size;  /**< Size of delay ring in sample */
  } delay;

  /**< Mixer info struture */
  struct
  {
    float real_frq;     /**< Audio frq in hz */
    u32   *buf;         /**< Audio YM mix buffer */
    int   buflen;       /**< Sample mixed by YM */
    int   stdbuflen;    /**< Sample mixed by for a pass at this frq */
    u32   cycleperpass; /**< Number of 68K cycle per mix pass */
    int   max_pass;     /**< Max pass per update */
    int   amiga_blend;  /**< Amiga LR blend factor [0..65536] */
  } mix;

  SC68config_t config;  /**< current configuration */

  reg68_t reg68;     /**< Copy of 68K registers */
  void *data;        /**< OS-dependant application data */

} SC68app_t;

/**  Check NULL application pointer validity.
 */
int SC68app_check(SC68app_t *app);

/**  Active application update.
 */
void SC68app_activ(SC68app_t *app);

/** Desactive application update.
 */
void SC68app_desactiv(SC68app_t *app);

/** Change Delay Time Parameter
 *
 * @return new delay time
 */
int SC68app_set_delay_time(SC68app_t *app, int delay_time);

/** Change Delay Strength Parameter.
 *
 * @return new delay strength
 */
int SC68app_set_delay_strength(SC68app_t *app, int delay_strength);

/** Copy config value into more friendly SC68app_t fields.
 */
int SC68app_configure(SC68app_t *app);

/**  Start current music of current disk.
 */
int SC68app_start_current_track(SC68app_t *app);

/** Stop playing, eject disk and reset hardware
 */
int SC68app_eject(SC68app_t * app);

/**  Run 68K & IO-emulation.
 *
 *  This function runs the music emulator  for a maximum of max_pass times
 *  or until AUDIO device becomes busy.
 *
 *  @return status-code
 *  @retval <0:error 0:continue, >0:quit
 */
int SC68app_update(SC68app_t *app);

/**  Load a disk file & Eventually update B.I.D.B.
 */
disk68_t *SC68app_load_diskfile(char *fname, int browseupdate);

/** Verify fname is a valid SC68 file.
 */
int SC68app_verify_diskfile(char *fname);

/** Read a file into 68K memory.
 *
 * This function is not a good idea , because it need to link with emu68
 * library.  That's why I removed it.
 */
int SC68_68Kfread(unsigned dest, FILE * f, unsigned sz);

/** Increment all time counters with 1 pass mix duration.
 */
void SC68app_advance_time(SC68app_t * app);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68APP_H_ */
