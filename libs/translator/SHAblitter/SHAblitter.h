/**
 * @ingroup   dcplaya_translator_devel
 * @file      SHAblitter.h
 * @brief     Soft blitter class definition.
 * @date      2001/08/01
 * @author    benjamin gerard
 * @version   $Id: SHAblitter.h,v 1.2 2003-03-26 23:02:48 ben Exp $
 */

/** @defgroup  SHAblitter  Software blitter
 *  @ingroup   dcplaya_translator_devel
 *  @brief     Software blitter.
 *  @author    benjamin gerard
 *  @{
 */

#ifndef _SHABLITTER_H_
#define _SHABLITTER_H_

#include "SHAsys/SHAsysTypes.h"

/** Software blitter class.
 *  @author    benjamin gerard
 */
class SHAblitter
{
public:
  /// Clean c-tor.
  SHAblitter();
  /// Set blitter source image.
  void Source(SHApixelFormat_e format, void *data, int width, int height, int modulo);
  /// Set blitter destination image.
  void Destination(SHApixelFormat_e format, void *data, int width, int height, int modulo);
  /// Launch blitter copy.
  void Copy(void);

protected:

  /// Copy same pixel format and no dimension change.
  void FastCopy(void);

  /// Copy same pixel format with dimension change.
  void FastStretch(void);

  typedef unsigned int Color;

  typedef struct {
    int mtx[3][3];
    int fix;
  } FilterMatrix;

  typedef struct {
    SHApixelFormat_e type;  ///< Pixel format
    unsigned char * data;   ///< Bitmap data address
    int bytePerPix;         ///< Number of byte per pixel
    int eolSkip;            ///< Number of byte to skip at each end of line
    int  width;             ///< Image width in pixel
    int  height;            ///< Image height in pixel
  } ImageInfo;

  ImageInfo src;          ///< Source image parameters
  ImageInfo dst;          ///< Destination image parameters

  Color keyColor;         ///< Key color (unused).

  FilterMatrix filterMtx; ///< Filter matrix. (unused).

  int CheckSameFormat(void);
  static void SetImageInfo(ImageInfo * inf, SHApixelFormat_e format,
			   void *data, int width, int height, int modulo);


  int Error(const char *errMsg, int errNo = -1);

};

/** @} */

#endif //#define _SHABLITTER_H_
