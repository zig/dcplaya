/** @ingroup dcplaya_devel
 *  @file    vtx.h
 *  @author  ben(jamin) gerard <ben@sashipa.com>
 *  @date    2003/01/19
 *  @brief   Vertex.
 *
 * $Id: vtx.h,v 1.1 2003-01-20 14:19:43 ben Exp $
 */

#ifndef _VTX_H_
#define _VTX_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** Vertex.
 *  @ingroup dcplaya_devel
 */
typedef struct {
  float x;  /**< X axis coordinate */
  float y;  /**< Y axis coordinate */
  float z;  /**< Z axis coordinate */
  float w;  /**< W homogeneous coordinate */
} vtx_t;

/** @name Vertex fucntions
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Negation (a = -a).
 *  @return a
 */
vtx_t * vtx_neg(vtx_t *a);

/** Negation (r = -a).
 *  @return r
 */
vtx_t * vtx_neg2(vtx_t *r, const vtx_t *a);

/** Addition (a = a + b).
 *  @return a
 */
vtx_t * vtx_add(vtx_t *a, const vtx_t *b);

/** Addition (r = a + b).
 *  @return r
 */
vtx_t * vtx_add3(vtx_t * r, vtx_t *a, const vtx_t *b);

/** Substraction (a = a - b).
 *  @return a
 */
vtx_t * vtx_sub(vtx_t *a, const vtx_t *b);

/** Substraction (r = a - b).
 *  @return r
 */
vtx_t * vtx_sub3(vtx_t * r, vtx_t *a, const vtx_t *b);

/** Multiplication (a = a * b).
 *  @return a
 */
vtx_t * vtx_mul(vtx_t *a, const vtx_t *b);

/** Multiplication (r = a * b).
 *  @return r
 */
vtx_t * vtx_mul3(vtx_t * r, vtx_t *a, const vtx_t *b);

/** Scale (a = a * b).
 *  @return a
 */
vtx_t * vtx_scale(vtx_t *a, const float b);

/** Scale (r = a * b).
 *  @return r
 */
vtx_t * vtx_scale3(vtx_t * r, const vtx_t *a, const float b);

/** Scale (a = a*f + b*(1-f)).
 *  @return a
 */
vtx_t * vtx_blend(vtx_t *a, const vtx_t *b, const float f);

/** Scale (r = a*f + b*(1-f)).
 *  @return r
 */
vtx_t * vtx_blend3(vtx_t *r, const vtx_t *a, const vtx_t *b, const float f);

/** Square norm.
 *  @return |a|^2
 */
float vtx_sqnorm(const vtx_t * a);

/** Norm.
 *  @return |a|
 */
float vtx_norm(const vtx_t * a);

/** Norm.
 *  @return |a|
 *  @retval -1 if too small (norm ~= 0).
 */
float vtx_inorm(const vtx_t * a);

/** Normalize a = a/|a| .
 *  @return a
 */
vtx_t * vtx_normalize(vtx_t * a);

/** Normalize r = a/|a| .
 *  @return r
 */
vtx_t * vtx_normalize2(vtx_t * r, const vtx_t * a);


/** Cross product a = a^b.
 *  @return a
 *  @warning a and b must be differente.
 */
vtx_t * vtx_cross_product(vtx_t * a, const vtx_t * b);

/** Cross product r = a^b.
 *  @return r
 *  @warning r must be different from both a and b.
 */
vtx_t * vtx_cross_product3(vtx_t * r, const vtx_t * a, const vtx_t * b);

/** Sinus.
 *  @return a
 */
vtx_t * vtx_sin(vtx_t * a);

/** Sinus into another vertex.
 *  @return r
 */
vtx_t * vtx_sin2(vtx_t * r, const vtx_t * a);

/** Cosinus.
 *  @return a
 */
vtx_t * vtx_cos(vtx_t * a);

/** Cosinus into another vertex.
 *  @return r
 */
vtx_t * vtx_cos2(vtx_t * r, const vtx_t * a);

/** Increments angle a = (a+b) mod 2*PI.
 *  @return a
 */
vtx_t * vtx_inc_angle(vtx_t * a, const vtx_t * b);

/** dot product a.b.
 *  @return a.b
 */
float vtx_dot_product(const vtx_t * a, const vtx_t * b);

/** Square of distance between 2 vertrices.
 */

float vtx_sqdist(const vtx_t * a, const vtx_t * b);

/** Distance between 2 vertrices.
 */
float vtx_dist(const vtx_t * a, const vtx_t * b);


/** @} */
DCPLAYA_EXTERN_C_END

#endif /* #define  _VTX_H_ */

