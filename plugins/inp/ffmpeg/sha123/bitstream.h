/**
 * @ingroup sha123
 * @file    sha123/bitstream.h
 * @author  benjamin gerard
 * @date    2003/04/09
 * @brief   bitstream read access.
 */

#ifndef _BITSTREAM_H_
#define _BITSTREAM_H_

/** Bit stream info struct. */
typedef struct
{
  int idx;                      /**< Current bit in wordpointer */
  const unsigned char * ptr;    /**< Pointer to current word.   */
  const unsigned char * buffer; /**< Bitstream buffer.          */
  const unsigned char * end;    /**< Bitstream buffer size.     */
} bsi_t;

#ifndef BSI_NO_INLINE

# define BSI_EXPORT static inline
# include "sha123/bitstream_inline.h"

#else

void bsi_set(bsi_t * bsi, void * bitbuffer, unsigned int size);
void bsi_backbits(bsi_t * bsi, const int nbits);
unsigned int bsi_getbitoffset(bsi_t * bsi) ;
unsigned int bsi_getbyte(bsi_t * bsi);
unsigned int bsi_getbits(bsi_t * bsi, const int nbits);
unsigned int bsi_getbits_fast(bsi_t * bsi, const int nbits);
unsigned int bsi_getbits_long(bsi_t * bsi, const int nbits);
unsigned int bsi_get1bit(bsi_t * bsi);
void bsi_skipbits(bsi_t * bsi, const int nbits);
void bsi_skipbytes(bsi_t * bsi, const int nbytes);
void bsi_collectbits_fast(bsi_t * bsi, const int nbits,
			  void * buffer, int n);

#endif

unsigned int * bsi_collectbits(bsi_t * bsi, const int nbits,
			       unsigned int * buffer, int n);

#endif /* #ifndef _BITSTREAM_H_ */
