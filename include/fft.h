#ifndef _FFT_H_
#define _FFT_H_

#include "int_fft.h"
 
#define FFT_LOG_2 8

extern short fft_R[], fft_I[], fft_F[];

void fft(int *spl, int nbSpl, int splFrame, int frq);

#endif
