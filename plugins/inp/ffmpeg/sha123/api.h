/**
 * @ingroup sha123
 * @file    sha123/api.h
 * @author  benjamin gerard
 * @date    2003/04/11
 * @brief   sha123 API.
 */

#ifndef _SHA123_API_H_
#define _SHA123_API_H_

#include "istream/istream.h"

typedef struct _sha123_t sha123_t;

/** */
typedef struct {
  istream_t *istream;
  int loop;                  /**< Loop type (0:infinite) */
  /** Bitstream buffer info. */
  struct {
    void * buffer;           /**< bitstream buffer (0:allocated). */
    unsigned int size;       /**< bitstream size (0:default).     */
  } bsi;
  void * cookie;             /**< user cookie. */
} sha123_param_t;

enum {
  SHA123_DECODE_ERROR = -1,
};

/** */
typedef struct {
  int layer;
  int channels;
  int sampling_rate;
} sha123_info_t;

/** Initialize sha123 library. */
int sha123_init(void);

/** Shutdown sha123 library. */
void sha123_shutdown(void);

/** Start mpeg decoder. */
sha123_t * sha123_start(sha123_param_t * param);

/** Close sha123 decoder. */
void sha123_stop(sha123_t * sha123);

/** Decode audio date. */
int sha123_decode(sha123_t * sha123, void * buffer, int n);

/** Get current info. */
const sha123_info_t * sha123_info(sha123_t * sha123);

/** Get last error message. */
const char * sha123_get_errstr(sha123_t * sha123);

#endif /* #define _SHA123_API_H_ */

