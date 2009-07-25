#ifndef _SHA123_DCT12_H_
#define _SHA123_DCT12_H_

#include "sha123/types.h"

void sha123_dct12(const real * in,
		  real * rawout1, real * rawout2,
		  const real * wi,
		  real * ts);

#endif /* #define _SHA123_DCT12_H_ */
