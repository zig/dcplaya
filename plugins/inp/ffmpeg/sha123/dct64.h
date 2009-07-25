/**
 *  @ingroup sha123
 *  @file    sha123/dct64.h
 *  @author  benjamin gerard
 *  @date    2003/04/09
 *  @brief   64 points discret cosine transform.
 */

#ifndef _SHA123_DCT64_H_
#define _SHA123_DCT64_H_

#include "sha123/dct.h"

/** dct64 function definition. */
typedef void (*sha123_dct64_t)(real *, real *, const real *);

/** Available dct64 function tables. */
extern sha123_dct_info_t * sha123_dct64_info[];

/** Current dct64 function. */
extern sha123_dct64_t sha123_dct64;

#endif /* #define _SHA123_DCT64_H_ */
