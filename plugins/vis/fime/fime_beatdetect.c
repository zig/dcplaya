/** @ingroup dcplaya_vis_driver
 *  @file    fime_beatdetect.c
 *  @author  benjamin gerard 
 *  @date    2003/01/19
 *  @brief   FIME : beat detection
 *  $Id: fime_beatdetect.c,v 1.3 2003-03-10 22:55:34 ben Exp $
 */ 

#include <stdlib.h>

#include "dcplaya/config.h"
#include "fime_beatdetect.h"
#include "fime_analysis.h"
#include "fft.h"
#include "int_fft.h"
#include "sysdebug.h"

#include "draw/primitives.h"

#define NO_BEATDETECT

#define FFT_SIZE (1 << FFT_LOG_2)

#ifndef NO_BEATDETECT

static short * fime_fft_buffer;
static int * fime_fft_F; /* FFT final */
static short * fime_fft_S; /* FFT scroll      */
static short * fime_fft_R; /* FFT real        */
static short * fime_fft_I; /* FFT imaginary   */
unsigned int fime_fft_idx;

#endif

int fime_beatdetect_init(void)
{
  int err = 0;
  SDDEBUG("[fime] : beatdetect init := [%s]\n", !err ? "OK" : "FAILED");
#ifndef NO_BEATDETECT

  if (!fime_fft_buffer) {
    int size = FFT_SIZE * (2+1+1) * sizeof(short) +
      (FFT_SIZE/2+1) * sizeof(int);
    fime_fft_buffer = malloc(size);
  }
  if (fime_fft_buffer) {
    fime_fft_S = fime_fft_buffer;
    fime_fft_R = fime_fft_S + FFT_SIZE*2;
    fime_fft_I = fime_fft_R + FFT_SIZE;
    fime_fft_F = (int *)(fime_fft_I + FFT_SIZE);
  } else {
    fime_fft_S = fime_fft_R = fime_fft_I = 0;
    fime_fft_F = 0;
  }
  fime_fft_idx = 0;
  err = !fime_fft_buffer;
#endif
  return -(!!err);
}

void fime_beatdetect_shutdown(void)
{
  SDDEBUG("[fime] : beatdetect shutdowned\n");
}

float fime_beatdetect_update(void)
{
#ifdef NO_BEATDETECT
  return 0;
#else

  int i;
  short * I, * R, * S, w;
  int *F;

  const fime_analyser_t * analyser;
  if (!fime_fft_buffer || (analyser = fime_analysis_get(1), !analyser)) {
    return 0;
  }

  S = fime_fft_S + fime_fft_idx;
  R = fime_fft_R;
  I = fime_fft_I;
  F = fime_fft_F;

  // Scroll buffer
  w = (short)(32767 * analyser->w);
  i = fime_fft_idx + FFT_SIZE - 1;
  fime_fft_S[i] =
    fime_fft_S[(i + FFT_SIZE) & (FFT_SIZE*2-1)] = w;
  fime_fft_idx = (fime_fft_idx+1) & (FFT_SIZE-1);

  // Copt scrol to Real buffer
  memcpy(R, S, FFT_SIZE * sizeof(*R));
  // Apply Hanning window
  fix_window(R, FFT_SIZE);

  // Clear imaginary. 
  memset(I, 0, FFT_SIZE * sizeof(*I));

  // Forward FFT : Time -> Frq
  fix_fft(R, I, FFT_LOG_2, 0);

  // Copy to final buffer.
  F[0] = (R[0] >> 1) * (R[0] >> 1);
  F[FFT_SIZE/2] >>= (R[FFT_SIZE/2] >> 1) * (R[FFT_SIZE/2] >> 1);
  
  for (i = 1; i < FFT_SIZE/2; ++i) {
    int rea, imm, v;
    rea = (int)R[i];
    imm = (int)I[i];
    v = rea*rea + imm*imm;
/*     v  |= ((0x7FFF-v)>>31); */
/*     v  &= 0x7FFF; */
    F[i] = v;
  }

  return 0;
#endif
}

extern short int_decibel[];

int fime_beatdetect_render(void)
{
#ifdef NO_BEATDETECT
  return 0;
#else
  int i;
  float x, w = 640.0f/(FFT_SIZE/2);
  float sy = 300 / 32768.0f;
  int flags = DRAW_NO_FILTER|DRAW_OPAQUE;
  draw_vertex_t vtx[4];

  //  const float f0 = 60.0f / (float)FFT_SIZE;
  unsigned int max = 0, imax=0;

  for (i=0;i<4;++i) {
    vtx[i].z = 300;
    vtx[i].w = 1;
    vtx[i].a = 1.0f;
    vtx[i].r = 1.0f;
    vtx[i].g = 1.0f;
    vtx[i].b = 1.0f;
  }


  for (i=0, x=0; i<FFT_SIZE/2; ++i, x+=w) {
    unsigned int v = int_sqrt(fime_fft_F[i]);
    float h;

    if (v > max) {
      max = v;
      imax = i;
    }



    if (v > 32767) v = 32767;
    v = int_decibel[v>>3];
    h = (float)v * sy;

    vtx[0].x = x;
    vtx[1].x = x+w;
    vtx[2].x = x;
    vtx[3].x = x+w;

    vtx[0].y = 280+0;
    vtx[1].y = 280+0;
    vtx[2].y = 280+h;
    vtx[3].y = 280+h;

    draw_strip(vtx, 4, flags);    
  }
/*   printf("[%d %.02f] ", imax, imax * f0 * 60.0f); */

  return 0;
#endif
}
