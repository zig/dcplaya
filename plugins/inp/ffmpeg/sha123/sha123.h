/**
 * @ingroup sha123
 * @file    sha123/sha123.h
 * @author  Benjamin Gerard
 * @author  Michael Hipp
 * @date    2003/04/10
 * @brief   sha123 structures.
 *
 *   This file is based on Michael Hipp mpg123.h which was based on musicout.h
 *   from mpegaudio package.
 */

#ifndef _SHA123_H_
#define _SHA123_H_

#include "sha123/api.h"
#include "sha123/defs.h"
#include "sha123/types.h"
#include "sha123/bitstream.h"
#include "sha123/equalizer.h"
#include "sha123/header.h"

#include "istream/istream.h"

/** Maximum size allowed for a single frame.
 * @note ISO limitation is 7680 bit.
 */
#define SHA123_MAXFRAMESIZE      1792

#define SBLIMIT     32 /**< Maximum number of sub-band. */
#define SCALE_BLOCK 12 /**< ???. */
#define SSLIMIT     18 /**< ???. */

/** Stereo synthesis function. */
typedef int (*sha123_synth_st_t) (const real *, int, unsigned char *, int *);
/** Mono synthesis function. */
typedef int (*sha123_synth_mn_t) (const real *, unsigned char *, int *);
/** Process layer function. */
typedef int (*sha123_dolayer_t) (sha123_t *);

/* typedef void (*sha123_dct36_t)(real *, real *, real *, real *, real *); */

/** Frame structure. */
typedef struct _sha123_frame_t {
  unsigned int num;           /**< Frame number.       */

  sha123_header_t header;     /**< Frame header.       */
  sha123_header_info_t info;  /**< Frame information.  */

  sha123_synth_st_t synth_st; /**< Stereo synthesis function for this layer. */
  sha123_synth_mn_t synth_mn; /**< Mono synthesis function for this layer.   */
  sha123_dolayer_t do_layer;  /**< Process function for this layer.          */

  /* Specific layer I and II. */
  struct al_table *alloc;
  int jsbound;
  int II_sblimit;

  /** layer III side info buffer.
   *
   *   Layer III frame begins an optionnal 16 bit CRC (depending on
   *   sha123_header_t::not_protected field) followed by side info data.
   *   The size of the side info data depends on sha123_header_info_t::lsf
   *   and sha123_header_info_t::log2chan fields.
   *    - @b lsf:0 @b chan:0 : 17 bytes
   *    - @b lsf:0 @b chan:1 : 32 bytes
   *    - @b lsf:1 @b chan:0 :  9 bytes
   *    - @b lsf:1 @b chan:1 : 17 bytes
   *
   * @see sha123_III_side_info_t
   */
  unsigned char sideinfo[34];

  /** frame data structure.
   *
   *  @warning data must follow prev_data field.
   */
  struct {
    /** current frame data bytes (without side info). */
    unsigned int size;
    /** Previous frames bit reservoir (layer III only). */
    unsigned char prev_data[512];
    /** Current frame data. */
    unsigned char data[SHA123_MAXFRAMESIZE];
  } buffer;

} sha123_frame_t;

typedef enum {
  SHA123_INIT = 0,
  SHA123_NEED_HEADER,
  SHA123_NEED_FRAME,
  SHA123_DECODER_ERROR = -1
} sha123_status_t;


/** sha123 main structure. */
struct _sha123_t {

  /** Current decoder status. */
  sha123_status_t status;

  /** Current frame info (external). */
  sha123_info_t xinfo;

  /** Loop info. */
  struct {
    int count;
    unsigned int pos;
  } loop;

  sha123_header_info_t prev_info;
  sha123_frame_t frame;

  struct {
    int frq;
    int downsample;
    int Long[24];
    int Short[14];
  } limits;

  /** Input stream. */
  istream_t * istream;

   /** Bit stream. */
  bsi_t bsi;
  sha123_equalizer_t equalizer;

  unsigned char *pcm_sample;
  int pcm_point;

  /** Last error string. */
  const char * errstr;
};

/** Set error message.
 *
 * @param  sha123
 * @param  errmsg  New error message.
 * @return new error message (currently always errmsg).
 */
const char * sha123_set_error(sha123_t *sha123, const char *errmsg);

/** Used by huffman (I guess.). */
struct al_table
{
  short bits;
  short d;
};

#endif /* #ifndef _SHA123_H_ */
