/**
 * @ingroup dcplaya_devel
 * @file    SHAwrapperImage.h
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/09/27
 * @brief   Image definition.
 *
 * $Id: SHAwrapperImage.h,v 1.4 2002-12-16 23:39:36 ben Exp $
 */
#ifndef _SHAWRAPPERIMAGE_H_
#define _SHAWRAPPERIMAGE_H_

#include "translator/SHAsys/SHAsysTypes.h"

/** Image format enumeration.
 *  @ingroup dcplaya_devel
 **/
typedef SHApixelFormat_e SHAwrapperImageFormat_e;

/* /\** @name Image format macros. */
/*  *  @ingroup dcplaya_devel */
/*  *  @{ */
/*  **\/ */

/* /\** Get log 2 of number of bit per pixel. *\/ */
/* #define SHAWIF_LOG2BPP(A)  SHAPF_LOG2BPP(A) */
/* /\** Get number of bit per pixel. *\/ */
/* #define SHAWIF_BPP(A)      SHAPF_BPP(A) */

/* /\**@}*\/ */

/** Image description structure.
 *  @ingroup dcplaya_devel
 *
 *    Image data is allocated in the same buffer that the image description
 *    structure.
 */
typedef struct {

  SHAwrapperImageFormat_e type; /**< Image type.                      */
  int width;             /**< Image width in pixel.                   */
  int height;            /**< Image heigth in pixel.                  */
  int lutSize;           /**< Number of entry in color look up table. */
  unsigned char data[4]; /**< Image bitmap data.                      */
} SHAwrapperImage_t;

#endif /* #define _SHAWRAPPERIMAGE_H_ */
