/**
 * Dreammp3 - visual driver
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: vis_driver.h,v 1.1.1.1 2002-08-26 14:15:00 ben Exp $
 */


#ifndef _VIS_DRIVER_H_
#define _VIS_DRIVER_H_

#include "any_driver.h"
#include "matrix.h"

/** Input driver */
typedef struct
{
  /** Any driver common structure :  {nxt, id, name} */ 
  any_driver_t common;

  /** Start visual. */
  int (*start)(void);
  
  /** Stop visual. */
  int (*stop)(void);

  /** Process visual calculation. */
  void (*process)(matrix_t projection, int elapsed_ms);

  /** Process TA opaque list. */
  void (*opaque_render)(void);

  /** Process TA transparent list. */
  void (*transparent_render)(void);

} vis_driver_t;

#endif
