#include "sha123/dct64.h"

extern sha123_dct_info_t sha123_dct64_std_info;

sha123_dct_info_t * sha123_dct64_info [] = {
  &sha123_dct64_std_info,
  0,
};

sha123_dct64_t sha123_dct64 = 0;
