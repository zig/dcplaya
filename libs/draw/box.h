/**
 * @ingroup dcplaya_draw
 * @file    box.h
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/11/22
 * @brief   Draw box primitives.
 *
 * $Id: box.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _DRAW_BOX_H_
#define _DRAW_BOX_H_

/** Draw uniform box in translucent mode. */
void draw_box1(float x1, float y1, float x2, float y2, float z,
			   float a, float r, float g, float b);

/** Draw horizontal gradiant box. */
void draw_box2h(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2);

/** Draw vertical gradiant box. */
void draw_box2v(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2);

/** Draw diagonal gradiant box. */
void draw_box2d(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2);

/** Draw general gradiant box. */
void draw_box4(float x1, float y1, float x2, float y2, float z,
			   float a1, float r1, float g1, float b1,
			   float a2, float r2, float g2, float b2,
			   float a3, float r3, float g3, float b3,
			   float a4, float r4, float g4, float b4);

#endif /* #define _DRAW_BOX_H_ */
