/**
 * @ingroup   sc68app_devel
 * @file      SC68mixer.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/05/17
 * @brief     sc68 - audio mixer
 * @version   $Id: SC68mixer.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *                        sc68 - sc68 - audio mixer
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

#ifndef _SC68MIXER_H_
#define _SC68MIXER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/type68.h"

#define SC68MIXER_SAME_SIGN 0
#define SC68MIXER_CHANGE_LEFT_CHANNEL_SIGN  0x00008000
#define SC68MIXER_CHANGE_RIGHT_CHANNEL_SIGN 0x80000000
#define SC68MIXER_CHANGE_SIGN 0x80008000

/** Mix 16-bit-stereo value into 16-bit-stereo.
 *  res = buf^sign
 *  sign=0          : Keep input sign,
 *  sign=0x00008000 : Change left channel sign
 *  sign=0x80000000 : Change right channel sign
 *  sign=0x80008000 : Change both channel
*/
void SC68mixer_stereo_16_LR(u32 *dest, u32 *src, int nb, u32 sign);

/** Mix 32-bit-stereo value into 16-bit-stereo with CHANNEL SWAPPING.
 */
void SC68mixer_stereo_16_RL(u32 *dest, u32 *src, int nb, u32 sign);

/** Mix 16-bit-stereo value into 32-bit-stereo-float (-1..1).
 */
void SC68mixer_stereo_FL_LR(float *dest, u32 *src, int nb, u32 sign);

/** Duplicate left channel into right channel before EOR transform.
 *  res = (buf|(buf<<16))^sign
 */
void SC68mixer_dup_L_to_R(u32 *dest, u32 *src, int nb, u32 sign);

/** Duplicate right channel into left channel before EOR transform.
 *  res = (buf|(buf>>16))^sign
 */
void SC68mixer_dup_R_to_L(u32 *dest, u32 *src, int nb, u32 sign);

/** Blend Left and right voice.
 *  factor [0..65536], 0:blend nothing, 65536:swap L/R
 */
void SC68mixer_blend_LR(u32 *dest, u32 *src, int nb, int factor, u32 sign);

/** Multiply left/right channel by ml/mr factor [0..65536].
*/
void SC68mixer_mult_LR(u32 *dest, u32 *src, int nb, unsigned ml, unsigned mr, u32 sign);

/** Fill buffer sign with value (RRRRLLLL).
 */
void SC68mixer_fill(u32 *dest, int nb, u32 sign);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68MIXER_H_ */
