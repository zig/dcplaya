/** @ingroup dcplaya_intfft
 *  @file    int_fft.h
 *  @author  Tom Roberts
 *  @author  Malcolm Slaney
 *  @date    1999/08/11
 *  @brief   Fixed-point Fast Fourier Transform
 */


#ifndef _INT_FFT_H_
#define _INT_FFT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_intfft Fixed-point Fast Fourier Transform
 *  @ingroup  dcplaya_fft
 *  @brief    Fixed-point Fast Fourier Transform
 *
 *  All data are fixed-point short integers, in which
 *  -32768 to +32768 represent -1.0 to +1.0. Integer arithmetic
 * is used for speed, instead of the more natural floating-point.
 *
 *  For the forward FFT (time -> freq), fixed scaling is
 *  performed to prevent arithmetic overflow, and to map a 0dB
 *  sine/cosine wave (i.e. amplitude = 32767) to two -6dB freq
 *  coefficients; the one in the lower half is reported as 0dB
 *  by fix_loud(). The return value is always 0.
 *
 *  For the inverse FFT (freq -> time), fixed scaling cannot be
 *  done, as two 0dB coefficients would sum to a peak amplitude of
 *  64K, overflowing the 32k range of the fixed-point integers.
 *  Thus, the fix_fft() routine performs variable scaling, and
 *  returns a value which is the number of bits LEFT by which
 *  the output must be shifted to get the actual amplitude
 *  (i.e. if fix_fft() returns 3, each value of fr[] and fi[]
 *  must be multiplied by 8 (2**3) for proper scaling.
 *  Clearly, this cannot be done within the fixed-point short
 *  integers. In practice, if the result is to be used as a
 *  filter, the scale_shift can usually be ignored, as the
 *  result will be approximately correctly normalized as is.
 *
 *  @author  Tom Roberts
 *  @author  Malcolm Slaney
 *  @author  benjamin gerard
 *  @{
 */

/** fixed integer type. */
#ifndef fixed
#define fixed short
#endif

/** Perform fixed-point multiplication. */
fixed fix_mpy(fixed a, fixed b);

/** Perform FFT or inverse FFT. */
int fix_fft(fixed fr[], fixed fi[], int m, int inverse);

/** Applies a Hanning window to the (time) input.*/
void fix_window(fixed fr[], int n);

/** Calculates the loudness of the signal, for each freq point.
 *
 * Result is an integer array, units are dB (values will be negative).
 */
int fix_loud(fixed loud[], fixed fr[], fixed fi[], int n, int scale_shift);

/** find loudness (in dB) from the complex amplitude. */
int db_from_ampl(fixed re, fixed im);

/** scale an integer value by (numer/denom). */
int fix_iscale(int value, int numer, int denom);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _INT_FFT_H_ */

