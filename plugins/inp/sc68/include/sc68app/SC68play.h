/**
 * @ingroup   sc68app_devel
 * @file      SC68play.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/05/11
 * @brief     sc68 - play bar interface
 * @version   $Id: SC68play.h,v 1.1 2003-03-08 09:54:16 ben Exp $
 */

/*
 *                        sc68 - play bar interface
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

#ifndef _SC68PLAY_H_
#define _SC68PLAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sc68app/SC68app.h"

/** Press PLAY ON TAPE.
 *
 *  Execute following commands :
 *  - Desactiv PAUSE mode & activ PLAY mode
 *  - Load, Init & Start Track
 *  - Run callback
 *
 *  @retval 0 success
 *  - Return 0 if successed
 */
int SC68play(SC68app_t *sc68);

/** Press PAUSE ON TAPE.
 *
 *  Execute following commands :
 *  - Toggle PAUSE mode
 *  - Run callback
 *
 *  @retval 0 success
 *  - Return 0 if successed
 */
int SC68pause(SC68app_t *sc68);

/** Press STOP ON TAPE.
 *
 *  Execute following commands :
 *  -Desactiv PLAY mode
 *  -Run callback if PLAY was activated
 *
 *  @retval 0 success
 */
int SC68stop(SC68app_t *sc68);

/** Press FAST-FORWARD ON TAPE.
 *
 *  Execute following commands :
 *  - Toggle FAST-FORWARD mode
 *  - Run callback
 *  @retval 0 success
 */
int SC68ffwd(SC68app_t *sc68);

/** Press TRACK-NUMBER ON TAPE.
 *
 *  Execute following commands :
 *  - Select default track if number is outside bound
 *  - Run callback
 *
 *  @retval 0 success
 */
int SC68track(SC68app_t *sc68, int track);

/** Press NEXT-TRACK ON TAPE
 *
 *  Execute following commands :
 *  -Toggle FAST-FORWARD mode
 *  -Run callback
 *
 *  @retval 0 success
 */
int SC68ntrk(SC68app_t *sc68);

/** Press PREVIOUS-TRACK ON TAPE
 *
 *  Execute following commands :
 *  -Toggle FAST-FORWARD mode
 *  -Run callback
 *
 *  @retval 0 success
 */
int SC68ptrk(SC68app_t *sc68);

/** Insert NEW DISK :
 *
 *  Execute following commands :
 *  -EJECT previous disk
 *  -LOAD new disk
 *  -PLAY
 *
 *  @retval 0 success
 */
int SC68load(SC68app_t *sc68, disk68_t *newdisk);

/** Press EJECT ON TAPE
 *
 *  Execute following commands :
 *  -STOP music
 *  -Run callback
 *
 *  @retval 0 success
 */
int SC68eject(SC68app_t *sc68);

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef _SC68PLAY_H_ */
