/**
 * @ingroup dcplaya_plugin_img
 * @file    img_driver.h
 * @author  benjamin gerard <ben@sahipa.com>
 * @date    2002/12/14
 * @brief   dcplaya image plugin.
 *
 * $Id: img_driver.h,v 1.1 2002-12-14 16:15:36 ben Exp $
 */

#ifndef _IMG_DRIVER_H_
#define _IMG_DRIVER_H_

/** @defgroup dcplaya_plugin_img  Image driver API
 *  @ingroup  dcplaya_plugin_devel
 *
 *  Image plugins are for loading image file.
 */

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "playa_info.h"
#include "any_driver.h"

/** Image format enumeration.
 *  @ingroup dcplaya_plugin_img
 **/
typedef enum {
  /* Less than 8 bit per pixel */
  SHAWIF_IND1     = 0x10, /**< Indexed 1 bit per pixel (black and white).   */
  SHAWIF_IND2     = 0x01, /**< Indexed 2 bit per pixel (4 colors).          */
  SHAWIF_IND4     = 0x02, /**< Indexed 4 bit per pixel (16 colors).         */

  /* 8 bit per pixel */
  SHAWIF_IND8     = 0x03, /**< Indexed 8 bit per pixel (256 colors).        */
  SHAWIF_RGB233   = 0x13, /**< Direct RGB 8 bit per pixel.                  */
  SHAWIF_GREY8    = 0x23, /**< Direct 8 bit intensity (grey level).         */
  SHAWIF_ARGB2222 = 0x33, /**< Direct ARGB 8 bit per pixel.                 */

  /* 16 bit per pixel */
  SHAWIF_RGB565   = 0x04, /**< Direct 16 bit per pixel R:5 G:6 B:5.         */
  SHAWIF_ARGB1555 = 0x14, /**< Direct 16 bit per pixel A:1 R:5 G:5 B:5.     */
  SHAWIF_ARGB4444 = 0x24, /**< Direct 16 bit per pixel A:4 R:4 G:4 B:4.     */
  SHAWIF_AIND88   = 0x34, /**< Interlaced 8 alpha bit, 8 indexed color bir. */

  /* 32 bit per pixel */
  SHAWIF_ARGB32   = 0x05, /**< Direct 32 bit per pixel A:8 R:8 G:8 B:8.     */
  SHAWIF_UNKNOWN  = 0x00  /**< Reserved for error or unknown format.        */
  
} image_format_e;

/** @name Image format macros.
 *  @ingroup dcplaya_plugin_img
 *  @{
 **/

/** Get log 2 of number of bit per pixel. */
#define SHAWIF_LOG2BPP(A)  ((int)(A)&7)
/** Get number of bit per pixel. */
#define SHAWIF_BPP(A)      (1<<SHAWIF_LOG2BPP(A))

/**@}*/

/** Image description structure.
 *
 *    Image data is allocated in the same buffer that the image description
 *    structure.
 */
typedef struct {
  image_format_e type;   /**< Image type.                             */
  int width;             /**< Image width in pixel.                   */
  int height;            /**< Image heigth in pixel.                  */
  int lutSize;           /**< Number of entry in color look up table. */
  unsigned char data[4]; /**< Image bitmap data.                      */
} image_t;

/** Image driver structure.
 *  @ingroup dcplaya_plugin_img
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
  void * translator;

/*   /\** Get image info from file. *\/ */
/*   int (*info_file)(const char * fn, image_t * image); */

/*   /\** Get image info from memory. *\/ */
/*   int (*info_file)(const void * buffer, unsigned int size, image_t * image); */

/*   /\** Load image from file. *\/ */
/*   image_t * (*load_file)(const char * fn); */

/*   /\** Load image from memory. *\/ */
/*   image_t * (*load_memory)(const void * buffer, unsigned int size); */

} img_driver_t;

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _IMG_DRIVER_H_ */
