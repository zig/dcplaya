/**
 * @ingroup   SHAblitter
 * @file      SHAblitter.cxx
 * @brief     Soft blitter class implementation.
 * @date      2001/08/01
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAblitter.cxx,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */

//#include "SHAsys/SHAsysInfo.h"
#include "SHAblitter/SHAblitter.h"

/*int SHAblitter::CheckSameFormat(void)
{
  if (src.type!= dst.type) {
    return Error("SHAblitter:: Source and destination format differ.");
  }
  return 0;
}
*/

SHAblitter::SHAblitter()
{
}

void SHAblitter::SetImageInfo(ImageInfo * inf, SHApixelFormat_e format, void *data, int width, int height, int modulo)
{
  inf->type = format;
  inf->data = (unsigned char *)data;
  inf->width = width;
  inf->height = height;
  inf->eolSkip = modulo;
  inf->bytePerPix = SHAPF_BPP(format) >> 3;
}

void SHAblitter::Source(SHApixelFormat_e format, void *data, int width, int height, int modulo)
{
  SetImageInfo(&src, format, data, width, height, modulo);
}

void SHAblitter::Destination(SHApixelFormat_e format, void *data, int width, int height, int modulo)
{
  SetImageInfo(&dst, format, data, width, height, modulo);
}

void SHAblitter::Copy(void)
{
  if (src.type == dst.type) {
    if ((src.width ^ dst.width) | (src.height ^ dst.height)) {
      FastStretch();
    }  else {
      FastCopy();
    }
  }
}
