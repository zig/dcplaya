/** @ingroup  dcplaya_obj_plugin_devel
 *  @file     obj_driver.h
 *  @author   benjamin gerard
 *  @brief    3d object plugin
 *  $Id: obj_driver.h,v 1.3 2003-03-26 23:02:48 ben Exp $
 */

#ifndef _OBJ_DRIVER_H_
#define _OBJ_DRIVER_H_

#include "any_driver.h"
#include "obj3d.h"

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup  dcplaya_obj_plugin_devel  3D Object driver
 *  @ingroup   dcplaya_plugin_devel
 *  @brief     3d object plugin
 *
 *    Object plugins are use to add 3d object to dcplaya. This plugin type
 *    is temporary. It should be replace by a smarter plugin like
 *    for img plugin that allow to add loader function for object file.
 *
 *  @author    benjamin gerard
 *  @{
 */

/** 3D-Object driver */
typedef struct {
  any_driver_t common; /**< All driver common structure */
  obj_t        obj;    /**< 3d object definition */
} obj_driver_t;

/* This fonctions are implemented in obj3d.c */
extern int obj3d_init(any_driver_t *);
extern int obj3d_shutdown(any_driver_t *);
extern driver_option_t * obj3d_options(any_driver_t * driver, int idx,
                                       driver_option_t * opt);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif
