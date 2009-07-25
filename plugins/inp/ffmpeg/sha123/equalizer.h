/**
 *  @ingroup sha123
 *  @file    sha123/equalizer.h
 *  @author  benjamin gerard
 *  @date    2003/04/09
 *  @brief   equalizer.
 */

#ifndef _SHA123_EQUALIZER_H_
#define _SHA123_EQUALIZER_H_

#include "sha123/types.h"

/** Equalizer structure. */
typedef struct {
  int active;    /**< Equalizer active status. */
  real mul[576]; /**< Equalizer band factor.   */
} sha123_equalizer_t;

/** Set equalizer. */
int sha123_set_eq(sha123_equalizer_t * equa, int on, real preamp, real *b);

#endif /* #define _SHA123_EQUALIZER_H_ */
