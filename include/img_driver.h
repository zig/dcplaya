/**
 * @ingroup dcplaya_img_plugin_devel
 * @file    img_driver.h
 * @author  benjamin gerard <ben@sahipa.com>
 * @date    2002/12/14
 * @brief   Image Plugin API.
 *
 * $Id: img_driver.h,v 1.4 2003-03-26 23:02:47 ben Exp $
 */

#ifndef _IMG_DRIVER_H_
#define _IMG_DRIVER_H_

/** @defgroup  dcplaya_img_plugin_devel  Image Plugins
 *  @ingroup   dcplaya_plugin_devel
 *  @brief     dcplaya image plugins
 *
 *  Image plugins are used for loading image file. Image plugins use
 *  the @link dcplaya_translator_devel image translator @endlink interface.
 *
 *  @see       dcplaya_translator_devel
 *  @author    benjamin gerard
 *  @{
 */

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#include "playa_info.h"
#include "any_driver.h"
#include "translator/translator.h"

/** Image format enumeration.
 */
typedef SHAwrapperImageFormat_e image_format_e;

/** Image description structure.
 */
typedef SHAwrapperImage_t image_t;

/** Image driver structure.
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

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _IMG_DRIVER_H_ */
