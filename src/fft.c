/**
 * @ingroup dcplaya_devel
 * @file    fft.c
 * @author  benjamin gerard <ben@sashipa.com>
 * 
 * @version $Id: fft.c,v 1.8 2002-12-30 06:28:18 ben Exp $
 */

#include <stdlib.h>
#include <arch/spinlock.h>
#include "sysdebug.h"
#include "fft.h"
#include "int_fft.h"
#include "fifo.h"
#include "math_int.h"


#define FFT_SIZE (1 << FFT_LOG_2)
#define FFT_FRQ  16000
#define PCM_SIZE (FFT_SIZE * 50000 / FFT_FRQ)


extern short int_decibel[4096];

static int pcm_buf[PCM_SIZE];        /* PCM read buffer */
static short pcm_F[2][FFT_SIZE];     /* PCM final       */
static short fft_F[2][FFT_SIZE/2+1]; /* FFT final       */
static short fft_R[FFT_SIZE];        /* FFT real        */
static short fft_I[FFT_SIZE];        /* FFT imaginary   */
static int cur_fft_buf;

static spinlock_t mutex;

int fft_init(void)
{
  cur_fft_buf = 0;
  spinlock_init(&mutex);
  return 0;
}

void fft_shutdown(void)
{
}

extern int playa_get_frq(void); /* playa.c */

void fft_queue(void)
{
  const int pcm_frq = playa_get_frq();
  int j;

  short * fft = fft_F[cur_fft_buf ^ 1];
  short * pcm0 = pcm_F[cur_fft_buf ^ 1];
  short * pcm1 = pcm_F[cur_fft_buf ^ 0];

  if (pcm_frq <= 0) {
    memset(pcm0, 0, sizeof(*pcm0) * FFT_SIZE);
    /* Safety net : flush bak-buffer */
    fifo_readbak(pcm_buf, PCM_SIZE);
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
  for (j = 0; j <= FFT_SIZE/2; ++j) {
    int rea, imm, v;
    rea = (int)fft_R[j];
    imm = (int)fft_I[j];
    v = int_sqrt((j+1) * (rea*rea + imm*imm));
    v  |= ((0x7FFF-v)>>31);
    v  &= 0x7FFF;
    fft[j] = v;
  }

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
  int i, j, step = (FFT_SIZE << 12) / n;
  for (i=j=0; i<n; ++i, j += step) {
    pcm[i] = pcm_src[j>>12];
  }
}
