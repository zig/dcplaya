/**
 * @ingroup   emu68_devel
 * @file      type68.h
 * @brief     Type definitions.
 * @date      13/03/1999
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: type68.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 *
 *   These types are used by EMU68 and SC68 related projects. Some uses are
 *   not neccessary and should be remove to improve execution time on 64 bit
 *   or more platform. Currently EMU68 will probably not run correctly if
 *   these types are not correct. It is a limitation to EMU68 portability.
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _TYPE68_H_
#define _TYPE68_H_

#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef u32
typedef unsigned int u32;   /**< Must be an unsigned 32 bit integer */
typedef signed int s32;     /**< Must be a signed 32 bit integer */
#endif

#ifndef u16
typedef unsigned short u16; /**< Must be an unsigned 16 bit integer */
typedef signed short s16;   /**< Must be a signed 16 bit integer */
#endif

#ifndef u8
typedef unsigned char u8;   /**< Must be an unsigned 8 bit integer */
typedef signed char s8;     /**< Must be a signed 8 bit integer */
#endif

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _TYPE68_H_ */


