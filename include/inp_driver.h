/**
 * Dreammp3 - input driver
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: inp_driver.h,v 1.1 2002-08-26 14:15:00 ben Exp $
 */

#ifndef _INP_DRIVER_H_
#define _INP_DRIVER_H_

#include "playa_info.h"
#include "any_driver.h"

/** Input driver */
typedef struct
{
  /** Any driver common structure :  {nxt, id, name} */ 
  any_driver_t common;

  /** Id code for FILE_TYPE. Set it as you mind. */
  int id;

  /** Extensions list terminated by a double zero. */
  const char * extensions;

  /** Start a new file and set info. */
  int (*start)(const char *fn, decoder_info_t *info);
  /** Stop playing file */
  int (*stop)(void);

  /** Decode next frame and updates info. */
  int (*decode)(decoder_info_t *info);
  
  /** Get file info. Can be use as is_mine() function. */
  int (*info)(playa_info_t * info, const char *fn);

} inp_driver_t;

#endif
