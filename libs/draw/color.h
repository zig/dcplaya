/**
 * @ingroup  dcplaya_draw
 * @file     draw_color.h
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    draw color definitions.
 *
 * $Id: color.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _DRAW_COLOR_H_
#define _DRAW_COLOR_H_

/** Draw color float structure. */
typedef struct {
  float a; /** Alpha componant [0..1] */
  float r; /** Red  componant [0..1]  */
  float g; /** Green componant [0..1] */
  float b; /** Blue componant [0..1]  */
} draw_color_t;

/** Draw color ARGB packed type. */
typedef unsigned int draw_argb_t;

/** Create a draw_argb_t from a draw_color_float_t. */
draw_argb_t draw_color_float_to_argb(const draw_color_t * col);

/** Create a draw_argb_t from 4 float componants. */
draw_argb_t draw_color_4float_to_argb(const float a, const float r,
									  const float g, const float b);


/** Create a draw_color_float_t from a draw_argb_t. */
void draw_color_argb_to_float(draw_color_t * col, const draw_argb_t argb);

/** Create an ARGB packed color from integer componants [0..255] without
 *  clipping.
 */
#define DRAW_ARGB(A,R,G,B) \
 (((int)(A)<<24)|\
 ((int)(R)<<16)|\
 ((int)(G)<<8)|\
 (int)(B))

/** Get alpha componant of an ARGB packed color. */
#define DRAW_ARGB_A(ARGB) ((draw_argb_t)(ARGB)>>24)

/** Get red componant of an ARGB packed color. */
#define DRAW_ARGB_R(ARGB) (((ARGB)>>16)&255)

/** Get green componant of an ARGB packed color. */
#define DRAW_ARGB_G(ARGB) (((ARGB)>>8)&255)

/** Get blue componant of an ARGB packed color. */
#define DRAW_ARGB_B(ARGB) ((ARGB)&255)

/** Create an ARGB packed color from floating-point componants [0..1] without
 *  clipping.
 */
#define DRAW_ARGBF(A,R,G,B) \
 DRAW_COLOR_ARGB(A*255.0f,R*255.0f,G*255.0f,B*255.0f)

#endif /* #define _DRAW_COLOR_H_ */
