/**
 *  @file    draw_clipping.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/10/15
 *  @brief   draw primitive clipping.
 *
 * $Id: draw_clipping.h,v 1.1 2002-10-16 18:50:41 benjihan Exp $
 */

#ifndef _DRAW_CLIPPING_H_
#define _DRAW_CLIPPING_H_

#include "draw_vertex.h"

/** The clipping box [xmin ymin xmax ymax]. */
extern float clipbox[];

/** @name Clipping fucntions.
 *  @{
 */
/** Set clipping box. */
void draw_set_clipping(const float xmin, const float ymin,
					   const float xmax, const float ymax);
/** Get clipping box. */
void draw_get_clipping(float * xmin, float * ymin,
					   float * xmax, float * ymax);
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
