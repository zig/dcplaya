/**
 * @ingroup   io68_devel
 * @file      mwemul.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/20
 * @brief     MicroWire - STE sound emulator
 * @version   $Id: mwemul.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _MWEMUL_H_
#define _MWEMUL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "emu68/type68.h"

/** @name  Micro-Wire registers.
 *  @{
 */

#define MW_ACTI 0x01        /**< Microwire enabled */

#define MW_BASH 0x03        /**< Microwire sample start address, bit 16-23 */
#define MW_BASM (MW_BASH+2) /**< Microwire sample start address, bit 8-15  */
#define MW_BASL (MW_BASH+4) /**< Microwire sample start address, bit 0-7   */

#define MW_CTH 0x09         /**< Microwire sample counter, bit 16-23 */
#define MW_CTM (MW_CTH+2)   /**< Microwire sample counter, bit 8-15  */
#define MW_CTL (MW_CTH+4)   /**< Microwire sample counter, bit 0-7   */

#define MW_ENDH 0x0f        /**< Microwire sample end address, bit 16-23 */
#define MW_ENDM (MW_ENDH+2) /**< Microwire sample end address, bit 8-15  */
#define MW_ENDL (MW_ENDH+4) /**< Microwire sample end address, bit 0-7   */

#define MW_MODE 0x21        /**< Microwire playing mode */

#define MW_DATA 0x22        /**< Microwire data register */
#define MW_CTRL 0x24        /**< Microwire control register */

/*@}*/


/** @name  Micro-Wire internal data.
 *  @{
 */

extern u8 mw[0x40]; /**< Micro-Wire internal register data */
extern u32 mw_ct;   /**< Micro-Wire internal sample counter */
extern u32 mw_end;  /**< Micro-Wire internal sample end location */

/*@}*/


/** @name  Initialization functions.
 *  @{
 */

/** Set output buffer replay frequency.
 *
 *    The MW_set_replay_frq() function set Micro-Wire emulator output PCM
 *    buffer frequency at f Hz.
 *
 *  @param  f  Output PCM buffer frequency in Hz in the range [8000..48000]
 *
 *  @return Real frequency used (could be different than parameter).
 */
u32 MW_set_replay_frq(u32 f);

/** Micro-Wire hardware reset.
 *
 *    The MW_reset() reset function perform a Micro-Wire reset. It performs
 *    following operations :
 *    - all registers zeroed
 *    - all internal counter zeroed
 *    - LMC reset
 *      - mixer mode YM2149+Micro-Wire
 *      - master volume to -40db
 *      - left and right volumes to -20db
 *      - low-pass filter to 0
 *
 *    @return error-code (always success)
 *    @retval 0  Success
 */
int MW_reset(void);

/** Micro-Wire first one first initialization.
 *
 *    The MW_init() must be call before all other PL functions. It performs following operations.
 *    - Init output level (volume) table.
 *    - Hardware reset
 *    - Set replay frequency to default (44100 Hz)
 *
 *    @return error-code (always success)
 *    @retval 0  Success
 *
 *  @see MW_reset()
 */
int MW_init( void );

/** @name  Emulation functions.
 *  @{
 */

/** Execute Micro-Wire emulation.
 *
 *    The MW_mix() function processes sample mixing with current internal
 *    parameters for n samples. Mixed samples are stored in a large enough
 *    (at least n) 32 bit buffer pointed by b. This buffer have to be
 *    previously filled with the YM-2149 samples. Typically it is the YM-2149
 *    emulator output buffer. This allows Micro-Wire emulator to honnor the
 *    LMC mixer mode.iven LMC mode. This porocess include the mono to stereo
 *    expansion. The mem68 starting pointer locates the 68K memory buffer
 *    where samples are stored to allow DMA fetch emulation.
 *
 *    @param  b      Pointer to YM-2149 source sample directly used for
 *                   Micro-Wire output mixing.
 *    @param  mem68  Pointer to 68K memory buffer start address
 *    @param  n      Number of sample to mix in b buffer
 *
 *    @see YM_mix()  @see YM_get_buffer()
 *    
 *    @todo   Stereo mode is a hack !
 */
void MW_mix( u32 *b, u8 *mem68, u32 n );

/*@}*/


/** Micro-Wire LMC control functions.
 *  @{
 */

/** Set LMC mixer type.
 *
 *   The MW_set_LMC_mixer() function choose the mixer mode :
 *   - 0  -12 Db
 *   - 1  YM+STE
 *   - 2  STE only
 *   - 3  reserved ???
 *
 *   @param n  New mixer mode (see above)
 */
void MW_set_LMC_mixer(u32 n);

/** Set LMC master volume.
 *
 *  @param  n  New volume in range [0..40]=>[-80Db..0Db]
 *
 *  @see MW_set_LMC_left()
 *  @see MW_set_LMC_right()
 */
void MW_set_LMC_master(u32 n);

/** Set LMC left channel volume.
 *
 *    Set LMC left channel volume in decibel.
 *
 *    @param  n  New volume in range [0..20]=>[-40Db..0Db]
 *
 *  @see MW_set_LMC_master()
 *  @see MW_set_LMC_right()
 */
void MW_set_LMC_left(u32 n);

/** Set LMC right channel volume.
 *
 *    @param  n  New volume in range [0..20]=>[-40Db..0Db]
 *
 *  @see MW_set_LMC_master()
 *  @see MW_set_LMC_left()
 */
void MW_set_LMC_right( u32 n );

/** Set high pass filter.
 *
 *  @param  n  New high pass filter [0..12]=>[-12Db..0Db]
 *
 *  @see MW_set_LMC_low()
 *
 *  @warning  Filters are not supported by MicroWire emulator.
 */
void MW_set_LMC_high(u32 n);

/** Set low pass filter.
 *
 *  @param  n  New low pass filter [0..12]=>[-12Db..0Db]
 *
 *  @see MW_set_LMC_high()
 *
 *  @warning  Unsupported by MicroWire emulator.
 */
void MW_set_LMC_low(u32 n);

/*@}*/


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _MWEMUL_H_*/
