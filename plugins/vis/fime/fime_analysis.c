/** @ingroup dcplaya_vis_driver
 *  @file    fime_analysis.c
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME. Frquancy and beat analysis.
 *  $Id: fime_analysis.c,v 1.2 2003-01-19 21:36:33 ben Exp $
 */ 

#include <stdlib.h>

#include "fime_analysis.h"
#include "fft.h"

extern short int_decibel[];

static fftbands_t * bands;

static fime_analyser_t b[2];

int fime_analysis_init(void)
{
  if (!bands) {
    static fftband_limit_t limits[] = {
      {0, 250},
      {1000, 3500},
    };
    bands = fft_create_bands(2,limits);
  }
  memset(b,0,sizeof(b));
  return bands ? 0 : -1;
}


void fime_analysis_shutdown(void)
{
  if (bands) {
    free(bands);
    bands = 0;
  }
}

static float max(const float a, const float b) {
  return a > b ? a : b;
}
static float min(const float a, const float b) {
  return a < b ? a : b;
}
static float bound(const float a, const float b, const float c) {
  return a < b ? b : (a > c ? c : a);
}

int fime_analysis_update(void)
{
  int j,r;

  if (bands) {
    float q;

    fft_fill_bands(bands);
   
    for (j=0; j<2; ++j) {
      const float avgf  = 0.988514020;
      const float avgf2 = 0.93303299;
      const float v = int_decibel[bands->band[j].v>>3] * (1.0/32768.0);
      b[j].v = b[j].v * 0.7 + v * 0.3;

      b[j].avg = b[j].avg * avgf + b[j].v * (1.0-avgf);
      b[j].avg2 = b[j].avg2 * avgf2 + b[j].v * (1.0-avgf2);
      b[j].max = max(b[j].max,0.1);
      b[j].max = max(b[j].max,b[j].avg);

      b[j].ts = bound(b[j].ts, 0.1, 0.98);
      b[j].thre = (b[j].max * b[j].ts) + (b[j].avg * (1.0-b[j].ts));

      if (b[j].v > b[j].thre) {
	if (b[j].tacu <= 0) {
	  b[j].tacu = 1;
	  b[j].max2 = b[j].v;
	} else {
	  ++b[j].tacu;
	  if (b[j].tacu > 10) {
	    if (b[j].max2 > b[j].max) {
	      b[j].max = b[j].max2;
	    } else {
	      b[j].ts *= 1.01;
	    }
	  }
	  b[j].max2 = max(b[j].max2,b[j].v);
	}
      } else {
	if (b[j].tacu > 0) {
	  b[j].max = b[j].max2;
	  b[j].tacu = 0;
	  b[j].max2 = b[j].v;
	}
	b[j].max2 = max(b[j].max2,b[j].v);

	--b[j].tacu;
	if (b[j].tacu < -10) {
	  b[j].max = b[j].max * avgf + b[j].max2 * (1.0f-avgf);
	  b[j].ts *= 0.99f;
	}
      }
      q = 0;
      if (b[j].tacu == 3) {
	q = b[j].max2;
      }
      b[j].w = q;
    }
  } else {
    for (j=0; j<2; ++j) {
      b[j].w = 0;
    }
  }

  r = (b[0].w>0) | ((b[1].w>0)<<1);
  if (r) {
    r |= 0x100 
      & -(b[0].tacu>=3 && b[0].tacu<20)
      & -(b[1].tacu>=3 && b[1].tacu<20);
  }

  return r;
}

const fime_analyser_t * fime_analysis_get(int i)
{
  if ((unsigned int) i >= 2) {
    return 0;
  }
  return b+i;
}
