/**
 * @ingroup dcplaya_devel
 * @file    pcm_buffer.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002
 * @brief   PCM and Bitstream buffer.
 *
 * $Id: pcm_buffer.h,v 1.3 2003-03-17 15:36:21 ben Exp $
 */

#ifndef _PCM_BUFFER_H_
#define _PCM_BUFFER_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_pcmbuffer_devel PCM and Bitstream buffers
 *  @ingroup  dcplaya_devel
 *  @brief    PCM and Bitstream buffers.
 *  @warning  Not thread safe.
 */

/** Current PCM buffer size (in mono PCM).
 *  @ingroup  dcplaya_pcmbuffer_devel
 */ 
#define PCM_BUFFER_SIZE pcm_buffer_size

/** Current Bitstream buffer size (in bytes).
 *  @ingroup  dcplaya_pcmbuffer_devel
 */ 
#define BS_SIZE         bs_buffer_size

/** Current PCM buffer.
 *  @ingroup  dcplaya_pcmbuffer_devel
 */ 
extern short * pcm_buffer;

/** Current PCM buffer size (in mono PCM).
 *  @ingroup  dcplaya_pcmbuffer_devel
 */ 
extern int pcm_buffer_size;

/** Current Bitstream buffer.
 *  @ingroup  dcplaya_pcmbuffer_devel
 */ 
extern char  * bs_buffer;

/** Current Bitstream buffer size (in bytes).
 *  @ingroup  dcplaya_pcmbuffer_devel
 */ 
extern int bs_buffer_size;

/** Initialise PCM and Bitstream buffer.
 *  @ingroup  dcplaya_pcmbuffer_devel
 *
 *    The pcm_buffer_init() function allocates realloc PCM and Bitstream
 *    buffers. For each buffer the reallocation occurs only if the new size
 *    is greater than the current. If the function failed, old buffer are
 *    preserved.
 *
 * @param  pcm_size  Number of mono PCM to alloc for pcm_buffer.
 *                   0:kill buffer -1:default size -2:Don't change.
 * @param  bs_size   Number of bytes to alloc for bs_buffer.
 *                   0:kill buffer -1:default size -2:Don't change.
 *
 * @return error-code
 * @retval  0 success
 * @retval <0 failure
 *
 * @warning In case of failure you need to read the pcm_buffer_size and
 *          bs_buffer_size since it is posible that the function failed
 *          after reallocting only one of this buffers
 */ 
int pcm_buffer_init(int pcm_size, int bs_size);

/** Shutdown PCM and Bitstream buffer.
 *  @ingroup  dcplaya_pcmbuffer_devel
 *
 *    The pcm_buffer_shutdown() is an alias for the pcm_buffer_init(0,0) call.
 */
void pcm_buffer_shutdown(void);

DCPLAYA_EXTERN_C_END

#endif
