/**
 * @ingroup  dcplaya_draw
 * @file     color.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    draw color functions.
 *
 * $Id: color.c,v 1.2 2002-11-27 09:58:09 ben Exp $
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

void draw_color_clip(draw_color_t *d)
{
  if (d->a < 0) d->a = 0;
  else if (d->a > 1) d->a = 1;
  if (d->r< 0) d->r= 0;
  else if (d->r> 1) d->r= 1;
  if (d->g< 0) d->g= 0;
  else if (d->g> 1) d->g= 1;
  if (d->b< 0) d->b= 0;
  else if (d->b> 1) d->b= 1;
}

void draw_color_clip3(draw_color_t *d, const float min, const float max)
{
  if (d->a < min) d->a = min;
  else if (d->a > max) d->a = max;
  if (d->r< min) d->r= min;
  else if (d->r> max) d->r= max;
  if (d->g< min) d->g= min;
  else if (d->g> max) d->g= max;
  if (d->b< min) d->b= min;
  else if (d->b> max) d->b= max;
}

void draw_color_neg(draw_color_t *d)
{
  d->a = -d->a;
  d->r = -d->r;
  d->g = -d->g;
  d->b = -d->b;
}

void draw_color_add(draw_color_t *d,
					const draw_color_t *a, const draw_color_t *b)
{
  d->a = a->a + b->a;
  d->r = a->r + b->r;
  d->g = a->g + b->g;
  d->b = a->b + b->b;
}

void draw_color_add_clip(draw_color_t *d,
						 const draw_color_t *a, const draw_color_t *b)
{
  d->a = a->a + b->a;
  if (d->a < 0) d->a = 0;
  else if (d->a > 1) d->a = 1;
  
  d->r = a->r + b->r;
  if (d->r< 0) d->r= 0;
  else if (d->r> 1) d->r= 1;
  
  d->g = a->g + b->g;
  if (d->g< 0) d->g= 0;
  else if (d->g> 1) d->g= 1;
  
  d->b = a->b + b->b;
  if (d->b< 0) d->b= 0;
  else if (d->b> 1) d->b= 1;
}

void draw_color_mul_clip(draw_color_t *d,
						 const draw_color_t *a, const draw_color_t *b)
{
  d->a = a->a * b->a;
  if (d->a < 0) d->a = 0;
  else if (d->a > 1) d->a = 1;

  d->r = a->r * b->r;
  if (d->r< 0) d->r= 0;
  else if (d->r> 1) d->r= 1;

  d->g = a->g * b->g;
  if (d->g< 0) d->g= 0;
  else if (d->g> 1) d->g= 1;

  d->b = a->b * b->b;
  if (d->b< 0) d->b= 0;
  else if (d->b> 1) d->b= 1;
}

void draw_color_mul(draw_color_t *d,
					const draw_color_t *a, const draw_color_t *b)
{
  d->a = a->a * b->a;
  d->r = a->r * b->r;
  d->g = a->g * b->g;
  d->b = a->b * b->b;
}

