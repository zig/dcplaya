/**
 * @file    fft.c
 * @author  benjamin gerard <ben@sashipa.com>
 * 
 * @version $Id: fft.c,v 1.3 2002-09-13 00:27:11 ben Exp $
 */
#include "sysdebug.h"

#include "int_fft.h"
#include "fft.h"
#include "math_int.h"

short fft_D[1<<FFT_LOG_2];
short fft_F[1<<FFT_LOG_2];
short fft_R[1<<FFT_LOG_2], fft_I[1<<FFT_LOG_2];

extern short int_decibel[4096];

void fft(int *spl, int nbSpl, int splFrame, int frq)
{
  int i,j, stp, re_frq;
  int scale;
#if DEBUG
  static int vmax = 0, vmax2 = 0, dbmax = 0;
#endif

  stp = (nbSpl << 12) >> FFT_LOG_2;

  // Copy sample to complex buffer / resampling !
  for (j = i = 0; i<(1<<FFT_LOG_2) ; ++i, j+=stp) {
    int v = spl[j>>12];
    fft_R[i] = ((v >> 16) + (short)v) >> 1;
    fft_I[i] = 0;
  }
  // Calculates new sampling frequency :
  re_frq = (frq << FFT_LOG_2) / nbSpl;

  // Apply Hanning window
  fix_window(fft_R, 1<<FFT_LOG_2);
  // Forward FFT : Time->Frq
  fix_fft(fft_R, fft_I, FFT_LOG_2, 0);

  // Copy to final buffer.
  // $$$ Here we need to rescale for each frequency.
  for (j = 0, scale = (1<<12), stp = (re_frq >> (FFT_LOG_2+FFT_LOG_2-12));
       j<(1<<(FFT_LOG_2-1));
       j++, scale += stp) {
    int rea, imm, v;
    rea = (int)fft_R[j];
    imm = (int)fft_I[j];
    v   = int_sqrt(rea*rea + imm*imm);

#if DEBUG
    if (v>vmax2) {
      vmax2 = v;
      //      SDDEBUG("vmax2:%5d\n", v);
    }
#endif

    v = (v * scale) >> 12;

#if DEBUG
    if (v > vmax) {
      vmax = v;
      //      SDDEBUG("vmax:%5d\n", v);
    }
#endif

    v |= ((0x7FFF-v)>>31);
    v &= 0x7FFF;
#if DEBUG
    if (v<0 || v>32767) {
      BREAKPOINT(0xDEAD0567);
    }
#endif
    fft_F[j] = v;

    v -= 256;
    if (v<0) v = 0;

    /*  Db calculation */
    v >>= (15-12);
    v = (32767-int_decibel[v]);

#if DEBUG
    if (v > dbmax) {
      dbmax = v;
      //      SDDEBUG("dbmax:%d\n", v);
    }

    if (v<0 || v>32767) {
      BREAKPOINT(0xDEAD7619);
    }
#endif

    //    v -= (2<<12); // 6db attenuation for the omitted alias components
    v = v & ~(v>>31);
#if DEBUG
    if (v < 0 || v > 32767) {
      BREAKPOINT(0xDEAD1463);
    }
#endif
    fft_D[j] = v;
  }
}
