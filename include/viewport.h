/**
 *
 * 2002/02/21
 *
 * $Id: viewport.h,v 1.4 2002-09-06 23:16:09 ben Exp $
 */

#ifndef _VIEWPORT_H_
#define _VIEWPORT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


/** 3D viewport */
typedef struct
{
  int x; /**< Viewport position X */
  int y; /**< Viewport position Y */
  int w; /**< Viewport width      */
  int h; /**< Viewport height     */

  float tx; /**< Translation XM   */
  float ty; /**< Translation YM   */
  float mx; /**< Multiplier X ( scale * w * 0.5 ) */
  float my; /**< Multiplier Y ( scale * h * 0.5 ) */
} viewport_t;

void viewport_set(viewport_t *v,
		  int posX, int posY,
		  int width, int height,
		  const float scale);

void viewport_apply(viewport_t *v,
		    float *d, int dbytes,
		    const float *s, int sbytes,
		    int nb);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _VIEWPORT_H_ */

