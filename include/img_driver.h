/**
 * @ingroup dcplaya_img_plugin_devel
 * @file    img_driver.h
 * @author  benjamin gerard <ben@sahipa.com>
 * @date    2002/12/14
 * @brief   Image Plugin API.
 *
 * $Id: img_driver.h,v 1.3 2003-03-19 05:16:16 ben Exp $
 */

#ifndef _IMG_DRIVER_H_
#define _IMG_DRIVER_H_

/** @defgroup  dcplaya_img_plugin_devel  Image Plugin API
 *  @ingroup   dcplaya_plugin_devel
 *  @author    benjamin gerard <ben@sashipa.com>
 *  @brief     Programming dcplaya image plugins
 *
 *  Image plugins are used for loading image file.
 */

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#include "playa_info.h"
#include "any_driver.h"
#include "translator/translator.h"

/** Image format enumeration.
 *  @ingroup dcplaya_img_plugin_devel
 **/
typedef SHAwrapperImageFormat_e image_format_e;

/** Image description structure.
 *  @ingroup dcplaya_img_plugin_devel
 */
typedef SHAwrapperImage_t image_t;

/** Image driver structure.
 *  @ingroup dcplaya_img_plugin_devel
 */
typedef struct
{
  /** Any driver common structure :  {nxt, id, name} */ 
  any_driver_t common;

  /** Id code for FILE_TYPE. Set it as you mind. */
  int id;

  /** Extensions list terminated by a double zero. */
  const char * extensions;

  /** Translator object. */
  translator_t translator;

} img_driver_t;

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _IMG_DRIVER_H_ */
