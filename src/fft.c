/**
 @ @ingroup dcplaya_devel
 * @file    fft.c
 * @author  benjamin gerard <ben@sashipa.com>
 * 
 * @version $Id: fft.c,v 1.7 2002-11-28 04:22:44 ben Exp $
 */

#include <stdlib.h>
#include "sysdebug.h"
#include "fft.h"
#include "math_int.h"

#define FFT_SIZE    (1 << FFT_LOG_2)
#define FFT_OVERLAP 32

#define USE_FLOAT

#ifdef USE_FLOAT
# include <dc/fmath.h>
# include "float_fft.h"
typedef float fft_t;
static fft_t fft_F[FFT_SIZE];
#else
# include "int_fft.h"
typedef short fft_t;
#endif

extern short int_decibel[4096];

static short fft_R[FFT_SIZE];
static fft_t fft_I[FFT_SIZE];

static int fft_idx, fft_max, fft_fill;
static short * fft_buf = 0;
static short * pcm_buf = 0;

static void fft_lock(void)
{
}

static void fft_unlock(void)
{
}


int fft_init(int nbuffer)
{
  int fft_size, pcm_size;
  
  if (nbuffer <= 0) {
	nbuffer = 32;
  }
  SDDEBUG("[%s] (%d)\n", __FUNCTION__, nbuffer);

#ifdef USE_FLOAT
  float_fft_init();
#endif

  fft_idx    = 0;
  fft_max    = nbuffer;
  fft_fill   = 0;

  fft_size = FFT_SIZE >> 1;
  pcm_size = FFT_SIZE;

  fft_buf = malloc(nbuffer * fft_size * sizeof(*fft_buf) +
					  nbuffer * pcm_size * sizeof(*pcm_buf));
  pcm_buf = (short *)(fft_buf + (nbuffer * fft_size)); 
  return fft_buf ? 0 : -1;
}

void fft_shutdown(void)
{
  if (fft_buf) {
	free(fft_buf);
  }
  fft_buf = 0;
  fft_max = 0;
  fft_idx = 0;
}

static void fft_queue_buffer(int frq)
{
  int j;
  short * fft;
  short * pcm;

  if (!fft_buf) {
	return;
  }

  /* Get a frag buffer. */
  fft_lock();
  fft_idx = (fft_idx + 1) % fft_max;

  fft = fft_buf + ( fft_idx << (FFT_LOG_2-1) );
  pcm = pcm_buf + ( fft_idx << (FFT_LOG_2) );
  fft_unlock();

  // Copy last sample into overlapping buffer. 
  memcpy(pcm, fft_R, FFT_SIZE << 1);

#ifndef USE_FLOAT
  // Apply Hanning window
  fix_window(fft_R, FFT_SIZE);
  // Forward FFT : Time -> Frq
  fix_fft(fft_R, fft_I, FFT_LOG_2, 0);
  // Copy to final buffer.
  fft_R[FFT_SIZE/2] >>= 1;
  fft_I[FFT_SIZE/2] = 0;
  fft_R[0] >>= 1;
  fft_I[0] = 0;
 
  for (j = 0;
	   j < FFT_SIZE/2;
	   j++) {
	int rea, imm, v;
	rea = (int)fft_R[j+1];
	imm = (int)fft_I[j+1];
	v = int_sqrt(rea*rea + imm*imm);
	v  |= ((0x7FFF-v)>>31);
	v  &= 0x7FFF;
	fft[j] = v;
  }

#else
  float_fft_set_short_data(fft_F, fft_I, fft_R, FFT_LOG_2);
  float_fft(fft_F, fft_I, 0);

/*   if (fft_R[0]) { */
/* 	printf("%.03f %.03f %.03f\n", fft_F[1], fft_I[1], */
/* 		   fsqrt(fft_F[1]*fft_F[1] + fft_I[1]*fft_I[1])); */
/*   } */

  for (j = 0;
	   j < FFT_SIZE/2;
	   j++) {
	float rea, imm;
	int v;
	rea = fft_F[j+1];
	imm = fft_I[j+1];
	v = (int) ( 20.0f * fsqrt( (float)j * (rea*rea + imm*imm)));
	v  |= ((0x7FFF-v)>>31);
	v  &= 0x7FFF;
	fft[j] = v;
  }
#endif

  // Start new fft buffer 
#if FFT_OVERLAP > 0
  memcpy(fft_R, pcm + FFT_SIZE - FFT_OVERLAP, FFT_OVERLAP<<1);
# ifndef USE_FLOAT
  memset(fft_I, 0, FFT_OVERLAP<<1);
#endif
#endif


}

int fft_frag_size(void)
{
  return FFT_SIZE;
}

int fft_frag_overlap(void)
{
  return FFT_OVERLAP;
}

int fft_frags(void)
{
  return fft_max;
}

void fft_queue(int *spl, int nbSpl, int frq)
{
  int i;

  // $$$
  nbSpl >>= 2;

  i = fft_fill;
  while (nbSpl > 0) {
	int n, rem;
	n = nbSpl;
	rem = FFT_SIZE - i;
	if (rem < n) {
	  n = rem;
	}
	nbSpl -= n;
	while (n--) {
	  int v;
	  v = *spl++;

	  // $$$
	  spl += 3;

	  fft_R[i] = ((v >> 16) + (short)v) >> 1;
#ifndef USE_FLOAT
	  fft_I[i] = 0;
#endif
	  ++i;
;	}

	if (i == FFT_SIZE) {
	  fft_queue_buffer(frq);
	  i = FFT_OVERLAP;
	}
	/*
	 *   0       1     ...   255
	 * 22050  22050/2       22050/256
	*/
	
  }
  fft_fill = i;
}

static void any_copy(short * dest, short * src, int m, int n)
{
  int i = 0;
  int stp = (m << 8) / n;

  // +.........+.........+.........+.........+ 
  //        |      |

#if 0
  do {
	*dest ++ = src[i>>8];
	i += stp;
  } while (--n);

  return;
#endif

  do {
	int j, jh, ih;

	j = i + stp;
	ih = (i >> 8);
	jh = (j >> 8);
	if (ih == jh) {
	  * dest++ = src[ih];
	} else {
	  int v;
	  v = src[ih] * (256 - (i&255));
	  while (++ih < jh) {
		v += src[ih] << 8;
	  }
	  v += src[ih] * (j&255); 
	  *dest ++ = v / stp;
	}
	i = j;
  } while (--n);
}

void fft_copy(short * fft, short * pcm, int n, int db)
{
  int idx;

  if (n > 0) {
	fft_lock();
	idx = (fft_idx+1) % fft_max;
	if (fft) {
	  any_copy(fft, fft_buf + (idx << (FFT_LOG_2-1)), FFT_SIZE >> 1, n);
	  if (db) {
		int i;
		const int sub = 0;//6 * 32768 / 36;

		for (i=0; i<n; ++i) {
		  int v = fft[i];
		  if (v < 0 || v > 32767) {
			printf("[!%d] ",v);
		  }
		  v = int_decibel [ (v>>3) & 4095 ];
		  v -= sub;
		  v &= ~(v>>31);

		  //v = int_sqrt(v<<15);
		  if (v < 0 || v > 32767) {
			printf("[%d!] ",v);
		  }

		  fft[i] = v;

		}
	  }
	}
	if (pcm) {
	  any_copy(pcm, pcm_buf + (idx << (FFT_LOG_2)), FFT_SIZE, n);
	}
	fft_unlock();
  }
}
