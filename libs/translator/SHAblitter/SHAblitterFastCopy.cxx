/**
 * @ingroup   SHAblitter
 * @file      SHAblitterFastCopy.cxx
 * @brief     Soft blitter fast copy implementation.
 * @date      2001/08/01
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAblitterFastCopy.cxx,v 1.2 2002-12-16 23:39:36 ben Exp $
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
// 	SDDEBUG("Copy one block %d bytes\n", lineSize);
    memcpy(dst.data, src.data, lineSize);
    dst.data += lineSize;
    src.data += lineSize;
  } else {
    int lines = dst.height;
    if (lines) {
// 	  SDDEBUG("Copy %d lines, skip [%d,%d]\n", lines,src.eolSkip,dst.eolSkip);

      SHAuint8 * d = dst.data;
      const int addDst = lineSize + dst.eolSkip;
      SHAuint8 * s = src.data;
      const int addSrc = lineSize + src.eolSkip;
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
