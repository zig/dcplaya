/**
 * @ingroup  dcplaya_fft
 * @file     fft.h
 * @author   benjamin gerard
 * @date     2002/07/??
 * @brief    dcplaya FFT.
 *
 * @id $Id: fft.h,v 1.7 2003-03-26 23:02:47 ben Exp $
 *
 */

#ifndef _FFT_H_
#define _FFT_H_

#include "extern_def.h"
#include "fftband.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_fft  Fast Fourrier Transform
 *  @ingroup  dcplaya_math_devel
 *  @brief    fast fourrier transform
 *  @author   benjamin gerard
 *  @{
 */

/* $$$ BEN: Ca macrhe pas  vraiment quand on le change ! */
#define FFT_LOG_2 9   /**< FFT size (log 2) . Max is 12. */

//extern short fft_R[]; /**< FFT Real numbers.                             */
//extern short fft_I[]; /**< FFT Imaginary numbers.                        */
//extern short fft_F[]; /**< FFT final frequency scaled result [0..32767]. */
//extern short fft_D[]; /**< FFT final in Db [0..32767].                   */


/** Init FFT.
 *
 *  The fft_init() function initializes FFT system.
 *
 *  @return error-code
 *  @retval 0 Ok
 */
int fft_init(void);

/** Shutdown FFT interface.
 */
void fft_shutdown(void);

/** Calculate FFT from PCM.
 */
void fft_queue(void);

/** Create a set of frequency band matching the fft. */
fftbands_t * fft_create_bands(int n, const fftband_limit_t * limits);

/** Fill a set of frequency band with the current fft data. */
void fft_fill_bands(fftbands_t * bands);

/** Fill a pcm buffer with current pcm data. */
void fft_fill_pcm(short * pcm, int n);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FFT_H_ */
