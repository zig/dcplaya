/**
 *  @ingroup dcplaya_draw
 *  @file    clipping.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/10/15
 *  @brief   draw clipping primitives.
 *
 * $Id: clipping.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _DRAW_CLIPPING_H_
#define _DRAW_CLIPPING_H_

#include "draw/vertex.h"

/** Clipping box structure.
 *  @ingroup dcplaya_draw
 */
typedef struct
{
  float x1; /**< Xmin. */
  float y1; /**< Ymin. */
  float x2; /**< Xmax. */
  float y2; /**< Ymax. */
} draw_clipbox_t;

//extern float clipbox[];

/** @name Clipping primitives.
 *  @ingroup dcplaya_draw
 *  @{
 */

/** Set clipping box.
 */
void draw_set_clipping4(const float xmin, const float ymin,
						const float xmax, const float ymax);

/** Set clipping box.
 */
void draw_set_clipping(const draw_clipbox_t * clipbox);

/** Get clipping box.
 */
void draw_get_clipping4(float * xmin, float * ymin,
						float * xmax, float * ymax);

/** Get clipping box.
 */
void draw_get_clipping(draw_clipbox_t * clipbox);

/**@}*/

/** Enter triangle clipping pipe.
 *
 *    Enter clipping pipe at a given stage defined by bit value.
 *    Stages are executed in this cascading order:
 *    - bit=0 : left clipping
 *    - bit=2 : right clipping
 *    - bit=1 : top clipping
 *    - bit=3 : bottom clipping
 *
 *    When running any stage, the triangle MUST be in a clipping case.
 *    When running left/top stage, both left/top and right/bottom clipping
 *    flags must be valid.
 *    When running right/top stage, only right/top clipping flags need to be
 *    valid.
 *    Top and bottom clipping flags will be computed after left/right stage
 *    because clipping could make Y coordinate change.
 */
void draw_triangle_clip_any(const draw_vertex_t *v1,
							const draw_vertex_t *v2,
							const draw_vertex_t *v3,
							int flags, const int bit);

#endif /* #define _DRAW_CLIPPING_H_ */
