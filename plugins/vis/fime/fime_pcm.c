/** @ingroup dcplaya_vis_driver
 *  @file    pcm.c
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME : pcm
 *  $Id: fime_pcm.c,v 1.1 2003-01-18 14:22:17 ben Exp $
 */ 

#include <stdlib.h>

#include "fime_pcm.h"
#include "fft.h"


/* Normalized sample buffer. */
static float * pcm;

int fime_pcm_init(void)
{
  if (!pcm) {
    pcm = malloc(sizeof(*pcm) * FIME_PCM_SIZE);
    if (pcm) {
      memset(pcm, 0, sizeof(*pcm) * FIME_PCM_SIZE);
    }
  }
  return pcm ? 0 : -1;
}

void fime_pcm_shutdown(void)
{
  if (pcm) {
    free(pcm);
    pcm = 0;
  }
}


float * fime_pcm_update(void)
{
  int i;
  short spl2[FIME_PCM_SIZE];
  const float scale = 1.0f/32768.0f;
  const float s0 = 0.75;
  const float s1 = 1.0 - s0;

  if (pcm) {
    fft_fill_pcm(spl2,FIME_PCM_SIZE);
    for (i=0; i<FIME_PCM_SIZE; ++i) {
      const float a = (float)spl2[i] * scale;
      pcm[i] = (pcm[i] * s0) + (a * s1);
    }
  }

  return pcm;
}

float * fime_pcm_get(void)
{
  return pcm;
}
