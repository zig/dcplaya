/**
 * @ingroup dcplaya_draw_viewport
 * @file    draw/viewport.h
 * @author  benjamin gerard
 * @date    2002/02/21
 * @brief   viewport definition.
 *
 * $Id: viewport.h,v 1.2 2003-03-23 23:54:54 ben Exp $
 */

#ifndef _VIEWPORT_H_
#define _VIEWPORT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup  dcplaya_draw_viewport Viewport
 *  @ingroup   dcplaya_draw
 *  @brief     viewport
 *  @author    benjamin gerard
 *  @{
 */

/** 3D viewport.
 */
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

/** Set viewport.
 *
 *  @param  v       Viewport to set.
 *  @param  posX    X position of Top/left corner of viewport window.
 *  @param  posY    Y position of Top/left corner of viewport window.
 *  @param  width   Width of viewport window.
 *  @param  height  Height of viewport window.
 *  @param  scale   Viewport global scaling factor.
 */
void viewport_set(viewport_t *v,
		  int posX, int posY,
		  int width, int height,
		  const float scale);

/** Apply viewport transformation.
 *
 *  @param  v       Viewport to transform vertrices from.
 *  @param  d       Destination vertrices.
 *                  Pointer to the X coordinate of first transformaed vertex.
 *  @param  dbytes  Size of transformed vertex structure (in bytes).
 *  @param  s       Source vertrices.
 *                  Pointer to the X coordinate of first vertex to transform.
 *  @param  sbytes  Size of source vertex structure (in bytes).
 *  @param  nb      Number of vertex to transform.
 */
void viewport_apply(viewport_t *v,
		    float *d, int dbytes,
		    const float *s, int sbytes,
		    int nb);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _VIEWPORT_H_ */
