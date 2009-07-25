/**
 * @ingroup sha123
 * @file    sha123/dct36.h
 * @author  Benjamin Gerard
 * @date    2003/04/10
 * @brief   sha123 structures.
 *
 */
#ifndef _SHA123_DCT36_H_
#define _SHA123_DCT36_H_

#include "sha123/dct.h"

/** dct64 function definition. */
typedef void (*sha123_dct36_t)(real * inbuf,
			       real * o1, real * o2,
			       const real * wintab, real * tsbuf);

/** Available dct36 function tables. */
extern sha123_dct_info_t * sha123_dct36_info[];

/** Current dct64 function. */
extern sha123_dct36_t sha123_dct36;

#endif /* #define _SHA123_DCT36_H_ */
