/**
 * @ingroup  dcplaya_translator_devel
 * @file     SHAsysPeek.h
 * @author   benjamin gerard
 * @brief    Endianess independant memory access
 *
 * $Id: SHAsysPeek.h,v 1.2 2003-03-26 23:02:48 ben Exp $
 */

#ifndef _SHASYSPEEK_H_
#define _SHASYSPEEK_H_

#include "SHAsys/SHAsysTypes.h"

/** @name Unaligned little endian memory access
 *  @{
 */

/** Write in little endian.
 *  @param a address
 *  @param v value
 *  @return written value (v converted to written foramt)
 */
inline int SHApoke32(void *a, int v)
{
  0[(SHAuint8 *)a] = v;
  1[(SHAuint8 *)a] = v>>8;
  2[(SHAuint8 *)a] = v>>16;
  3[(SHAuint8 *)a] = v>>24;
  return (int)(SHAsint32)v;
}

/// Write 32 bit unsigned LE.
inline unsigned int SHApokeU32(void *a, int v)
{
  0[(SHAuint8 *)a] = v;
  1[(SHAuint8 *)a] = v>>8;
  2[(SHAuint8 *)a] = v>>16;
  3[(SHAuint8 *)a] = v>>24;
  return (unsigned int)(SHAuint32)v;
}

inline int SHApoke16(void *a, int v)
{
  0[(SHAuint8 *)a] = v;
  1[(SHAuint8 *)a] = v>>8;
  return (int)(SHAsint16)v;
}

inline unsigned int SHApokeU16(void *a, int v)
{
  0[(SHAuint8 *)a] = v;
  1[(SHAuint8 *)a] = v>>8;
  return (unsigned int)(SHAsint16)v;
}

inline int SHApeek32(const void *a)
{
  return (0[(SHAuint8 *)a]<<0)  +
         (1[(SHAuint8 *)a]<<8)  +
         (2[(SHAuint8 *)a]<<16) +
         ((int)3[(SHAsint8 *)a]<<24);
}

inline unsigned int SHApeekU32(const void *a)
{
  return (0[(SHAuint8 *)a]<<0)  +
         (1[(SHAuint8 *)a]<<8)  +
         (2[(SHAuint8 *)a]<<16) +
         (3[(SHAuint8 *)a]<<24);
}

inline int SHApeek16(const void *a)
{
  return (0[(SHAuint8 *)a]<<0)  +
         ((int)1[(SHAsint8 *)a]<<8);
}

inline int SHApeekU16(const void *a)
{
  return (0[(SHAuint8 *)a]<<0)  +
         (1[(SHAuint8 *)a]<<8);
}

inline unsigned int SHApokeU24(void *a, unsigned int v)
{
  0[(SHAuint8 *)a] = v;
  1[(SHAuint8 *)a] = v>>8;
  2[(SHAuint8 *)a] = v>>16;
  return v&0xFFFFFF;
}

inline int SHApoke24(void *a, int v)
{
  0[(SHAuint8 *)a] = v;
  1[(SHAuint8 *)a] = v>>8;
  2[(SHAuint8 *)a] = v>>16;
  const unsigned int shf = (sizeof(int)<<3) - 24;
  return (v << shf) >> shf;
}

inline unsigned int SHApeekU24(const void *a)
{
  return (0[(SHAuint8 *)a]<<0) +
         (1[(SHAuint8 *)a]<<8) +
         (2[(SHAuint8 *)a]<<16);
}

inline int SHApeek24(const void *a)
{
  return (0[(SHAuint8 *)a]<<0) +
         (1[(SHAuint8 *)a]<<8) +
         ((int)2[(SHAsint8 *)a]<<16);
}

///@}

#endif //#define _SHASYSPEEK_H_
