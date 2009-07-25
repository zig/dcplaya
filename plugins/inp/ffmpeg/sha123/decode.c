/* 
 * Mpeg Layer-1,2,3 audio decoder 
 * ------------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README'
 *
 */

#include "sha123/dct64.h"
#include "sha123/decode.h"
#include "sha123/tables.h"

int sha123_synth_1to1_mono(const real * bandPtr,
			   unsigned char *samples,
			   int *pnt)
{
  pcm_t samples_tmp[64];
  pcm_t *tmp1 = samples_tmp;
  int i, ret;
  int pnt1 = 0;
  
  ret = sha123_synth_1to1(bandPtr, 0, (unsigned char *) samples_tmp, &pnt1);
  samples += *pnt;
  
  for (i = 0; i < 32; i++) {
    *((pcm_t *) samples) = *tmp1;
    samples += 2;
    tmp1 += 2;
  }
  *pnt += 64;
  
  return ret;
}

int sha123_synth_1to1(const real * bandPtr,
		      int channel,
		      unsigned char *out,
		      int *pnt)
{
  static real buffs[2][2][0x110];
  static const int step = 2;
  static int bo = 1;
  pcm_t *samples = (pcm_t *) (out + *pnt);
  
  real *b0, (*buf)[0x110];
  int clip = 0;
  int bo1;
  int i = 0;

  if (!channel) {
    bo--;
    bo &= 0xf;
    buf = buffs[0];
  } else {
    samples++;
    buf = buffs[1];
  }

  if (bo & 0x1) {
    b0 = buf[0];
    bo1 = bo;
    sha123_dct64(buf[1] + ((bo + 1) & 0xf), buf[0] + bo, bandPtr);
  } else {
    b0 = buf[1];
    bo1 = bo + 1;
    sha123_dct64(buf[0] + bo, buf[1] + bo + 1, bandPtr);
  }

  {
    register int j;
    real *window = sha123_decwin + 16 - bo1;

    for (j = 16; j; j--, window += 0x10, samples += step) {
      real sum;
      
      sum  = *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      sum += *window++ * *b0++;
      sum -= *window++ * *b0++;
      i++;
      SHA123_WRITE_SAMPLE(samples, sum, clip);
    }

    {
      real sum;

      sum  = window[0x0] * b0[0x0];
      sum += window[0x2] * b0[0x2];
      sum += window[0x4] * b0[0x4];
      sum += window[0x6] * b0[0x6];
      sum += window[0x8] * b0[0x8];
      sum += window[0xA] * b0[0xA];
      sum += window[0xC] * b0[0xC];
      sum += window[0xE] * b0[0xE];
      SHA123_WRITE_SAMPLE(samples, sum, clip);

      b0 -= 0x10, window -= 0x20, samples += step;
    }
    window += bo1 << 1;

    for (j = 15; j; j--, b0 -= 0x20, window -= 0x10, samples += step) {
      register real sum;

      sum = -*(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      sum -= *(--window) * *b0++;
      SHA123_WRITE_SAMPLE(samples, sum, clip);
    }
  }

  *pnt += 128;

  return clip;
}
