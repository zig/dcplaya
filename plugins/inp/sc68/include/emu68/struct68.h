/**
 * @ingroup   emu68_devel
 * @file      struct68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      13/03/1999
 * @brief     Struture definitions.
 * @version   $Id: struct68.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _STRUCT68_H_
#define _STRUCT68_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/type68.h"

/** IO no pending interruption return value.
 *
 *    The next_int function of IO plugin must return IO68_NO_INT when no
 *    interruption are expected.
 */
#define IO68_NO_INT (0x80000000)

#ifndef NULL
# define NULL 0L
#endif

/** Mask for memory overflow.
 */
#define MEM68MSK ((1<<19)-1)   /* 512 Kb memory */
/* #define MEM68MSK (reg68.memmsk) */

/** @name  Memory access caller type */
/*{@*/

/** Read memory function type */
typedef u32  (*memrfunc68_t)(u32 addr, u32 cycle);
/** Write memory function type */
typedef void (*memwfunc68_t)(u32 addr, u32 value, u32 cycle);

/*@}*/

/** First level (16 lines) decoder function type */
typedef void (linefunc68_t)(u32, u32);

/**  68K interruption exception structure.
 */
typedef struct
{
  u32 vector;                  /**< Interrupt vector */
  u32 level;                   /**< Interrupt level */
} int68_t;

/**  IO emulator pluggin structure.
 *
 *      All 68K IO must have a filled io68_t structure to be warm plug or
 *      unplug with ioplug interface.
 *
 */
typedef struct _io68_t
{
  struct _io68_t *next;        /**< Pointer to next IO in plugged IO list */
  char name[32];               /**< IO name */
  u32 addr_low;                /**< IO mapping area start address (low) */
  u32 addr_high;               /**< IO mapping area end adress (high) */
  memrfunc68_t Rfunc[3];       /**< IO read function table (addr, cycle) */
  memwfunc68_t Wfunc[3];       /**< IO write function(addr, value, cycle) */
  int68_t *(*interupt)(u32);   /**< IO interruption function claim */
  u32 (*next_int)(u32);        /**< IO get next interruption cycle */
  void (*adjust_cycle)(u32);   /**< IO adjust cycle */
  u32 (*reset)(void);          /**< IO reset function */
  u32  rcycle_penalty;         /**< Read cycle penalty */
  u32  wcycle_penalty;         /**< Write cycle penalty */
} io68_t;

/** 68K emulator registers, memory and IO.
 */
typedef struct
{
  s32 d[8];                    /**< 68000 data registers */
  s32 a[8];                    /**< 68000 address registers */
  s32 usp;                     /**< 68000 User Stack Pointers */
  s32 pc;                      /**< 68000 Program Counter */
  u32 sr;                      /**< 68000 Status Register */

  u32 cycle;                   /**< Internal cycle counter */

  u8  *mem;                    /**< Memory buffer */
#ifdef EMU68DEBUG
  u8  *chk;                    /**< Access-Control-Memory buffer */
  u32 framechk;                /**< ORed chk change for current frame */
#endif
  u32 memsz;                   /**< Memory size in byte */
  u32 memmsk;                  /**< Memory mask for memory access modulo */

  u32 nio;                     /**< # IO plug in IO-list */
  io68_t *iohead;              /**< Head of IO-list */

  int status;

} reg68_t;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _STRUCT68_H_ */
