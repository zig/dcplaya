/**
 * @file    vmu_fileheader.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   vmu file header
 *
 *  Thanks to Marcus Comstedt for this information.
 *
 * $Id: vmu_fileheader.c,v 1.1 2002-10-21 14:57:00 benjihan Exp $
 */

#include "vmu_fileheader.h"

int vmu_fileheader_crc(const uint8 * buf, int size)
{
  int i, c, n;

  for (n = i = 0; i < size; i++) {
    n ^= (buf[i]<<8);
    for (c = 0; c < 8; c++) {
      if (n & 0x8000) {
	n = (n << 1) ^ 4129;
      } else {
	n = (n << 1);
      }
    }
  }
  return n & 0xffff;
}
