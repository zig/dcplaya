/**
 * @file    SHAwrapperImage.h
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/09/27
 * @brief   Image definition.
 *
 * $Id: SHAwrapperImage.h,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */
#ifndef _SHAWRAPPERIMAGE_H_
#define _SHAWRAPPERIMAGE_H_

typedef enum {
  // Less than 8 bit per pixel
  SHAWIF_IND1     = 0x10,
  SHAWIF_IND2     = 0x01,
  SHAWIF_IND4     = 0x02,

  // 8 bit per pixel
  SHAWIF_IND8     = 0x03,
  SHAWIF_RGB233   = 0x13,
  SHAWIF_GREY8    = 0x23,
  SHAWIF_ARGB2222 = 0x33,

  // 16 bit per pixel
  SHAWIF_RGB565   = 0x04,
  SHAWIF_ARGB1555 = 0x14,
  SHAWIF_ARGB4444 = 0x24,
  SHAWIF_AIND88   = 0x34,

  // 32 bit per pixel
  SHAWIF_ARGB32   = 0x05,
  SHAWIF_UNKNOWN  = 0x00
  
} SHAwrapperImageFormat_e;

#define SHAWIF_LOG2BPP(A)  ((int)(A)&7)
#define SHAWIF_BPP(A)      (1<<SHAWIF_LOG2BPP(A))

typedef struct {
  int type;
  int width;
  int height;
  int lutSize;
} SHAwrapperImage_t;

#endif /* #define _SHAWRAPPERIMAGE_H_ */
