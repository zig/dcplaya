/**
 * @ingroup  dcplaya
 * @file     fft.h
 * @author   Ben(jamin) Gerard <ben@sashipa.com>
 * @date     2002/07/??
 * @brief    dcplaya FFT.
 *
 * @id $Id: fft.h,v 1.5 2002-11-14 23:40:27 benjihan Exp $
 *
 */

#ifndef _FFT_H_
#define _FFT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/* $$$ BEN: Ca macrhe pas  vraiment quand on le change ! */
#define FFT_LOG_2 9   /** FFT size (log 2) . Max is 12. */

//extern short fft_R[]; /**< FFT Real numbers.                             */
//extern short fft_I[]; /**< FFT Imaginary numbers.                        */
//extern short fft_F[]; /**< FFT final frequency scaled result [0..32767]. */
//extern short fft_D[]; /**< FFT final in Db [0..32767].                   */


/** Init FFT.
 *
 *  The fft_init() function initializes FFT buffering system.
 *
 *  @param  nbuffer   Number of FFT buffers
 *  @return error-code
 *  @retval 0 Ok
 */
int fft_init(int nbuffer);

/** Shutdown FFT interface.
 */
void fft_shutdown(void);

/** Get FFT buffer size. */
int fft_frag_size(void);
/** Get FFT buffer overlapping size. */
int fft_frag_overlap(void);
/** Get number of FFT buffer. */
int fft_frags(void);

/** Calculate FFT from PCM.
 *
 *  The fft() function calculates FFT for a given 16 bit stereo PCM buffer.
 *  Left and right channels are mixed together.
 *
 *  @param  spl       16-bit-stereo-PCM buffer.
 *  @param  nbSPl     Number of sample in spl buffer.
 *  @param  splFrame  Spl buffer id. Avoid to calcul twice (or more) for the
 *                    same samples. @b Unused.
 *  @param  frq       Playback frequency (in Hz).
 *
 */
void fft_queue(int * spl, int nbSpl, int frq);

/** */
void fft_copy(short * fft, short * pcm, int n, int db);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FFT_H_ */
