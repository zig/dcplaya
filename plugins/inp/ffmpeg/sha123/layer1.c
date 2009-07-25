/* 
 * Mpeg Layer-1 audio decoder 
 * --------------------------
 * copyright (c) 1995 by Michael Hipp, All rights reserved. See also 'README'
 * near unoptimzed ...
 *
 * may have a few bugs after last optimization ... 
 *
 */

/* benjamin gerard 2003/04/14 
 *
 * Add some optimizations here :
 * - Separated mono and stereo processing to avoid test in loop.
 * - Removed intermediate buffer in step 2.
 */

#include "sha123/sha123.h"
#include "sha123/tables.h"

static void I_step_one_mn(unsigned int balloc[],
			  unsigned int * sca,
			  bsi_t * bsi)
{
  int i;
  unsigned int * ba;

  for (ba = balloc, i=0; i < SBLIMIT; i++) {
    *ba++ = bsi_getbits_fast(bsi, 4);
  }
  for (ba = balloc, i=0; i < SBLIMIT; i++) {
    if ((*ba++)) {
      *sca++ = bsi_getbits_fast(bsi, 6);
    }
  }
}

static void I_step_one_st(unsigned int balloc[],
			  unsigned int * sca,
			  bsi_t * bsi,
			  const int jsbound)
{
  int i;
  unsigned int * ba;

  for (ba=balloc, i=0; i<jsbound; i++) {
    int bits = bsi_getbits_fast(bsi, 8);
    *ba++ = bits >> 4;
    *ba++ = bits & 0xF;
  }
  for (; i<SBLIMIT; i++) {
    *ba++ = bsi_getbits(bsi, 4);
  }

  for (ba = balloc, i=0; i<jsbound; i++) {
    if ((*ba++)) {
      *sca++ = bsi_getbits_fast(bsi, 6);
    }
    if ((*ba++)) {
      *sca++ = bsi_getbits_fast(bsi, 6);
    }
  }
  for (; i < SBLIMIT; i++) {
    if ((*ba++)) {
      int bits = bsi_getbits(bsi, 12);
      *sca++ = bits >> 6;
      *sca++ = bits & 0x3F;
    }
  }
}

static void I_step_two_mn(real * f0,
			  unsigned int * ba,
			  unsigned int * sca,
			  bsi_t * bsi)
{
  int i;

  for (i=0; i<SBLIMIT; i++) {
    int n;
    real r;

    n = *ba++;
    if (n) {
      int sample = bsi_getbits(bsi, n + 1);
      r = (real) (((-1) << n) + sample + 1) * sha123_muls[n + 1][*sca++];
    } else {
      r = 0;
    }
    *f0++ = r;
  }
}

static void I_step_two_st(real * f0,
			  const unsigned int * ba,
			  const unsigned int * sca,
			  bsi_t * bsi,
			  const int jsbound)
{
  int i;

  for (i = 0; i<jsbound; i++) {
    int n;
    real r;

    n = *ba++;
    if (n) {
      int sample = bsi_getbits(bsi, n + 1);
      r = (real) (((-1) << n) + sample + 1) * sha123_muls[n + 1][*sca++];
    } else {
      r = 0;
    }
    f0[i] = r;
      
    n = *ba++;
    if (n) {
      int sample = bsi_getbits(bsi, n + 1);
      r = (real) (((-1) << n) + sample + 1) * sha123_muls[n + 1][*sca++];
    } else {
      r = 0;
    }
    f0[i+SBLIMIT] = r;
  }

  for (; i<SBLIMIT; i++) {
    int n;
    real r0,r1;

    n = *ba++;
    if (n) {
      int sample = bsi_getbits(bsi, n + 1);
      real samp = (((-1) << n) + sample + 1);
      r0 = samp * sha123_muls[n + 1][*sca++];
      r1 = samp * sha123_muls[n + 1][*sca++];
    } else {
      r0 = r1 = 0;
    }
    f0[i] = r0;
    f0[i+SBLIMIT] = r1;
  }
}

int sha123_do_layer1(sha123_t * sha123)
{
  sha123_frame_t * fr = &sha123->frame;
  unsigned int balloc[SBLIMIT*2];
  unsigned int scale_index[2*SBLIMIT];
  real fraction[2][SBLIMIT];

  const int jsbound = 
    (fr->header.mode == MPG_MD_JOINT_STEREO)
    ? (fr->header.mode_ext << 2) + 4
    : 32;

  /* $$$ ben : may be not need anymore */
  fr->jsbound = jsbound;

  if (!fr->info.log2chan) {
    /* Process mono */
    int i;
    I_step_one_mn(balloc, scale_index, &sha123->bsi);
    for (i = 0; i < SCALE_BLOCK; i++) {
      I_step_two_mn(fraction[0], balloc, scale_index, &sha123->bsi);
      fr->synth_mn(fraction[0], sha123->pcm_sample, &sha123->pcm_point);
    }
  } else {
    /* Process stereo */
    int i;
    I_step_one_st(balloc, scale_index, &sha123->bsi, jsbound);
    for (i = 0; i < SCALE_BLOCK; i++) {
      int p1 = sha123->pcm_point;
      I_step_two_st(fraction[0], balloc, scale_index, &sha123->bsi, jsbound);
      fr->synth_st(fraction[0], 0, sha123->pcm_sample, &p1);
      fr->synth_st(fraction[1], 1, sha123->pcm_sample,
		&sha123->pcm_point);
    }
  }
  return 0;
}
