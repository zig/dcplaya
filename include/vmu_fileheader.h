/**
 * @ingroup dcplaya_vmu_fileheader
 * @file    vmu_fileheader.h
 * @author  benjamin gerard
 * @date    2002/09/30
 * @brief   VMU file header
 *
 *  Thanks to Marcus Comstedt for the documentation.
 *
 * $Id: vmu_fileheader.h,v 1.2 2003-03-26 23:02:48 ben Exp $
 */

#ifndef _VMU_FILEHEADER_H_
#define _VMU_FILEHEADER_H_

#include <arch/types.h>
#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup  dcplaya_vmu_fileheader  VMU file header
 *  @ingroup   dcplaya_vmu_devel
 *  @brief     VMU file header
 *
 * @par VMS file header.
 *
 *   All VMS files should contain a standard header which is used by the
 *   file managers in the VMS and in the DC boot ROM to display
 *   information about the file. (ICONDATA files use a somewhat simplified
 *   headers since they are not shown in the DC boot ROM file manager.)
 *   For data files, the header is stored at the very beginning of the
 *   file. For game files, it begins in the second block of the file
 *   (offset $200). This fact should be reflected by the header offset
 *   field in the VMS directory, which should contain 1 for game files,
 *   and 0 for data files.
 *
 * @par Icon bitmaps.
 *
 *   The header should contain at least one icon bitmap for the file. If
 *   an animated icon is desired, up to three bitmaps can be used (set the
 *   vmu_fileheader_t::icons field of accordingly). The bitmaps are
 *   stored  directly after each other, starting just after the
 *   vmu_fileheader_t (offset 0x80). The bitmaps contain one nybble per
 *   pixel. Each byte thus represents two horizontally adjacent
 *   pixels, the high nybble being the left one and the low nybble
 *   being the right one. Each complete bitmap contains 1024 (32 * 32) 
 *   nybbles, or 512 bytes, for a 32 x 32 pixel resolution. 
 *   
 *  Thanks to Marcus Comstedt for the documentation.
 *
 *  @author    benjamin gerard
 *  @{
 */

/** VMS file header.
 *  @see dcplaya_vmu_fileheader for more information.
 */
typedef struct {
  /** Description as shown in VMS file menu. Padded with 0x20.               */
  char vms_desc[16];
  /** Description as shown in DC boot ROM file manager. Padded with 0x20.    */
  char long_desc[32];
  /** Identifier of application that created the file. Padded with 0x00.     */
  char appli[16];
  /** Number of icons [1..3] (>1 for animated icons). @see vmu_icon_bitmap_t */
  uint16 icons;
  /** Icon animation speed. Is it in frame unit ?                            */
  uint16 anim_speed;
  /** Graphic eyecatch type (0 = none). @see vmu_eyecatch_e.                 */
  uint16 eyecatch;
  /** CRC. Ignored for game files.                                           */
  uint16 crc;
  /** Number of bytes of actual file data following header, icon(s) and
      graphic eyecatch. Ignored for game files.                              */
  uint32 data_size;
  /** Reserved (fill with zeros).                                            */
  uint8 reserved[20];
  /** Icon palette (16 x ARGB4444). Alpha 0 is transparent, 15 is opaque.    */
  uint16 lut[16];
} vmu_fileheader_t;

/** Icon bitmaps.
 *  @see dcplaya_vmu_fileheader for more information.
 */
typedef struct {
  uint8 data[32][16]; /**< Complete bitmap (4 pixels by bytes).              */
} vmu_icon_bitmap_t;

/** Visual eyecatch graphic format. @see vmu_fileheader_t::eyecatch.         */
enum vmu_eyecatch_e {
  /** No eyecatch graphics.                                                  */
  VMU_EYECATCH_NONE       = 0,
  /** ARGB4444 pixel format. Graphics size is 8064 (72x56x2).                */
  VMU_EYECATCH_ARGB4444   = 1,
  /** 256 ARGB4444 paletted mode. Graphics size is 4544 (256x2 + 72x56).     */
  VMU_EYECATCH_LUT256     = 2,
  /** 16 ARGB4444 paletted mode. Graphics size is 2048 (16x2 + 72x56/2).     */
  VMU_EYECATCH_LUT16      = 3
};

/** CRC calculation.
 *
 *   The CRC should be computed on the entire file, including header and
 *   data payload, but excluding any padding in the final block. When
 *   calculating the CRC, set the CRC field itself to 0 to avoid
 *   miscalculating the CRC for the header.
 *
 *  @param  buf   File data including file header as explain above.
 *  @param  size  Fiel size in bytes.
 *
 *  @return crc value [0..0xffff].
 *
 *   The CRC calculation algorithm is as follows (C code):
 *@code
 *int calcCRC(const unsigned char *buf, int size)
 *{
 *  int i, c, n = 0;
 *  for (i = 0; i < size; i++)
 *  {
 *    n ^= (buf[i]<<8);
 *    for (c = 0; c < 8; c++)
 *      if (n & 0x8000)
 *	n = (n << 1) ^ 4129;
 *      else
 *	n = (n << 1);
 *  }
 *  return n & 0xffff;
 *}
 *@endcode
 */
int vmu_fileheader_crc(const uint8 * buf, int size);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #define _VMU_FILEHEADER_H_ */
