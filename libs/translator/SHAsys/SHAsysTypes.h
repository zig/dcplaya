/**
 */

#ifndef _SHATYPE_H_
#define _SHATYPE_H_

/** @name   Default integer types
 *  @{
 */
typedef long            SHAlong;
typedef signed long     SHAslong;
typedef unsigned long   SHAulong;

typedef int             SHAint;
typedef signed int      SHAsint;
typedef unsigned int    SHAuint;

typedef short           SHAshort;
typedef signed short    SHAsshort;
typedef unsigned short  SHAushort;

typedef char            SHAchar;
typedef signed char     SHAschar;
typedef unsigned char   SHAuchar;

///@}

/** @name   Fixed size integer types
 *  @{
 */
//#include "sys/types.h"

typedef long long           SHAint64;
typedef int                 SHAint32;
typedef short               SHAint16;
typedef signed char         SHAint8;
                            
typedef signed long long    SHAsint64;
typedef signed int          SHAsint32;
typedef signed short        SHAsint16;
typedef signed char         SHAsint8;

typedef unsigned long long  SHAuint64;
typedef unsigned int        SHAuint32;
typedef unsigned short      SHAuint16;
typedef unsigned char       SHAuint8;

/** Pixel format enumeration.
 *
 *   Format of pixel type number : 0xAB
 *   -[A] Most signifiant quartet is type identifier.  
 *   -[B] Log 2 of bit per pixel for the pixel format.
 */
typedef enum {
  // Less than 8 bit per pixel
  SHAPF_IND1     = 0x10,
  SHAPF_IND2     = 0x01,
  SHAPF_IND4     = 0x02,

  // 8 bit per pixel
  SHAPF_IND8     = 0x03,
  SHAPF_RGB233   = 0x13,
  SHAPF_GREY8    = 0x23,
  SHAPF_ARGB2222 = 0x33,

  // 16 bit per pixel
  SHAPF_RGB565   = 0x04,
  SHAPF_ARGB1555 = 0x14,
  SHAPF_ARGB4444 = 0x24,
  SHAPF_AIND88   = 0x34,

  // 32 bit per pixel
  SHAPF_ARGB32   = 0x05,

  SHAPF_UNKNOWN  = 0x00
} SHApixelFormat_e;

#define SHAPF_LOG2BPP(A)  ((int)(A)&7)
#define SHAPF_BPP(A)      (1<<SHAPF_LOG2BPP(A))

#endif //#define _SHATYPE_H_
