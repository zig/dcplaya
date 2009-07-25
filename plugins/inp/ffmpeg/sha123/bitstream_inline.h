/* Do not compile this file. Provided for inlining. */

#include "sha123/debug.h"

#ifdef SHA123_PARANO

static int bsi_check(bsi_t * bsi, const char * name, int idx0)
{
  if (!bsi) {
    sha123_debug("bsi_check [%s] : null pointer\n", name);
    return -1;
  }
  if ((unsigned int)bsi->idx > 7u) {
    sha123_debug("bsi_check [%s] : idx [%d] out of range\n", name, bsi->idx);
    return -1;
  }
  if (!bsi->buffer
      || bsi->end <= bsi->buffer
      || bsi->ptr < bsi->buffer
      || bsi->ptr > bsi->end) {
    sha123_debug("bsi_check [%s] : weird pointer [%p %p,%d %p]\n",
		 name, bsi->buffer, bsi->ptr, bsi->idx, bsi->end);
    return -1;
  }
  if (idx0 && bsi->idx) {
    sha123_debug("bsi_check [%s] : idx [%d] not 0\n", name, bsi->idx);
    return -1;
  }
  return 0;
}

static int bsi_check_parm(bsi_t * bsi, const char * name,
			  int min, int v, int max)
{
  if (v < min || v > max) {
    sha123_debug("bsi_check : [%s] out of range [%d < %d < %d]\n",
		 name, min,v,max);
    return -1;
  }
  return 0;
}

# define BSI_CHECK(N) if (bsi_check(bsi, N, 0))\
 { SHA123_BREAKPOINT(0xdeadbeef); } else
# define BSI_CHECK_ALIGNED(N) if (bsi_check(bsi, N, 1))\
 { SHA123_BREAKPOINT(0xdeadbeef); } else
# define BSI_CHECK_RANGE(N,A,B,C) if (bsi_check_parm(bsi, (N), (A), (B), (C)))\
 { SHA123_BREAKPOINT(0xdeadbeef); } else

#else

# define BSI_CHECK(N)
# define BSI_CHECK_ALIGNED(N)
# define BSI_CHECK_RANGE(N,A,B,C)

#endif


BSI_EXPORT
void bsi_set(bsi_t * bsi, void * bitbuffer, unsigned int size)
{
  bsi->ptr = bitbuffer;
#ifdef SHA123_PARANO
  bsi->buffer = bsi->ptr;
  bsi->end = bsi->buffer + size;
#endif
  bsi->idx = 0;
  BSI_CHECK(__FUNCTION__);
}


BSI_EXPORT
void bsi_backbits(bsi_t * bsi, const int nbits)
{
  register int idx = bsi->idx - nbits;

  BSI_CHECK_RANGE(__FUNCTION__,0,nbits,2048);
  bsi->ptr += idx >> 3;
  bsi->idx = idx & 7;
  BSI_CHECK(__FUNCTION__);
}

BSI_EXPORT
void bsi_skipbits(bsi_t * bsi, const int nbits)
{
  register int idx = bsi->idx + nbits;

  BSI_CHECK_RANGE(__FUNCTION__,0,nbits,2048 * 8);
  bsi->ptr += idx >> 3;
  bsi->idx = idx & 7;
  BSI_CHECK(__FUNCTION__);
}

BSI_EXPORT
void bsi_skipbytes(bsi_t * bsi, const int nbytes)
{
  BSI_CHECK_ALIGNED(__FUNCTION__);
  bsi->ptr += nbytes;
  BSI_CHECK(__FUNCTION__);
}

BSI_EXPORT
unsigned int bsi_getbitoffset(bsi_t * bsi) 
{
  BSI_CHECK(__FUNCTION__);
  return (-bsi->idx) & 7;
}

BSI_EXPORT
unsigned int bsi_getbyte(bsi_t * bsi)
{
  BSI_CHECK_ALIGNED(__FUNCTION__);
  return *bsi->ptr++;
}

BSI_EXPORT
unsigned int bsi_getbits_long(bsi_t * bsi, const int nbits)
{
  register unsigned int rval;
  register int idx = bsi->idx + nbits;
  register const unsigned char * ptr = bsi->ptr;

  BSI_CHECK_RANGE(__FUNCTION__, 0, nbits, 24);

  rval = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
  rval >>= 32-idx;
  rval &= (1<<nbits) - 1;

  bsi->ptr = ptr + (idx >> 3);
  bsi->idx = idx & 7;

  BSI_CHECK(__FUNCTION__);
  return rval;
}

BSI_EXPORT
unsigned int bsi_getbits(bsi_t * bsi, const int nbits)
{
  register unsigned int rval;
  register int idx = bsi->idx + nbits;
  register const unsigned char * ptr = bsi->ptr;

  BSI_CHECK_RANGE(__FUNCTION__, 0, nbits, 16);

  rval = (ptr[0] << 16) | (ptr[1] << 8) | ptr[2];
  rval >>= 24-idx;
  rval &= (1<<nbits) - 1;

  bsi->ptr = ptr + (idx >> 3);
  bsi->idx = idx & 7;

  BSI_CHECK(__FUNCTION__);
  return rval;
}

BSI_EXPORT
unsigned int bsi_getbits_fast(bsi_t * bsi, const int nbits)
{
  register unsigned int rval;
  register int idx = bsi->idx + nbits;
  register const unsigned char * ptr = bsi->ptr;

  BSI_CHECK_RANGE(__FUNCTION__, 0, nbits, 8);

  rval = (ptr[0] << 8) | ptr[1];
  rval >>= 16-idx;
  rval &= (1<<nbits) - 1;

  bsi->ptr = ptr + (idx >> 3);
  bsi->idx = idx & 7;

  BSI_CHECK(__FUNCTION__);

  return rval;
}

BSI_EXPORT
void bsi_collectbits_fast(bsi_t * bsi, const int nbits,
			  void * buffer, int n)
{
  BSI_CHECK_RANGE(__FUNCTION__, 0, nbits, 8);
  if (n) {
    register unsigned int * b = buffer;

    if (!nbits) {
      do {
	*b++ = 0;
      } while (--n);
    } else {
      register unsigned int rval;
      register int idx = bsi->idx;
      register const unsigned char * ptr = bsi->ptr;
      const int mask = (1<<nbits) - 1;

      sha123_debug("bsi_collectbits_fast (%d,%d)\n", nbits, n); 

      rval = (ptr[0] << 8) | ptr[1];

      do {
	idx += nbits;
	*b++ = (rval >> (16-idx)) & mask;
	if (idx & 8) {
	  ++ptr;
	  rval = (rval << 8) | ptr[1];
	  idx &= 7;
	}
      } while (--n);
      bsi->ptr = ptr;
      bsi->idx = idx;
    }
  }
  BSI_CHECK(__FUNCTION__);
}



BSI_EXPORT
unsigned int bsi_get1bit(bsi_t * bsi)
{
  register int rval;
  register int idx = bsi->idx;
  register const unsigned char * ptr = bsi->ptr;
  
  rval = 1 & (*ptr >> (8 - ++idx));
  bsi->ptr = ptr + (idx >> 3);
  bsi->idx = idx & 7;

  BSI_CHECK(__FUNCTION__);
  return rval;
}

