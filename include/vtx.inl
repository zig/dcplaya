/** @ingroup dcplaya_devel
 *  @file    vtx.inl
 *  @author  ben(jamin) gerard <ben@sashipa.com>
 *  @date    2003/01/19
 *  @brief   Vertex functions definitions (inlined or not).
 *
 *  @wraning Do NOT include this file directly. Use "vtx.h" indeed.
 *
 * $Id: vtx.inl,v 1.3 2003-01-24 10:48:40 ben Exp $
 */

#ifndef _VTX_INL_
#define _VTX_INL_

#include "math_float.h"

VTX_FUNCTION
vtx_t * vtx_identity(vtx_t *a)
{
  a->x = a->y = a->z = 0;
  a->w = 1;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_set(vtx_t *a, const float x, const float y, const float z)
{
  a->x = x;
  a->y = y;
  a->z = z;
  a->w = 1;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_neg(vtx_t *a)
{
  a->x = -a->x;
  a->y = -a->y;
  a->z = -a->z;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_neg2(vtx_t *r, const vtx_t *a)
{
  r->x = -a->x;
  r->y = -a->y;
  r->z = -a->z;
  r->w = 1;
  return r;
}

VTX_FUNCTION
vtx_t * vtx_add(vtx_t *a, const vtx_t *b)
{
  a->x += b->x;
  a->y += b->y;
  a->z += b->z;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_add3(vtx_t * r, const vtx_t *a, const vtx_t *b)
{
  r->x = a->x + b->x;
  r->y = a->y + b->y;
  r->z = a->z + b->z;
  r->w = 1;
  return r;
}

VTX_FUNCTION
vtx_t * vtx_sub(vtx_t *a, const vtx_t *b)
{
  a->x -= b->x;
  a->y -= b->y;
  a->z -= b->z;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_sub3(vtx_t * r, const vtx_t *a, const vtx_t *b)
{
  r->x = a->x - b->x;
  r->y = a->y - b->y;
  r->z = a->z - b->z;
  r->w = 1;
  return r;
}

VTX_FUNCTION
vtx_t * vtx_mul(vtx_t *a, const vtx_t *b)
{
  a->x *= b->x;
  a->y *= b->y;
  a->z *= b->z;
/*   a->w *= b->w; */
  return a;
}

VTX_FUNCTION
vtx_t * vtx_mul3(vtx_t * r, vtx_t *a, const vtx_t *b)
{
  r->x = a->x * b->x;
  r->y = a->y * b->y;
  r->z = a->z * b->z;
  r->w = 1; /* a->w * b->w; */
  return r;
}

VTX_FUNCTION
vtx_t * vtx_scale(vtx_t *a, const float b)
{
  a->x *= b;
  a->y *= b;
  a->z *= b;
/*   a->w *= b; */
  return a;
}

VTX_FUNCTION
vtx_t * vtx_scale3(vtx_t * r, const vtx_t *a, const float b)
{
  r->x = a->x * b;
  r->y = a->y * b;
  r->z = a->z * b;
  r->w = 1; /* a->w * b->w; */
  return r;
}

VTX_FUNCTION
vtx_t * vtx_blend(vtx_t *a, const vtx_t *b, const float f)
{
  const float of = 1.0f - f;
  a->x = a->x * f + b->x * of;
  a->y = a->y * f + b->y * of;
  a->z = a->z * f + b->z * of;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_blend3(vtx_t *r, const vtx_t *a, const vtx_t *b, const float f)
{
  const float of = 1.0f - f;
  r->x = a->x * f + b->x * of;
  r->y = a->y * f + b->y * of;
  r->z = a->z * f + b->z * of;
  r->w = 1;
  return r;
}

VTX_FUNCTION
float vtx_sqnorm(const vtx_t * a)
{
  return a->x * a->x + a->y * a->y + a->z * a->z;
}

VTX_FUNCTION
float vtx_norm(const vtx_t * a)
{
  return Sqrt(vtx_sqnorm(a));
}

VTX_FUNCTION
float vtx_inorm(const vtx_t * a)
{
  const float d = vtx_sqnorm(a);
  return (d > MF_EPSYLON2) ? ISqrt(d) : -1;
}

static vtx_t * __vtx_max_to_1(vtx_t * a)
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


VTX_FUNCTION
vtx_t * vtx_normalize(vtx_t * a)
{
  const float d = vtx_inorm(a);
  return (d == -1) ? __vtx_max_to_1(a) : vtx_scale(a, d);
}

VTX_FUNCTION
vtx_t * vtx_normalize2(vtx_t * r, const vtx_t * a)
{
  const float d = vtx_inorm(a);
  if (d == -1) {
    *r = *a;
    __vtx_max_to_1(r);
    return r;
  } else {
    return vtx_scale3(r, a, d);
  }
}

VTX_FUNCTION
vtx_t * vtx_cross_product(vtx_t * a, const vtx_t * b)
{
  const float x=a->x, y=a->y, z=a->z;

  a->x = y * b->z - z * b->y;
  a->y = z * b->x - x * b->z;
  a->z = x * b->y - y * b->x;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_cross_product3(vtx_t * r, const vtx_t * a, const vtx_t * b)
{
  r->x = a->y * b->z - a->z * b->y;
  r->y = a->z * b->x - a->x * b->z;
  r->z = a->x * b->y - a->y * b->x;
  r->w = 1;
  return r;
}

VTX_FUNCTION
float vtx_dot_product(const vtx_t * a, const vtx_t * b)
{
  return a->x * b->x + a->y * b->y + a->z * b->z;
}

VTX_FUNCTION
vtx_t * vtx_sin(vtx_t * a)
{
  a->x = Sin(a->x);
  a->y = Sin(a->y);
  a->z = Sin(a->z);
  return a;
}

VTX_FUNCTION
vtx_t * vtx_sin2(vtx_t * r, const vtx_t * a)
{
  r->x = Sin(a->x);
  r->y = Sin(a->y);
  r->z = Sin(a->z);
  r->w = 1;
  return r;
}

VTX_FUNCTION
vtx_t * vtx_cos(vtx_t * a)
{
  a->x = Cos(a->x);
  a->y = Cos(a->y);
  a->z = Cos(a->z);
  return a;
}

VTX_FUNCTION
vtx_t * vtx_cos2(vtx_t * r, const vtx_t * a)
{
  r->x = Cos(a->x);
  r->y = Cos(a->y);
  r->z = Cos(a->z);
  r->w = 1;
  return r;
}

static float __vtx_inc_angle(float a, const float b)
{
  a += b;
  while (a >= 2.0f*MF_PI) {
    a -= 2.0f*MF_PI;
  }
  while (a < 0) {
    a += 2.0f*MF_PI;
  }
  return a;
}

VTX_FUNCTION
vtx_t * vtx_inc_angle(vtx_t * a, const vtx_t * b)
{
  a->x = __vtx_inc_angle(a->x, b->x);
  a->y = __vtx_inc_angle(a->y, b->y);
  a->z = __vtx_inc_angle(a->z, b->z);
  return a;
}

VTX_FUNCTION
float vtx_sqdist(const vtx_t * a, const vtx_t * b)
{
  const float dx = a->x - b->x;
  const float dy = a->y - b->y;
  const float dz = a->z - b->z;

  return dx*dx + dy*dy + dz*dz;
}

VTX_FUNCTION
float vtx_dist(const vtx_t * a, const vtx_t * b)
{
  return Sqrt(vtx_sqdist(a,b));
}

VTX_FUNCTION
vtx_t * vtx_apply(vtx_t * a)
{
  const float iw = Inv(a->w);
  a->x *= iw;
  a->y *= iw;
  a->z *= iw;
  return a;
}

VTX_FUNCTION
vtx_t * vtx_apply2(vtx_t * r, const vtx_t * a)
{
  const float iw = Inv(a->w);
  r->x = a->x * iw;
  r->y = a->y * iw;
  r->z = a->z * iw;
  r->w = 1;
  return r;
}

VTX_FUNCTION
int vtx_clip_flags(const vtx_t *a)
{
  const float W = a->w;
  return
    (Fsign(a->x + W) << 0) |
    (Fsign(W - a->x) << 1) |
    (Fsign(a->y + W) << 2) |
    (Fsign(W - a->y) << 3) |
    (Fsign(a->z)     << 4) |
    (Fsign(W - a->z) << 5);
}

VTX_FUNCTION
int vtx_znear_clip_flags(const vtx_t *a)
{
  return Fsign(a->z);
}

#endif /* #ifndef _VTX_INL_ */
