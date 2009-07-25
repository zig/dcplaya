#ifndef BSI_NO_INLINE
# define BSI_NO_INLINE
#endif

#include "sha123/bitstream.h"

#define BSI_EXPORT

#include "sha123/bitstream_inline.h"

unsigned int * bsi_collectbits(bsi_t * bsi, const int nbits,
			       unsigned int * buffer, int n)
{
  if (n) {
    const register int mask = (1<<nbits) - 1;
    const register unsigned char * ptr = bsi->ptr;
    register int idx = bsi->idx;

    do {
      register unsigned int rval;

      idx += nbits;
      rval = (ptr[0] << 8) | (ptr[1]);
      rval >>= 16 - idx;
      rval &= mask;
      *buffer++ = rval;
      ptr = ptr + (idx >> 3);
      idx &= 7;
    } while (--n);
    bsi->ptr = ptr;
    bsi->idx = idx;
  }
  return buffer;
}
