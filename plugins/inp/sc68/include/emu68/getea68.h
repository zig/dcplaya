/**
 * @ingroup   emu68_devel
 * @file      getea68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      13/03/1999
 * @brief     68k effective address calculation function table.
 * @version   $Id: getea68.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 *
 *   The get_ab[bwl] tables are used by EMU68 to calculate operands
 *   effective address and are indexed by operand addressing mode. The
 *   called function do everything neccessary to update processor state like
 *   address register increment or decrement. Function parameter is register
 *   number coded in three bit 0 to 2 of 68k opcode. When the mode is 7,
 *   register parameter is used as an index in a second level function table
 *   for extended addressing mode.
 *
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _GETEA68_H_
#define _GETEA68_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/type68.h"

/** Byte operand effective address calculation function table */
extern u32 (*get_eab[8])(u32 reg);

/** Word operand effective address calculation function table */
extern u32 (*get_eaw[8])(u32 reg);

/** Long operand effective address calculation function table */
extern u32 (*get_eal[8])(u32 reg);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _GETEA68_H_ */

