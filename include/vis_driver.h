/**
 * Dreammp3 - visual driver
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: vis_driver.h,v 1.4 2002-09-06 23:16:09 ben Exp $
 */


#ifndef _VIS_DRIVER_H_
#define _VIS_DRIVER_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "any_driver.h"
#include "viewport.h"
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
  int (*process)(viewport_t * vp, matrix_t projection, int elapsed_ms);

  /** Process TA opaque list. */
  int (*opaque_render)(void);

  /** Process TA translucent list. */
  int (*translucent_render)(void);

} vis_driver_t;

DCPLAYA_EXTERN_C_END

#endif
