/**
 * @ingroup   SHAblitter
 * @file      SHAblitter.cxx
 * @brief     Soft blitter class implementation.
 * @date      2001/08/01
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAblitter.cxx,v 1.2 2002-12-16 23:39:36 ben Exp $
 */

//#include "SHAsys/SHAsysInfo.h"
#include "SHAblitter/SHAblitter.h"

#include "sysdebug.h"

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

void SHAblitter::SetImageInfo(ImageInfo * inf, SHApixelFormat_e format,
							  void *data, int width, int height, int modulo)
{
  inf->type = format;
  inf->data = (unsigned char *)data;
  inf->width = width;
  inf->height = height;
  inf->eolSkip = modulo;
  inf->bytePerPix = SHAPF_BPP(format) >> 3;
}

void SHAblitter::Source(SHApixelFormat_e format, void *data,
						int width, int height, int modulo)
{
  SetImageInfo(&src, format, data, width, height, modulo);
}

void SHAblitter::Destination(SHApixelFormat_e format, void *data,
							 int width, int height, int modulo)
{
  SetImageInfo(&dst, format, data, width, height, modulo);
}

void SHAblitter::Copy(void)
{
  if (src.type == dst.type) {
// 	SDDEBUG("Blitter Copy [%dx%d] -> [%dx%d]\n",
// 		   src.width,src.height,dst.width,dst.height);
    if ((src.width ^ dst.width) | (src.height ^ dst.height)) {
// 	  SDDEBUG("FastStretch\n");
      FastStretch();
    }  else {
// 	  SDDEBUG("FastCopy\n");
      FastCopy();
    }
  }
}
