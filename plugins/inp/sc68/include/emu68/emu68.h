/**
 * @ingroup   emu68_devel
 * @file      emu68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/13
 * @brief     68K emulator user interface.
 * @version   $Id: emu68.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */
 
/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _EMU68_H_
#define _EMU68_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"
#include "emu68/getea68.h"
#include "emu68/macro68.h"
#include "emu68/error68.h"
#include "emu68/cc68.h"
#include "emu68/inst68.h"
#include "emu68/mem68.h"


/** @name  EMU68 internal struct access */
/*@{*/

/** EMU68 internal 68K register set structure */
extern reg68_t reg68;

/** Set new interrupt IO
 *
 *     This version of EMU68 was specially build for SC68. For optimization
 *     purposes only one IO plugged chip could interrupt processor.
 *
 *  @param  io  Pointer to the only IO that could eventually interrupt.
 *
 *  @return  Pointer to previous interrupt IO.
 */
io68_t *EMU68_set_interrupt_io(io68_t *io);

/** Copy specified register set to EMU68 internal register set */
void EMU68_set_registers(reg68_t *r);

/** Copy EMU68 internal register set to specified register set */
void EMU68_get_registers(reg68_t *r);

/** Set EMU68 internal cycle counter */
void EMU68_set_cycle(u32 cycle);

/** Get EMU68 internal cycle counter */
u32 EMU68_get_cycle(void);

/*@}*/


/** @name  Init functions */
/*@{*/

/** 68K Hardware Reset
 *
 *    Perform following operations :
 *    - PC = 0
 *    - SR = 2700
 *    - A7 = end of mem - 4
 *    - All registers cleared
 *    - All IO reseted
 */
void EMU68_reset(void);

/** First time init.
 *
 *   Initialize EMU68. The maxmem parameter is not used to allocate memory.
 *   The function checks if the value is valid. It must be a power of 2 in
 *   the range [2^17..2^24], 128Kb to 16 Mb.
 *
 *  @param  maxmem  68K memory amount in byte (power of 2 only).
 *
 *  @return error-code
 *  @retval  0  Success
 *  @retval <0  Failure
 */
int EMU68_init(u32 maxmem);

/** Clean exit.
 */
void EMU68_kill(void);

/*@}*/


/** @name  EMU68 on-board memory access */
/*@{*/

/** Check if a memory block is in 68K on-board memory range.
 *
 *  @return error-code
 *  @retval  0  Success
 *  @retval <0  Failure
 */
int EMU68_memvalid(u32 dest, u32 sz);

/** Get byte in 68K onboard memory.
 *
 *  @see EMU68_poke()
 */
u8 EMU68_peek(u32 addr);

/** Put a byte in 68K onboard memory.
 *
 *  @see EMU68_peek()
 */
u8 EMU68_poke(u32 addr, u8 v);

/** Put a memory block to 68K on-board memory.
 *
 *    The function copy a memory block in 68K on-board memory after verifying
 *    that the operation access valid 68K memory.
 *
 *  @see EMU68_memget()
 *  @see EMU68_memvalid()
 */
int EMU68_memput(u32 dest, u8 *src, u32 sz);

/** Get 68K on-board memory into a memory block.
 *
 *    The function copy a 68K on-board memory to a memory location after
 *    verifying that the operation access valid 68K memory.
 *
 *  @see EMU68_memput()
 *  @see EMU68_memvalid()
 */
int EMU68_memget(u8 *dest, u32 src, u32 sz);

/*@}*/


/** @name  Execution functions */
/*@{*/

/** Execute one instruction. */
void EMU68_step(void);

/** Execute until RTS.
 *
 *   This function runs an emulation loop until stack address becomes higher
 *   than its value at start. After what, the interruption are tested and
 *   executed for this pass, with an execution time given in parameter
 *   whatever the time passed in the execution loop. This function is very
 *   specific to SC68 implementation.
 */
void EMU68_level_and_interrupt(u32 cycleperpass);

/** Execute for given number of cycle
 */
void EMU68_cycle(u32 cycleperpass);

/** Execute until PC reachs breakpoint.
 *
 *   @param  breakpc  Breakpoint location
 */
void EMU68_break(u32 breakpc);

/*@}*/


/** @name  Version checking functions */
/*@{*/

#ifdef EMU68DEBUG

/** Dummy function for version checking
 *
 *   Include a call to the EMU68_debugcompiled()0 function to prevent
 *   program that needs EMU68 debug special features to be linked with bad
 *   version library.
 *
 *  @see EMU68_sizeof_reg68()
 */
void EMU68_debugcompiled(void);

#endif

/** Get size of reg68_t register set.
 *
 *   The EMU68_sizeof_reg68() function returns the size of reg68_t structure
 *   at compile time. As the size of this structure changes when EMU68 is
 *   compiled in debug mode (EMU68DEBUG defined at compile time), it is
 *   useful to prevent bad library linkage errors.
 *
 * @see EMU68_debugcompiled()
 */
unsigned EMU68_sizeof_reg68(void);

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /* End of file emu68.h */

