/**
 * @ingroup dcplaya_devel
 * @file    fft.c
 * @author  benjamin gerard <ben@sashipa.com>
 * 
 * @version $Id: fft.c,v 1.12 2003-01-05 18:08:39 zigziggy Exp $
 */

#include <stdlib.h>
#include <arch/spinlock.h>
#include "sysdebug.h"
#include "fft.h"
#include "int_fft.h"
#include "fifo.h"
#include "math_int.h"


#define FFT_SIZE (1 << FFT_LOG_2)
/*#define FFT_FRQ  16000*/
#define FFT_FRQ  12000
#define PCM_SIZE (FFT_SIZE * 50000 / FFT_FRQ)


extern short int_decibel[4096];

static int pcm_buf[PCM_SIZE];        /* PCM read buffer */
static short pcm_F[2][FFT_SIZE];     /* PCM final       */
static short fft_F[2][FFT_SIZE/2+1]; /* FFT final       */
static short fft_R[FFT_SIZE];        /* FFT real        */
static short fft_I[FFT_SIZE];        /* FFT imaginary   */
static int cur_fft_buf;
static int pcm_start, pcm_end;

static spinlock_t mutex;

int fft_init(void)
{
  cur_fft_buf = 0;
  spinlock_init(&mutex);
  pcm_start = 0;
  pcm_end   = FFT_SIZE;
  return 0;
}

void fft_shutdown(void)
{
}

extern int playa_get_frq(void); /* playa.c */

