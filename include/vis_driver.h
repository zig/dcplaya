/**
 * @ingroup dcplaya_vis_plugin_devel
 * @file    vis_driver.h
 * @author  benjamin gerard
 * @date    2002
 * @brief   visual plugin API.
 *
 * $Id: vis_driver.h,v 1.7 2003-03-26 23:02:48 ben Exp $
 */

#ifndef _VIS_DRIVER_H_
#define _VIS_DRIVER_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup  dcplaya_vis_plugin_devel  Visual Plugin
 *  @ingroup   dcplaya_plugin_devel
 *  @author    benjamin gerard
 *  @brief     programming dcplaya visual plugins
 *
 *  Visual plugins are dcplaya graphical effects drivers.
 *  @{
 */

#include "any_driver.h"
#include "draw/viewport.h"
#include "matrix.h"

/** Visual driver structure.
 */
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

/**@*/

DCPLAYA_EXTERN_C_END

#endif
