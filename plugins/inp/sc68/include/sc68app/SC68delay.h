/**
 * @ingroup   sc68app_devel
 * @file      SC68delay.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/07/27
 * @brief     Delay processing functions
 * @version   $Id: SC68delay.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *                         sc68 - delay processor
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


#ifndef _SC68DELAY_H_
#define _SC68DELAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/type68.h"

/** Create delay buffer.
 *
 * @param   buffer_size  Round to a greater or equal power of 2
 *
 * @return  new buffer size
 */
int SC68delay_init(int buffer_size);

/** Kill delay buffers.
 */
void SC68delay_kill();

/* Apply delay to buffer.
 *
 * @param   buffer    buffer to delay
 * @param   nb        number of sample in buffer
 * @param   delay     delay value (in sample)
 * @param   strength  mix buffer volume [0..256]
 */
void SC68delay(u32 *buffer, int nb, int delay, int strenght, int sign);

/** Apply a Left to right delay.
 *
 * @param   buffer  L-R buffer to delay
 * @param   nb      number of sample in buffer
 * @param   delay   delay value (in sample)
 */
void SC68delay_LR(u32 *buffer, int nb, int delay);

/** Get Delay ring default strength.
 */
int SC68delay_default_strength(void);

/**  Get Delay ring default strength.
 */
int SC68delay_default_buffer_size(void);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68DELAY_H_ */
