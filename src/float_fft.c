/**
 * @file    float_fft.c
 * @author  Don Cross <dcross@intersrv.com>
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/11/12
 * @brief   Floating point FFT and inverse FFT.
 *
 *   Based on fourierf.c by Don Cross <dcross@intersrv.com>.
 *   http://www.intersrv.com/~dcross/fft.html
 *
 */

#include <stdlib.h>
#include "dcplaya/config.h"
#include "sysdebug.h"
#include "math_float.h"
#include "float_fft.h"

static int * reverseBits;   /* Bit reversing table                         */
static float * sinCosTable; /* Precalculated sin/cos table                 */
static int alloc_log2;      /* log2 of allocated size for previous tables  */ 
static int fft_log2;        /* current fft log2 size (the power of 2)      */
static int fft_size;        /* current fft size ( 2^fft_log2 )             */

int float_fft_init(void)
{
  reverseBits = 0;
  sinCosTable = 0;
  alloc_log2 = 0;
  fft_log2 = 0;
  fft_size = 0;
  return 0;
}

void float_fft_shutdown(void)
{
  if (reverseBits) free(reverseBits);
  if (sinCosTable) free(sinCosTable);
  float_fft_init();
}

static int ReverseBits (int index, int NumBits)
{
  int i, rev;
  
  for ( i=rev=0; i < NumBits; i++ )  {
    rev = (rev << 1) | (index & 1);
    index >>= 1;
  }
  return rev;
}

static int resize(int numBits)
{
  int n = 1 << numBits;

  if (reverseBits) {
    free(reverseBits);
  }
  reverseBits = malloc(sizeof(*reverseBits) * n);
  if (!reverseBits) {
    goto error;
  }

  if (sinCosTable) {
    free(sinCosTable);
  }
  sinCosTable = malloc(sizeof(*sinCosTable) * (n + (n>>2)));
  if (!sinCosTable) {
    goto error;
  }
  alloc_log2 = numBits;

  SDDEBUG("float-FFT alloc : %d %d\n", alloc_log2, 1 << alloc_log2);

  return 0;

 error:
  float_fft_shutdown();
  return -1;
}

static void recalc(int numBits)
{
  if (numBits != fft_log2) {
    const int n = 1 << numBits;
    const float stp = 6.283118530748 / (float) n;
    int i;
    float a;

    /* Calculates reverse bit table. */
    for (i=0; i<n; ++i) {
      reverseBits[i] = ReverseBits(i,numBits);
      //	  SDDEBUG("reverse %04x => %04x\n", i, reverseBits[i]);
    }

    /* Calculates sin/cos table.
     * sin (a+PI/2) = cos (a)
     * cos(0) = sin(90)
     */
	
    for (i=0, a=0; i<n; ++i, a+=stp) {
      sinCosTable[i] = Sin(a);
      //	  SDDEBUG("sin[%03x] = %.02f\n", i, sinCosTable[i]);
    }
    for (; i<n+(n>>2); ++i) {
      sinCosTable[i] = sinCosTable[i-n];
      //	  SDDEBUG("sin[%03x] = %.02f\n", i, sinCosTable[i]);
    }

    fft_log2 = numBits;
    fft_size = n;
  }
}

int float_fft_set_short_data(float * realOut, float * imagOut,
			     const short * realIn, int numBits)
{
  int i, n;

  if (numBits != alloc_log2 && resize(numBits) < 0) {
    return -1;
  }
  recalc(numBits);
  n = 1 << numBits;
  for (i=0; i<n; ++i) {
    const float sc = 1.0f / 32768.0f;
    int j = reverseBits[i];
    realOut[j] = sc * (float) realIn[i];
    imagOut[j] = 0;
  }

  return 0;
}


int float_fft_set_data(float * realOut, float * imagOut,
		       const float * realIn,
		       int numBits)
{
  int i, n;

  if (numBits != alloc_log2 && resize(numBits) < 0) {
    return -1;
  }
  recalc(numBits);
  n = 1 << numBits;
  for (i=0; i<n; ++i) {
    int j = reverseBits[i];
    realOut[j] = realIn[i];
    imagOut[j] = 0;
  }

  return 0;
}

void float_fft(float * realOut, float * imagOut, int inverse)
{
  const int angle_numerator = inverse ? fft_size : -fft_size;
  const float * const sinTable = sinCosTable;
  const float * const cosTable = sinCosTable + (fft_size >> 2);

  if (!fft_log2) {
    return;
  }
  
  /*
  **   Do the FFT itself...
  */
  int blk_log2;

  for (blk_log2 = 1;
       blk_log2 <= fft_log2;
       ++blk_log2) {

    const int blk_size = 1 << blk_log2;
    const int blk_end  = blk_size >> 1; 

    const int delta_angle = angle_numerator >> blk_log2;
    const int angle1 = delta_angle & (fft_size-1);
    const int angle2 = (angle1 << 1) & (fft_size-1);
	  
    const float sm2 = sinTable[angle2];
    const float sm1 = sinTable[angle1];
    const float cm2 = cosTable[angle2];
    const float cm1 = cosTable[angle1]; 
    const float w   = cm1 * 2.0f;

    int i;

    for (i = 0; i < fft_size; i += blk_size) {
      int j, n;

      float ar2,ar1,ar0;
      float ai2,ai1,ai0;

      ar2 = cm2;
      ar1 = cm1;
		  
      ai2 = sm2;
      ai1 = sm1;

      for (j = i, n = j + blk_end; j < n; ++j) {
	float tr, ti;
	int k;

	ar0 = w*ar1 - ar2;
	ar2 = ar1;
	ar1 = ar0;
			
	ai0 = w*ai1 - ai2;
	ai2 = ai1;
	ai1 = ai0;
			
	k = j + blk_end;
	tr = ar0 * realOut[k] - ai0 * imagOut[k];
	ti = ar0 * imagOut[k] + ai0 * realOut[k];
			
	realOut[k] = realOut[j] - tr;
	imagOut[k] = imagOut[j] - ti;
		
	realOut[j] += tr;
	imagOut[j] += ti;
      }
    }
  }

  /* Need to normalize if inverse transform.
   */
  if (inverse) {
    const float scale = Inv((float) fft_size);
    int i;

    for (i=0; i < fft_size; i++) {
      realOut[i] *= scale;
      imagOut[i] *= scale;
    }
  }
}

