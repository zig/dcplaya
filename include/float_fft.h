/**
 * @ingroup dcplaya_floatfft
 * @file    float_fft.h
 * @author  Don Cross <dcross@intersrv.com>
 * @author  benjamin gerard
 * @date    2002/11/12
 * @brief   Floating point FFT and inverse FFT.
 *
 *   Based on fourierf.c by Don Cross <dcross@intersrv.com>.
 *
 *   http://www.intersrv.com/~dcross/fft.html
 *
 */

#ifndef _FLOAT_FFT_H_
#define _FLOAT_FFT_H_

/** @defgroup dcplaya_floatfft  Floating point FFT and inverse FFT
 *  @ingroup  dcplaya_fft
 *  @brief    floating point FFT and inverse FFT
 *  @author  Don Cross
 *  @author  benjamin gerard
 *  @{
 */

/** Initialize the FFT.
 *  @return error-code
 *  @retval 0 success (should never failed here)
 */
int float_fft_init(void);

/** Shutdown the FFT.
 * 
 *  Free some internal buffers.
 */
void float_fft_shutdown(void);

/** Set FFT data (PCM) from short integer source.
 *
 *    The float_fft_set_short_data() fill the realOut with the values 
 *    stored in the realIn buffer after normalization (dividing by 32768).
 *    The imagOut buffer is cleared. On first call or if the size change
 *    from the previous call, the function will calculate some internal
 *    buffers (sinus and bit reversal), including a risk for memory
 *    allocation failure and cpu overhead. 
 *
 *  @param  realOut  Filled floating point buffer
 *  @param  imagOut  Cleared floating point buffer 
 *  @param  realIn   Source real buffer
 *  @param  numBits  Log2 of number of value to set (effective is 2^numBits).
 *  @return error-code
 *  @retval 0 sucess
 *
 *  @see float_fft_set_data()
 */
int float_fft_set_short_data(float * realOut, float * imagOut,
			     const short * realIn, int numBits);

/** Set FFT data (PCM) from normalized floating source.
 *  @param  realOut  Filled floating point buffer
 *  @param  imagOut  Cleared floating point buffer 
 *  @param  realIn   Source real buffer
 *  @param  numBits  Log2 of number of value to set (effective is 2^numBits).
 *  @return error-code
 *  @retval 0 sucess
 *  @see float_fft_set_short_data() for details.
 */
int float_fft_set_data(float * realOut, float * imagOut,
		       const float * realIn, int numBits);

/** Compute the FFT ot inverse FFT.
 *
 *  @param  realOut  floating point buffer holding real
 *  @param  imagOut  floating point buffer holding imaginary
 *  @param  inverse  0 : forward FFT (time to freq domain)
 *                   1 : backward(inverse) FFT (freq to time domain)
 */
void float_fft(float * realOut, float * imagOut, int inverse);

/**@}*/

#endif /* #define  _FLOAT_FFT_H_ */
