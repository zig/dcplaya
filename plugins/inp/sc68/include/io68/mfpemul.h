/**
 * @ingroup   io68_devel
 * @file      mfpemul.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MFP-68901 emulator
 * @version   $Id: mfpemul.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MFPEMUL_H_
#define _MFPEMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

#define TIMER_A   0   /**< MFP timer 'A' */
#define TIMER_B   1   /**< MFP timer 'B' */
#define TIMER_C   2   /**< MFP timer 'C' */
#define TIMER_D   3   /**< MFP timer 'D' */

/** MFP shadow register array. */
extern u8 mfp[0x40];

/** MFP reset.
 */
u32 MFP_reset(void);

/** MFP init.
 */
int MFP_init(void);

/** MFP get Timer Data register.
 */
u8 MFP_getTDR(u32 timer, unsigned int cycle);

/** MFP write Timer data register.
 */
void MFP_putTDR(u32 timer, u8 v, unsigned int cycle);

/** MFP write Timer control register.
 */
void MFP_putTCR(u32 timer, u8 v, unsigned int cycle);

/** Is MFP generate an interruption ???
 */
int68_t *MFP_interrupt( unsigned int cycle );

/** When MFP generates an interruption ???
 */
u32 MFP_nextinterrupt(unsigned int cycle);

/** Change cycle count base.
 */
void MFP_subcycle(unsigned int subcycle);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MFPEMUL_H_ */
