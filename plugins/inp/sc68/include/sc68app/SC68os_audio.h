/**
 * @ingroup   sc68app_devel
 * @file      SC68os_audio.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/05/12
 * @brief     sc68 - audio device interface
 * @version   $Id: SC68os_audio.h,v 1.1 2003-03-08 09:54:15 ben Exp $
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

#ifndef _SC68OS_AUDIO_H_
#define _SC68OS_AUDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/** Open audio output.
 * @return real frequency for opened sound stream or
 * @retval <0 if error
 */
float SC68os_audio_open(float frq);

/** Close audio output
 */
void SC68os_audio_close(void);

/** Write to audio output.
 *
 * @param a Pointer to 16 bit stereo sample buffer ( machine endian )
 * @param n Buffer size ( in sample L/R : 32 bit )
 *
 * @return number of sample not send
 */
int SC68os_audio_write(void *a, int n);

/** Flush audio buffer.
 *
 * Kill bufferized sample. Useful when using big buffer and wants immediat
 * reaction.
 *
 */
void SC68os_audio_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68OS_AUDIO_H_ */
