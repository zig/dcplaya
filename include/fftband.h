/**
 * @ingroup dcplaya_fftband_devel
 * @file    fftband.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/12/29
 * @brief   fft frequency band.
 *
 * $Id: fftband.h,v 1.5 2003-03-22 00:35:26 ben Exp $ 
 */
 
#ifndef _FFTBAND_H_
#define _FFTBAND_H_

#include <arch/types.h>

/**
 * @defgroup dcplaya_fftband_devel  fft frequency band
 * @ingroup  dcplaya_devel
 * @brief   fft frequency band
 *
 */

/** Frequency band limits.
 *  @ingroup dcplaya_fftband_devel
 */
typedef struct {
  unsigned int fmin; /**< Band lower frequency (in Hz). */
  unsigned int fmax; /**< Band higher frequency (in Hz). */
} fftband_limit_t;

/** Frequency band info.
 *  @ingroup dcplaya_fftband_devel
 */
typedef struct {
  unsigned int fmin;    /**< Band lower frequency (in Hz). */
  unsigned int fmax;    /**< Band higher frequency (in Hz). */
  unsigned int bmin;    /**< First fft band. */
  unsigned int bmax;    /**< Last fft band. */
  unsigned int bminscale; /**< First fft band scaling factor. */
  unsigned int bmaxscale; /**< Last fft band scaling factor. */
  unsigned int bdivisor;  /**< fft band power divisor. */
/*   unsigned int tap[4];    /\**< Holds previous band values. *\/ */
/*   unsigned int tapacu;    /\**< Index of current value in tap[]. *\/ */
  unsigned int v;

} fftband_t;

/** Set of frequency band.
 *  @ingroup dcplaya_fftband_devel
 */
typedef struct {
  int n;                 /**< Number of band. */
  unsigned int oof0;     /**< 1/f0. */
  unsigned int fftsize;  /**< Number of sample in FFT buffer */
  unsigned int sampling; /**< Sampling frequency. */
  unsigned int loudness; /**< Loudness (linear). */
  unsigned int imin;     /**< index of minimal band */
  unsigned int imax;     /**< index of maximal band */

  /* Keep at end of struct ! */
  fftband_t band[1];     /**< Frequency band buffer. */
} fftbands_t;

/** Create a frequency band analyser.
 *  @ingroup dcplaya_fftband_devel
 *
 *   The fftband_create() fucntion creates a fftbands_t structure.
 *   If no limits is given the function will create a logarithmic scale to
 *   suit human ear properties.
 *
 *  @param  n         Number of band (>0)
 *  @param  fft_size  Size of te FFT used to fill the bands. 
 *  @param  sampling  PCM sampling rate (in Hz)
 *  @param  limits    Optionnal band description.
 *
 *  @return fft bands pointer
 *  @retval 0 on error
 *  @retval !0 valid pointer on fftbands_t (must be freed after use).
 */

fftbands_t * fftband_create(int n, int fft_size, int sampling,
			    const fftband_limit_t * limits);

/** Fill a frequency band analyser.
 *  @ingroup dcplaya_fftband_devel
 *
 *  @param  bands     fft bands to fill
 *  @param  fft       fft buffer (must be compatible with parameters
 *                    when fftbands_t was created.)
 *
 *  @see fftband_create()
 */
void fftband_update(fftbands_t * bands, const uint16 * fft);


#endif /* #define _FFTBAND_H_ */

