/* $Id: fft.h,v 1.2 2002-09-06 23:16:09 ben Exp $ */

#ifndef _FFT_H_
#define _FFT_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "int_fft.h"
 
#define FFT_LOG_2 8

extern short fft_R[], fft_I[], fft_F[];

void fft(int *spl, int nbSpl, int splFrame, int frq);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FFT_H_ */
