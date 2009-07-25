/**
 *  @ingroup sha123
 *  @file    sha123/dct.h
 *  @author  benjamin gerard
 *  @date    2003/04/18
 *  @brief   dct interface.
 */

#ifndef _SHA123_DCT_H_
#define _SHA123_DCT_H_

#include "sha123/types.h"

/** dct init function type. */
typedef int (*sha123_dct_init_t)(int out_scale);
/** dct shutdown function type. */
typedef void (*sha123_dct_shutdown_t)(void);
/** dct process function type. */
typedef void * sha123_dct_function_t;

typedef enum {
  DCT_MODE_DEFAULT = 0,
  DCT_MODE_FASTEST,
  DCT_MODE_SAFEST,
  DCT_MODE_MAX
} sha123_dct_mode_t;

typedef struct {
  const char * name;               /**< dct name. */
  const int modes;                 /**< dct modes. */
  sha123_dct_init_t init;          /**< pointer to init function.  */
  sha123_dct_shutdown_t shutdown;  /**< pointer to shutdown function.  */
  sha123_dct_function_t dct;       /**< pointer to dct function. */
} sha123_dct_info_t;

/** Initiatialize dct system. */
int sha123_dct_init(sha123_dct_mode_t mode, int out_scale);

/** Shutdown dct system. */
void sha123_dct_shutdown(void);

#endif /* #ifndef _SHA123_DCT_H_ */
