/**
 * @ingroup sha123
 * @file    sha123/header.h
 * @author  Benjamin Gerard
 * @date    2003/04/10
 * @brief   mpeg frame header.
 *
 */
#ifndef _SHA123_HEADER_H_
#define _SHA123_HEADER_H_

/** Mpeg modes. */
enum {
  MPG_MD_STEREO = 0,
  MPG_MD_JOINT_STEREO,
  MPG_MD_DUAL_CHANNEL,
  MPG_MD_MONO
};

/** Decoded header info. */
typedef struct {
  unsigned int layer;         /**< Layer type [1..3].                   */
  unsigned int frame_bytes;   /**< Frame size in bytes.                 */
  unsigned int lsf;           /**< Dunno exactly what it is ?           */
  unsigned int log2chan;      /**< Log2 of number of channel [0..1].    */
  unsigned int ssize;         /**< layer III side info size (in bytes). */
  unsigned int sampling_rate; /**< Sampling rate (in hz).               */
  unsigned int bit_rate;      /**< Bit rate (in kbps).                  */
  unsigned int sampling_idx;  /**< sampling rate index [0..8]           */
} sha123_header_info_t;

/** Mpeg header. */
typedef struct {
  unsigned int emphasis  : 2;
  unsigned int original  : 1;
  unsigned int copyright : 1;
  unsigned int mode_ext  : 2;
  unsigned int mode      : 2;

  unsigned int private   : 1;
  unsigned int pad       : 1;
  unsigned int sr_index  : 2;
  unsigned int br_index  : 4;

  unsigned int not_protected : 1;
  unsigned int option    : 2;
  unsigned int id        : 1;

  unsigned int not_mpg25 : 1;
  unsigned int sync7ff   : 11;
} sha123_header_t;

int sha123_header_check(const sha123_header_t head);
int sha123_decode_header(sha123_header_info_t * info,
			 const sha123_header_t head);
const char * sha123_modestr(unsigned int mode);

void sha123_header_dump(const sha123_header_t head);
void sha123_header_info_dump(const sha123_header_info_t * info);

#define sha123_header_get(head) (*(unsigned int *)(head))
#define sha123_header_set(head,v) (*(unsigned int *)(head)) = (v)

#endif /* #ifndef _SHA123_HEADER_H_ */
