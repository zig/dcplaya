/**
 * @ingroup   SHAblitter
 * @file      SHAblitterFastCopy.cxx
 * @brief     Soft blitter fast copy implementation.
 * @date      2001/08/01
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAblitterFastCopy.cxx,v 1.3 2003-01-31 14:48:30 ben Exp $
 */

#include "SHAblitter/SHAblitter.h"
#include "SHAsys/SHAsysTypes.h"
#include <string.h>

#include "sysdebug.h"

void SHAblitter::FastCopy(void)
{
  int lineSize = dst.bytePerPix * dst.width;
  if (!(src.eolSkip | dst.eolSkip)) {
    // Linear memory copy
    lineSize *= dst.height;
//     SDDEBUG("Copy one block %d bytes\n", lineSize);
    memcpy(dst.data, src.data, lineSize);
    dst.data += lineSize;
    src.data += lineSize;
  } else {
    int lines = dst.height;
    if (lines) {

      SHAuint8 * d = dst.data;
      const int addDst = lineSize + dst.eolSkip;
      SHAuint8 * s = src.data;
      const int addSrc = lineSize + src.eolSkip;

//       SDDEBUG("Copy %d lines, src [%d,%d,%d,%d], dst  [%d,%d,%d,%d]\n",
// 	      lines,
// 	      src.bytePerPix, src.width, addSrc, src.eolSkip,
// 	      dst.bytePerPix, dst.width, addDst, dst.eolSkip);

      do {
        memcpy(d, s, lineSize);
        d += addDst;
        s += addSrc;
      } while (--lines);
      dst.data = d;
      src.data = s;
    }
  }
}