void fft_queue(void)
{
  const int pcm_frq = playa_get_frq();
  int j;//, scale;

  short * fft = fft_F[cur_fft_buf ^ 1];
  short * pcm0 = pcm_F[cur_fft_buf ^ 1];
  short * pcm1 = pcm_F[cur_fft_buf ^ 0];

  if (pcm_frq <= 0) {
    memset(pcm0, 0, sizeof(*pcm0) * FFT_SIZE);
    fifo_readbak(pcm_buf, PCM_SIZE); /* Safety net : flush bak-buffer */
  } else {
    int pcm_off, read, inbuf, wanted_pcm, step;

    pcm_off = 0;
    wanted_pcm = (FFT_SIZE * pcm_frq + FFT_FRQ - 1) / FFT_FRQ;
    if (wanted_pcm > PCM_SIZE) {
      SDWARNING("[%s] : to many wanted PCM (%d > %d)\n",
		__FUNCTION__, wanted_pcm, PCM_SIZE);
      wanted_pcm = PCM_SIZE;
    }
    read = fifo_readbak(pcm_buf, wanted_pcm);
    inbuf = read * FFT_FRQ / pcm_frq;
    if (inbuf < FFT_SIZE) {
      pcm_off = FFT_SIZE - inbuf;
      memcpy(pcm0, pcm1+FFT_SIZE-pcm_off, pcm_off * sizeof(*pcm0));
    }
    step = (pcm_frq << 12) / FFT_FRQ;
    for (j=0; pcm_off<FFT_SIZE; ++pcm_off, j += step) {
      int v = pcm_buf[j >> 12];
      pcm0[pcm_off] = ((short)v + (v>>16)) >> 1;
    }
    if (j > (read<<12)) {
      SDWARNING("[%s] : overflow %x > %x\n", __FUNCTION__, j, read<<12);
    }
  }

  // Setup FFT data (real and imaginary).
  memcpy(fft_R,pcm0,FFT_SIZE*sizeof(*fft_R));
  memset(fft_I,0,FFT_SIZE*sizeof(*fft_I));

  // Apply Hanning window
  fix_window(fft_R, FFT_SIZE);

  // Forward FFT : Time -> Frq
  fix_fft(fft_R, fft_I, FFT_LOG_2, 0);

  // Copy to final buffer.
  fft_R[FFT_SIZE/2] >>= 1;
  fft_I[FFT_SIZE/2] = 0;
  fft_R[0] >>= 1;
  fft_I[0] = 0;

  fft[0] = fft_R[0] < 0 ? -fft_R[0] : fft_R[0];
  for (j = 1; j <= FFT_SIZE/2; ++j) {
    int rea, imm, v;
    rea = (int)fft_R[j];
    imm = (int)fft_I[j];
    v = int_sqrt(j * (rea*rea + imm*imm));
    v  |= ((0x7FFF-v)>>31);
    v  &= 0x7FFF;
    fft[j] = v;
  }

  /* Find pcm min and max ...  */
  {
    int pcm_acu, pcm_amp;
    short pcm_min,pcm_max, pcm_mid, pcm_avg, pcm_thrmin, pcm_thrmax;
    int start,end;
    pcm_acu = pcm_min = pcm_max = *pcm0;
    for (j=1; j<FFT_SIZE; ++j) {
      short v = pcm0[j];
      if (v < pcm_min) {
	pcm_min = v;
      } else if (v>pcm_max) {
	pcm_max = v;
      }
      pcm_acu += v;
    }
    pcm_avg = pcm_acu >> FFT_LOG_2;
    pcm_mid = (pcm_min + pcm_max) >> 1;
    pcm_amp = pcm_max - pcm_min;
    pcm_thrmin = pcm_min + (pcm_amp>>2);
    pcm_thrmax = pcm_max - (pcm_amp>>2);

    /* Default */
    start = end = -1;

    /* find 1st value below mid point */
    do {
      int center = pcm_mid;
      int crossing = -1;
      int old,v,amp;
      int tmin = pcm_thrmin - center;
      int tmax = pcm_thrmax - center;

      old = pcm0[0] - center;
      if (old >= -(pcm_amp>>7) && old <= -(pcm_amp>>7)) {
	crossing = 0;
      }
      amp = old;
      for (j=1; j<FFT_SIZE; ++j) {
	v = pcm0[j] - center;
	if ((v^old) < 0) {
	  if (amp <= tmin) {
	    end = j;
	  } else if (amp >= tmax && start < 0) {
	    start = crossing;
	  }
	  amp = 0;
	  crossing = j;
	}
	if (v < 0) {
	  if (v < amp) amp = v;
	} else if (v > amp) {
	  amp = v;
	}
	old = v;
      }

    } while (0);

/*     { */
/*       static int cnt = 0; */
/*       if (++cnt > 512) { */
/* 	cnt = 0; */
/* 	SDDEBUG("%d - %d\n", start, end); */
/*       } */
/*     } */

    start = (start < 0) ? 0 : start;
    end = (end <= start) ? FFT_SIZE : end;

    pcm_start = start;
    pcm_end = end;
  }

#if 0
  /* Inverse FFT */
  scale = fix_fft(fft_R, fft_I, FFT_LOG_2, 1);
  for (j=0; j<FFT_SIZE; ++j) {
    int r = fft_R[j]<<scale;
    int i = fft_I[j]<<scale;
    int v = (r*r + i*i) >> 15;// << scale;
/*     if (v<32768) v = 32768; */
/*     else if (v>32767) { */
/*       v = 32767; */
/*     } */
    pcm0[j] = v;
  }
#endif

  // Swap buffers
  cur_fft_buf ^= 1;
}

fftbands_t * fft_create_bands(int n, const fftband_limit_t * limits)
{
  return fftband_create(n, FFT_SIZE, FFT_FRQ, limits);
}

void fft_fill_bands(fftbands_t * bands)
{
  fftband_update(bands, fft_F[cur_fft_buf]);
}

void fft_fill_pcm(short * pcm, int n)
{
  short * pcm_src = pcm_F[cur_fft_buf];
  int start, end;
  int m, i, j, k, step;

  if (!pcm || n<=0) return;

  start = pcm_start;
  end   = pcm_end;
  m     = end - start;
  if (m<=0 || m > FFT_SIZE) {
    start = 0;
    m = FFT_SIZE;
  }
  step = (m << 12) / n;
  for (i=0, j=(start<<12); i<n; ++i, j = k) {
    int ij,ik,v;

    k = j + step;
    ij = j >> 12;
    ik = k >> 12;
    if (ij == ik) {
      v = pcm_src[ij];
    } else {
      v = (int)pcm_src[ij++] * (0x1000 - (j&0xFFF));
      while (ij<ik) {
	v += (int)pcm_src[ij++] << 12;
      }
      v += (int)pcm_src[ij] * (k&0xFFF);
      v /= step;
    }
    pcm[i] = v;
  }
}
