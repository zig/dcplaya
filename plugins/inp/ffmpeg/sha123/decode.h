/**
 *  @ingroup sha123
 *  @file    sha123/decode.h
 *  @author  benjamin gerard
 *  @date    2003/04/09
 *  @brief   mpeg decoder.
 */

#ifndef _SHA123_DECODE_H_
#define _SHA123_DECODE_H_

#include "sha123/types.h"

/** int sign bit position. */
#define SHA123_SIGN_BIT ((sizeof(int)<<3)-1)

/* $$$ ben :
 * Smart convertion but no clip counter. It should be easy to add.
 * Really helps CPU because there is no conditionnal jump in there.
 * $$$ ben : check float to int conversion. In most time libC used a
 * slow conversion routine that check overflow and NAN.
 */
#define SHA123_WRITE_SAMPLE(samples,sum,clip) {\
   int v = (int)(sum);\
   v += 32768;                            /* change sign */\
   v &= ~(v>>SHA123_SIGN_BIT);            /* Lower clip  */\
   v |= (65535 - v) >> SHA123_SIGN_BIT;   /* Upper clip  */\
   *(samples) = v ^ 0x8000;\
}

int sha123_synth_1to1_mono(const real * bandPtr,
			   unsigned char *samples,
			   int *pnt);

int sha123_synth_1to1(const real * bandPtr,
		      int channel,
		      unsigned char *out,
		      int *pnt);

int sha123_synth_2to1_mono(const real * bandPtr,
			   unsigned char *samples,
			   int *pnt);

int sha123_synth_2to1(const real * bandPtr, int channel,
		      unsigned char *out,
		      int *pnt);

int sha123_synth_4to1_mono(const real * bandPtr,
			   unsigned char *samples,
			   int *pnt);
int sha123_synth_4to1(const real * bandPtr, int channel,
		      unsigned char *out,
		      int *pnt);

#endif /* #define _SHA123_DECODE_H_ */
