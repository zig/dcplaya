/** @ingroup dcplaya_devel
 *  @file    vtx.c
 *  @author  ben(jamin) gerard <ben@sashipa.com>
 *  @date    2003/01/19
 *  @brief   Vertex functions.
 *
 * $Id: vtx.c,v 1.1 2003-01-20 14:19:43 ben Exp $
 */

#include <dc/fmath.h>
#include "vtx.h"

#ifndef EPSYLON
# define EPSYLON 0.00001
#endif

#define VTX_PI 3.14159265359f

vtx_t * vtx_neg(vtx_t *a)
{
  a->x = -a->x;
  a->y = -a->y;
  a->z = -a->z;
  return a;
}

vtx_t * vtx_neg2(vtx_t *r, const vtx_t *a)
{
  r->x = -a->x;
  r->y = -a->y;
  r->z = -a->z;
  r->w = 1;
  return r;
}

vtx_t * vtx_add(vtx_t *a, const vtx_t *b)
{
  a->x += b->x;
  a->y += b->y;
  a->z += b->z;
  return a;
}

vtx_t * vtx_add3(vtx_t * r, vtx_t *a, const vtx_t *b)
{
  r->x = a->x + b->x;
  r->y = a->y + b->y;
  r->z = a->z + b->z;
  r->w = 1;
  return r;
}

vtx_t * vtx_sub(vtx_t *a, const vtx_t *b)
{
  a->x -= b->x;
  a->y -= b->y;
  a->z -= b->z;
  return a;
}

vtx_t * vtx_sub3(vtx_t * r, vtx_t *a, const vtx_t *b)
{
  r->x = a->x - b->x;
  r->y = a->y - b->y;
  r->z = a->z - b->z;
  r->w = 1;
  return r;
}

vtx_t * vtx_mul(vtx_t *a, const vtx_t *b)
{
  a->x *= b->x;
  a->y *= b->y;
  a->z *= b->z;
/*   a->w *= b->w; */
  return a;
}

vtx_t * vtx_mul3(vtx_t * r, vtx_t *a, const vtx_t *b)
{
  r->x = a->x * b->x;
  r->y = a->y * b->y;
  r->z = a->z * b->z;
  r->w = 1; /* a->w * b->w; */
  return r;
}

vtx_t * vtx_scale(vtx_t *a, const float b)
{
  a->x *= b;
  a->y *= b;
  a->z *= b;
/*   a->w *= b; */
  return a;
}

vtx_t * vtx_scale3(vtx_t * r, const vtx_t *a, const float b)
{
  r->x = a->x * b;
  r->y = a->y * b;
  r->z = a->z * b;
  r->w = 1; /* a->w * b->w; */
  return r;
}

vtx_t * vtx_blend(vtx_t *a, const vtx_t *b, const float f)
{
  const float of = 1.0f - f;
  a->x = a->x * f + b->x * of;
  a->y = a->y * f + b->y * of;
  a->z = a->z * f + b->z * of;
  return a;
}

vtx_t * vtx_blend3(vtx_t *r, const vtx_t *a, const vtx_t *b, const float f)
{
  const float of = 1.0f - f;
  r->x = a->x * f + b->x * of;
  r->y = a->y * f + b->y * of;
  r->z = a->z * f + b->z * of;
  r->w = 1;
  return r;
}

float vtx_sqnorm(const vtx_t * a)
{
  return a->x * a->x + a->y * a->y + a->z * a->z;
}

float vtx_norm(const vtx_t * a)
{
  return fsqrt(vtx_sqnorm(a));
}

float vtx_inorm(const vtx_t * a)
{
  const float d = vtx_sqnorm(a);
  return (d > EPSYLON) ? frsqrt(d) : -1;
}

static vtx_t * max_to_1(vtx_t * a)
{
  if (a->x >= a->y) {
    if (a->x >= a->z) {
      a->x = 1.0f;
      a->y = a->z = 0.0f;
    } else {
      a->z = 1.0f;
      a->x = a->y = 0.0f;
    }
  } else {
    if (a->y >= a->z) {
      a->y = 1.0f;
      a->x = a->z = 0.0f;
    } else {
      a->z = 1.0f;
      a->x = a->y = 0.0f;
    }
  }
  return a;
}

vtx_t * vtx_normalize(vtx_t * a)
{
  const float d = vtx_inorm(a);
  return (d == -1) ? max_to_1(a) : vtx_scale(a, d);
}

vtx_t * vtx_normalize2(vtx_t * r, const vtx_t * a)
{
  const float d = vtx_inorm(a);
  if (d == -1) {
    *r = *a;
    return r;
  } else {
    return vtx_scale3(r, a, d);
  }
}

vtx_t * vtx_cross_product(vtx_t * a, const vtx_t * b)
{
  const float x=a->x, y=a->y, z=a->z;

  a->x = y * b->z - z * b->y;
  a->y = z * b->x - x * b->z;
  a->z = x * b->y - y * b->x;
  return a;
}

vtx_t * vtx_cross_product3(vtx_t * r, const vtx_t * a, const vtx_t * b)
{
  r->x = a->y * b->z - a->z * b->y;
  r->y = a->z * b->x - a->x * b->z;
  r->z = a->x * b->y - a->y * b->x;
  r->w = 1;
  return r;
}

float vtx_dot_product(const vtx_t * a, const vtx_t * b)
{
  return a->x * b->x + a->y * b->y + + a->z * b->z;
}

vtx_t * vtx_sin(vtx_t * a)
{
  a->x = fsin(a->x);
  a->y = fsin(a->y);
  a->z = fsin(a->z);
  return a;
}

vtx_t * vtx_sin2(vtx_t * r, const vtx_t * a)
{
  r->x = fsin(a->x);
  r->y = fsin(a->y);
  r->z = fsin(a->z);
  return r;
}

vtx_t * vtx_cos(vtx_t * a)
{
  a->x = fcos(a->x);
  a->y = fcos(a->y);
  a->z = fcos(a->z);
  return a;
}

vtx_t * vtx_cos2(vtx_t * r, const vtx_t * a)
{
  r->x = fcos(a->x);
  r->y = fcos(a->y);
  r->z = fcos(a->z);
  return r;
}

static float inc_angle(float a, const float b)
{
  a += b;
  while (a >= 2.0f*VTX_PI) {
    a -= 2.0f*VTX_PI;
  }
  while (a < 0) {
    a += 2.0f*VTX_PI;
  }
  return a;
}

vtx_t * vtx_inc_angle(vtx_t * a, const vtx_t * b)
{
  a->x = inc_angle(a->x, b->x);
  a->y = inc_angle(a->y, b->y);
  a->z = inc_angle(a->z, b->z);
  return a;
}

float vtx_sqdist(const vtx_t * a, const vtx_t * b)
{
  const float dx = a->x - b->x;
  const float dy = a->y - b->y;
  const float dz = a->z - b->z;

  return dx*dx + dy*dy + dz*dz;
}

float vtx_dist(const vtx_t * a, const vtx_t * b)
{
  return fsqrt(vtx_sqdist(a,b));
}
