/**
 * @ingroup dcplaya_draw
 * @file    primitives.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/10/10
 * @brief   2D drawing primitives.
 *
 * $Id: primitives.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _DRAW_PRIMITIVES_H_
#define _DRAW_PRIMITIVES_H_

#include "draw/vertex.h"

/** @name Draw primtives.
 *  @ingroup dcplaya_draw
 *  @{
 */

/** Draw line.*/
void draw_line(float x1, float y1, float z1, float x2, float y2, float z2,
			   float a1, float r1, float g1, float b1,
			   float a2, float r2, float g2, float b2,
			   float w);

/** Draw triangle without clipping tests. */
void draw_triangle_no_clip(const draw_vertex_t *v1,
						   const draw_vertex_t *v2,
						   const draw_vertex_t *v3,
						   int flags);

/** Draw triangle. */
void draw_triangle(const draw_vertex_t *v1,
				   const draw_vertex_t *v2,
				   const draw_vertex_t *v3,
				   int flags);

/** Draw strip. */
void draw_strip(const draw_vertex_t *v, int n, int flags);

/**@}*/

#endif /* #define _DRAW_PRIMITIVES_H_ */
