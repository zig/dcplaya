#include "fft.h"
#include "math_int.h"

short fft_F[1<<FFT_LOG_2];
short fft_R[1<<FFT_LOG_2], fft_I[1<<FFT_LOG_2];

extern short int_decibel[4096];

void fft(int *spl, int nbSpl, int splFrame, int frq)
{
  int i,j, stp = nbSpl<<(12-FFT_LOG_2);
  
  // Copy sample to complex buffer
  for (j = i = 0; i<(1<<FFT_LOG_2) ; ++i, j+=stp) {
    int v = spl[j>>12];
    fft_R[i] = ((v >> 16) + (short)v) >> 1;
    fft_I[i] = 0;
  }
  // Apply Hanning window
  fix_window(fft_R, 1<<FFT_LOG_2);
  // Forward FFT : Time->Frq
  fix_fft(fft_R, fft_I, FFT_LOG_2, 0);

  // Copy final buffer
  for (j = 0; j<(1<<(FFT_LOG_2-1)); j++) {
    int rea, imm, v;
    rea = (int)fft_R[j];
    imm = (int)fft_I[j];
    v   = int_sqrt(rea*rea + imm*imm);

#if 0		
    v = (unsigned int)v>>1;
    v |= ((4095-v)>>31);
    v &= 4095;
    v = (32767-int_decibel[v]) << 1;
    if (v<0) {
      *(int *)1 = 0xAABBCCDD;
    }
    v -= (2<<12); // 6db attenuation for the omitted alias components
    //    v = v & ~(v>>31);
#endif
    v &= ~(v>>31);
    fft_F[j] = v;
    if (v<0) {
      *(int *)1 = 0x55667788;
    }
  }
}
