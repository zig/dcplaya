/**
 * @ingroup   SHAblitter
 * @file      SHAblitter.cxx
 * @brief     Soft blitter class definition.
 * @date      2001/08/01
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAblitter.h,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */

/** @defgroup SHAblitter  Software blitter
 **/

#ifndef _SHABLITTER_H_
#define _SHABLITTER_H_

#include "SHAsys/SHAsysTypes.h"

/** Software blitter class.
 * @ingroup   SHAblitter
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAblitter
{
public:
  SHAblitter();
  void Source(SHApixelFormat_e format, void *data, int width, int height, int modulo);
  void Destination(SHApixelFormat_e format, void *data, int width, int height, int modulo);
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

  Color keyColor;

  FilterMatrix filterMtx;

  int CheckSameFormat(void);
  static void SetImageInfo(ImageInfo * inf, SHApixelFormat_e format, void *data, int width, int height, int modulo);


  int Error(const char *errMsg, int errNo = -1);

};

#endif //#define _SHABLITTER_H_
