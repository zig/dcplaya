/**
 * @ingroup  dcplaya_translator_devel
 * @file     SHAsysTypes.h
 * @author   benjamin gerard
 * @brief    sashipa library type definitions
 *
 * $Id: SHAsysTypes.h,v 1.3 2003-03-26 23:02:48 ben Exp $
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
 *  @ingroup dcplaya_devel
 *
 *   Format of pixel type number : 0xAB
 *   -[A] Most signifiant quartet is type identifier.  
 *   -[B] Log 2 of bit per pixel for the pixel format.
 */
typedef enum {
  /* Less than 8 bit per pixel */
  SHAPF_IND1     = 0x10, /**< Indexed 1 bit per pixel (black and white).   */
  SHAPF_IND2     = 0x01, /**< Indexed 2 bit per pixel (4 colors).          */
  SHAPF_IND4     = 0x02, /**< Indexed 4 bit per pixel (16 colors).         */

  /* 8 bit per pixel */
  SHAPF_IND8     = 0x03, /**< Indexed 8 bit per pixel (256 colors).        */
  SHAPF_RGB233   = 0x13, /**< Direct RGB 8 bit per pixel.                  */
  SHAPF_GREY8    = 0x23, /**< Direct 8 bit intensity (grey level).         */
  SHAPF_ARGB2222 = 0x33, /**< Direct ARGB 8 bit per pixel.                 */

  /* 16 bit per pixel */
  SHAPF_RGB565   = 0x04, /**< Direct 16 bit per pixel R:5 G:6 B:5.         */
  SHAPF_ARGB1555 = 0x14, /**< Direct 16 bit per pixel A:1 R:5 G:5 B:5.     */
  SHAPF_ARGB4444 = 0x24, /**< Direct 16 bit per pixel A:4 R:4 G:4 B:4.     */
  SHAPF_AIND88   = 0x34, /**< Interlaced 8 alpha bit, 8 indexed color bir. */

  /* 32 bit per pixel */
  SHAPF_ARGB32   = 0x05, /**< Direct 32 bit per pixel A:8 R:8 G:8 B:8.     */
  SHAPF_UNKNOWN  = 0x00  /**< Reserved for error or unknown format.        */
 
} SHApixelFormat_e;

/** @name Image format macros.
 *  @ingroup dcplaya_devel
 *  @{
 **/

/** Get log 2 of number of bit per pixel. */
#define SHAPF_LOG2BPP(A)  ((int)(A)&7)
/** Get number of bit per pixel. */
#define SHAPF_BPP(A)      (1<<SHAPF_LOG2BPP(A))

/**@}*/

#endif //#define _SHATYPE_H_
