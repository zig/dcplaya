/**
 * @ingroup   io68_devel
 * @file      paulaemul.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1998/07/18
 * @brief     Paula emulator (Amiga soundchip)
 * @version   $Id: paulaemul.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _PAULA_EMUL_H_
#define _PAULA_EMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/struct68.h"

/** Amiga Vertical/Horizontal electron bean position */
#define PAULA_VHPOSR    0x06

/** @name  Amiga interruption registers
 *
 *   Interrupt Request & Enable register bits :
 *   - bit 7   Audio channel A
 *   - bit 8   Audio channel B
 *   - bit 9   Audio channel C
 *   - bit 10  Audio channel D
 *   - bit 14  Master interrupt
 *
 *  @{
 */

#define PAULA_INTREQR   0x1E  /**< Interruption request read */
#define PAULA_INTREQRH  0x1E  /**< Interruption request read MSB */
#define PAULA_INTREQRL  0x1F  /**< Interruption request read LSB */

#define PAULA_INTREQ    0x9C  /**< Interruption request write */
#define PAULA_INTREQH   0x9C  /**< Interruption request write MSB */
#define PAULA_INTREQL   0x9D  /**< Interruption request write LSB */

#define PAULA_INTENAR   0x1C  /**< Interruption enable read */
#define PAULA_INTENARH  0x1C  /**< Interruption enable read MSB */
#define PAULA_INTENARL  0x1D  /**< Interruption enable read LSB */

#define PAULA_INTENA    0x9A  /**< Interruption enable write */
#define PAULA_INTENAH   0x9A  /**< Interruption enable write MSB */
#define PAULA_INTENAL   0x9B  /**< Interruption enable write LSB */

/*@}*/


/** @name  Amiga DMA registers.
 *  @{
 *
 *   Amiga DMA control register bits :
 *   - bit 0  Audio DMA channel A
 *   - bit 1  Audio DMA channel B
 *   - bit 2  Audio DMA channel C
 *   - bit 3  Audio DMA channel D
 *   - bit 9  General DMA
 */

#define PAULA_DMACONR   0x02  /**< DMA control read */
#define PAULA_DMACONRH  0x02  /**< DMA control read MSB */
#define PAULA_DMACONRL  0x03  /**< DMA control read LSB */

#define PAULA_DMACON    0x96  /**< DMA control write */
#define PAULA_DMACONH   0x96  /**< DMA control write MSB */
#define PAULA_DMACONL   0x97  /**< DMA control write LSB */

/*@}*/


/** @name  Amiga Paula registers.
 *  @{
 */

#define PAULA_VOICE(I) ((0xA+(I))<<4) /**< Paula channel #I register base */
#define PAULA_VOICEA   0xA0           /**< Paula channel A register base */
#define PAULA_VOICEB   0xB0           /** Paula channel B register base */
#define PAULA_VOICEC   0xC0           /**< Paula channel C register base */
#define PAULA_VOICED   0xD0           /**< Paula channel D register base */

/*@}*/


/** @name  Amiga Paula frequencies (PAL).
 *  @{
 */

#define PAULA_PER 2.79365E-7      /**< Paula period (1 cycle duration) */
#define PAULA_FRQ 3579610.53837   /**< Paula frequency (1/PAULA_PER) */

/*@}*/


/** Counter fixed point precision 13+19(512kb)=>32 bit */
#define PAULA_CT_FIX		13


/** @name  Internal Paula emulation data.
 *  @{
 */

/** Paula voice information data structure. */
typedef struct
{
  u32 adr;   /**< current sample counter (<<PAULA_CT_FIX) */
  u32 start; /**< loop address */
  u32 end;   /**< end address (<<PAULA_CT_FIX) */

/*  int v[16]; //$$$ */

} paulav_t;

extern u8 paula[];        /**< Paula regiter data storage */
extern paulav_t paulav[]; /**< Paula voices(channel) table (4 voices) */
extern int paula_dmacon;  /**< Internal DMA control register copy */
extern int paula_intena;  /**< Internal interrupt enable register copy */
extern int paula_intreq;  /**< Internal interrupt request register copy */

/*@}*/


/** @name  Initialization functions */
/*@{*/

/** Set output buffer replay frequency
 *
 *    The PL_set_replay_frq() function set Paula emulator output PCM buffer
 *    frequency at f Hz.
 *
 *  @param  f  Output PCM buffer frequency in Hz in the range [8000..48000]
 *
 *  @return Real frequency used (could be different of entry).
 */
u32 PL_set_replay_frq(u32 f);

/** Paula hardware reset.
 *
 *    The PL_reset() reset function perform a Paula reset. It performs
 *    following operations :
 *    - all registers zeroed
 *    - all internal voices set to dummy 2 samples len address.
 *    - general DMA enabled
 *    - all audio DMA disabled
 *    - interrupt master enabled
 *    - all audio interrupt disbled
 *
 *    @return error-code (always success)
 *    @return  0  Success
 */
int PL_reset(void);

/** Paula first one first initialization.
 *
 *    The PL_init() must be call before all other PL functions. It performs following operations.
 *    - Init output level (volume) table.
 *    - Hardware reset
 *    - Set replay frequency to default (44100 Hz)
 *
 *    @return error-code (always success)
 *    @return  0  Success
 *
 *  @see PL_reset()
 */
int PL_init(void);

/** @name  Emulation functions */
/*@{*/

/** Execute Paula emulation.
*
*    The PL_mix() function processes sample mixing with current internal
*    parameters for n samples. Mixed samples are stored in a large enough
*    (at least n) 32 bit buffer pointed by b. The mem68 starting and
*    mem68end ending pointer locate the 68K memory buffer where Amiga sample
*    are stored and allow DMA fetch emulation to be safe. The Paula emulator
*    don't care about Amiga chip/fast memory organization.
*
*    @param  b         Pointer to destination 32-bit data buffer
*    @param  mem68     Pointer to 68K memory buffer start address
*    @param  mem68end  Pointer to 68K memory buffer end address
*    @param  n         Number of sample to mix in b buffer
*
*  @warning  Currently mem68end seems to be unused.
*/
void PL_mix(u32 *b, u8 *mem68, u8 *mem68end, u32 n);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PAULA_EMUL_H_ */
