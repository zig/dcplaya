/*
 * @ingroup dcplaya_math_devel
 * @file    math_int.h
 * @author  benjamin gerard
 * @date    2002/02/12
 * @brief   Integer mathematic support.
 *
 * $Id: math_int.h,v 1.4 2003-03-26 23:02:47 ben Exp $
 */

#ifndef _MATH_INT_H_
#define _MATH_INT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @name Integer mathematic basics.
 *  @ingroup dcplaya_math_devel
 *  @{
 */

/** Small Bijection for calculate a square root.
 *  @param x value
 *  @return square root of x
 */
unsigned int int_sqrt (unsigned int x);

/** @} */

DCPLAYA_EXTERN_C_END


#endif /* #ifndef _MATH_INT_H_ */
