/**
 * @ingroup dcplaya_devel
 * @file    fftband.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/12/29
 * @brief   fft frequency band.
 *
 * $Id: fftband.h,v 1.4 2003-01-25 11:37:44 ben Exp $ 
 */
 
#ifndef _FFTBAND_H_
#define _FFTBAND_H_

#include <arch/types.h>

/** Frequency band limits. */
typedef struct {
  unsigned int fmin; /**< Band lower frequancy (in Hz). */
  unsigned int fmax; /**< Band higher frequancy (in Hz). */
} fftband_limit_t;

/** Frequency band info. */
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

/** Set of frequancy band. */
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

fftbands_t * fftband_create(int n, int fft_size, int sampling,
			    const fftband_limit_t * limits);

void fftband_update(fftbands_t * bands, const uint16 * fft);


#endif /* #define _FFTBAND_H_ */

