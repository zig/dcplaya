/**
 * @ingroup dcplaya_math_devel
 * @file    math_int.h
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/02/12
 * @brief   Integer mathematic support.
 *
 * $Id: math_int.h,v 1.3 2003-03-22 00:35:27 ben Exp $
 */

#ifndef _MATH_INT_H_
#define _MATH_INT_H_

#include "extern_def.h"

/** @name Integer mathematic basics.
 *  @ingroup dcplaya_math_devel
 *  @{
 */

DCPLAYA_EXTERN_C_START

/** Small Bijection for calculate a square root. */
unsigned int int_sqrt (unsigned int x);

DCPLAYA_EXTERN_C_END

/** @} */

#endif /* #ifndef _MATH_INT_H_ */
