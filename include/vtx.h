/** @ingroup dcplaya_vertex_devel
 *  @file    vtx.h
 *  @author  ben(jamin) gerard <ben@sashipa.com>
 *  @date    2003/01/19
 *  @brief   Vertex.
 *
 * $Id: vtx.h,v 1.6 2003-03-22 00:35:27 ben Exp $
 */

#ifndef _VTX_H_
#define _VTX_H_

#ifndef _VTX_INLINED_
# define _VTX_INLINED_ 1
#endif

#include "extern_def.h"

/** @defgroup  dcplaya_vertex_devel  Vertex support.
 *  @ingroup   dcplaya_math_devel
 *  @brief     Performing vertex operations.
 *
 *  @author    benjamin gerard <ben@sashipa.com>
 */


DCPLAYA_EXTERN_C_START

#if !defined(VTX_FUNCTION)
# if _VTX_INLINED_
#  define VTX_FUNCTION inline static
# else
#  define VTX_FUNCTION
# endif
#endif

/** Vertex definition.
 *  @ingroup dcplaya_vertex_devel
 */
typedef struct {
  float x;  /**< X axis coordinate. */
  float y;  /**< Y axis coordinate. */
  float z;  /**< Z axis coordinate. */
  float w;  /**< W homogeneous coordinate. */
} vtx_t;

#if ! _VTX_INLINED_

/** Identity vertex [0 0 0 1].
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_identity(vtx_t *a);

/** Set vertex to [x y z 1].
 *  @return a
 */
vtx_t * vtx_set(vtx_t *a, const float x, const float y, const float z);

/** Negation (a = -a).
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_neg(vtx_t *a);

/** Negation (r = -a).
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_neg2(vtx_t *r, const vtx_t *a);

/** Addition (a = a + b).
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_add(vtx_t *a, const vtx_t *b);

/** Addition (r = a + b).
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_add3(vtx_t * r, const vtx_t *a, const vtx_t *b);

/** Substraction (a = a - b).
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_sub(vtx_t *a, const vtx_t *b);

/** Substraction (r = a - b).
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_sub3(vtx_t * r, const vtx_t *a, const vtx_t *b);

/** Multiplication (a = a * b).
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_mul(vtx_t *a, const vtx_t *b);

/** Multiplication (r = a * b).
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_mul3(vtx_t * r, vtx_t *a, const vtx_t *b);

/** Scale (a = a * b).
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_scale(vtx_t *a, const float b);

/** Scale (r = a * b).
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_scale3(vtx_t * r, const vtx_t *a, const float b);

/** Scale (a = a*f + b*(1-f)).
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_blend(vtx_t *a, const vtx_t *b, const float f);

/** Scale (r = a*f + b*(1-f)).
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_blend3(vtx_t *r, const vtx_t *a, const vtx_t *b, const float f);

/** Square norm.
 *  @ingroup dcplaya_vertex_devel
 *  @return |a|^2
 */
float vtx_sqnorm(const vtx_t * a);

/** Norm.
 *  @ingroup dcplaya_vertex_devel
 *  @return |a|
 */
float vtx_norm(const vtx_t * a);

/** Norm.
 *  @ingroup dcplaya_vertex_devel
 *  @return |a|
 *  @retval -1 if too small (norm ~= 0).
 */
float vtx_inorm(const vtx_t * a);

/** Normalize a = a/|a| .
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_normalize(vtx_t * a);

/** Normalize r = a/|a| .
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_normalize2(vtx_t * r, const vtx_t * a);


/** Cross product a = a^b.
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 *  @warning a and b must be differente.
 */
vtx_t * vtx_cross_product(vtx_t * a, const vtx_t * b);

/** Cross product r = a^b.
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 *  @warning r must be different from both a and b.
 */
vtx_t * vtx_cross_product3(vtx_t * r, const vtx_t * a, const vtx_t * b);

/** Sinus.
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_sin(vtx_t * a);

/** Sinus into another vertex.
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_sin2(vtx_t * r, const vtx_t * a);

/** Cosinus.
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_cos(vtx_t * a);

/** Cosinus into another vertex.
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 */
vtx_t * vtx_cos2(vtx_t * r, const vtx_t * a);

/** Increments angle a = (a+b) mod 2*PI.
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 */
vtx_t * vtx_inc_angle(vtx_t * a, const vtx_t * b);

/** dot product a.b.
 *  @ingroup dcplaya_vertex_devel
 *  @return a.b
 */
float vtx_dot_product(const vtx_t * a, const vtx_t * b);

/** Square of distance between 2 vertrices.
 *  @ingroup dcplaya_vertex_devel
 */
float vtx_sqdist(const vtx_t * a, const vtx_t * b);

/** Distance between 2 vertrices.
 *  @ingroup dcplaya_vertex_devel
 */
float vtx_dist(const vtx_t * a, const vtx_t * b);

/** Apply homogenous coordinate a = [x/w, y/w, z/w, w]
 *  @ingroup dcplaya_vertex_devel
 *  @return a
 *  @warning For optimize behaviour this function does not affect w.
 *  @see vtx_apply2()
 */
vtx_t * vtx_apply(vtx_t * a);

/** Apply homogenous coordinate r = [a.x/a.w, a.y/a.w, a.z/a.w, 1]
 *  @ingroup dcplaya_vertex_devel
 *  @return r
 *  @see vtx_apply()
 */
vtx_t * vtx_apply2(vtx_t * r, const vtx_t * a);

/** Determine clipping flags.
 *  @return bit-field
 *         - bit 0 : Xmin
 *         - bit 1 : Xmax
 *         - bit 2 : Ymin
 *         - bit 3 : Ymax
 *         - bit 4 : Znear
 *         - bit 5 : Zfar
 */
int vtx_clip_flags(const vtx_t *a);

/** Determine Znear clipping flags.
 *  @ingroup dcplaya_vertex_devel
 *  @return Znear clipping flag
 *  @retval 0 not clipped
 *  @retval 1 clipped
 */
int vtx_znear_clip_flags(const vtx_t *a);

#ifdef DEBUG
/** Print formatted vertex.
 *  @ingroup dcplaya_vertex_devel
 */
void vtx_dump(const vtx_t * v);
#endif

#else

# include "vtx.inl"

#endif /* #else ! _VTX_INLINED_ */

DCPLAYA_EXTERN_C_END

#endif /* #define  _VTX_H_ */

