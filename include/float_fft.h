/**
 * @file    float_fft.h
 * @author  Don Cross <dcross@intersrv.com>
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/11/12
 * @brief   Floating point FFT and inverse FFT.
 *
 *   Based on fourierf.c by Don Cross <dcross@intersrv.com>.
 *   http://www.intersrv.com/~dcross/fft.html
 *
 */

#ifndef _FLOAT_FFT_H_
#define _FLOAT_FFT_H_

int float_fft_init(void);
void float_fft_shutdown(void);

int float_fft_set_short_data(float * realOut, float * imagOut,
							 const short * realIn, int numBits);

int float_fft_set_data(float * realOut, float * imagOut,
					   const float * realIn, int numBits);

void float_fft(float * realOut, float * imagOut, int inverse);

#endif /* #define  _FLOAT_FFT_H_ */

