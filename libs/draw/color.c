/**
 * @ingroup  dcplaya_draw
 * @file     color.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    draw color functions.
 *
 * $Id: color.c,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#include "draw/color.h"

static inline int clip255(int v)
{
  const int sign_bit = (sizeof(int) << 3) - 1;
  v &= ~(v >> sign_bit);
  v |= (255-v) >> sign_bit;
  v &= 255;
  return v;
}

/** Create a draw_argb_t from a draw_color_float_t. */
draw_argb_t draw_color_float_to_argb(const draw_color_t * col)
{
  return DRAW_ARGB(clip255(col->a * 255.0f),
				   clip255(col->r * 255.0f),
				   clip255(col->g * 255.0f),
				   clip255(col->b * 255.0f));
}

/** Create a draw_argb_t from 4 float componants. */
draw_argb_t draw_color_4float_to_argb(const float a, const float r,
									  const float g, const float b)
{
  return DRAW_ARGB(clip255(a * 255.0f),
				   clip255(r * 255.0f),
				   clip255(g * 255.0f),
				   clip255(b * 255.0f));
}

/** Create a draw_color_float_t from a draw_argb_t. */
void draw_color_argb_to_float(draw_color_t *col, const draw_argb_t argb)
{
  const float f = 1.0f / 255.0f;
  col->a = f * DRAW_ARGB_A(argb);
  col->r = f * DRAW_ARGB_R(argb);
  col->g = f * DRAW_ARGB_G(argb);
  col->b = f * DRAW_ARGB_B(argb);
}
