/**
 * @ingroup  dcplaya
 * @file     fft.h
 * @author   Ben(jamin) Gerard <ben@sashipa.com>
 * @date     2002/07/??
 * @brief    dcplaya FFT.
 *
 * @id $Id: fft.h,v 1.6 2002-12-30 06:28:18 ben Exp $
 *
 */

#ifndef _FFT_H_
#define _FFT_H_

#include "extern_def.h"
#include "fftband.h"

DCPLAYA_EXTERN_C_START

/* $$$ BEN: Ca macrhe pas  vraiment quand on le change ! */
#define FFT_LOG_2 9   /** FFT size (log 2) . Max is 12. */

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

/** */
/* void fft_copy(short * fft, short * pcm, int n, int db); */

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FFT_H_ */
