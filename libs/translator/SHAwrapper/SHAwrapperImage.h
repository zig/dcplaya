/**
 * @ingroup dcplaya_devel
 * @file    SHAwrapperImage.h
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/09/27
 * @brief   Image definition.
 *
 * $Id: SHAwrapperImage.h,v 1.3 2002-12-15 16:15:03 ben Exp $
 */
#ifndef _SHAWRAPPERIMAGE_H_
#define _SHAWRAPPERIMAGE_H_

/** Image format enumeration.
 *  @ingroup dcplaya_devel
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
 
} SHAwrapperImageFormat_e;

/** @name Image format macros.
 *  @ingroup dcplaya_devel
 *  @{
 **/

/** Get log 2 of number of bit per pixel. */
#define SHAWIF_LOG2BPP(A)  ((int)(A)&7)
/** Get number of bit per pixel. */
#define SHAWIF_BPP(A)      (1<<SHAWIF_LOG2BPP(A))

/**@}*/


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
