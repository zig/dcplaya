/*      fix_fft.h - Fixed-point Fast Fourier Transform  */

#ifndef _INT_FFT_H_
#define _INT_FFT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#ifndef fixed
#define fixed short
#endif

fixed fix_mpy(fixed a, fixed b);
int fix_fft(fixed fr[], fixed fi[], int m, int inverse);
void fix_window(fixed fr[], int n);
int fix_loud(fixed loud[], fixed fr[], fixed fi[], int n, int scale_shift);
int db_from_ampl(fixed re, fixed im);
int fix_iscale(int value, int numer, int denom);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _INT_FFT_H_ */

