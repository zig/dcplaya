/** @ingroup dcplaya_fifo_devel
 *  @file    fifo.h
 *  @author  benjamin gerard
 *  @brief   PCM fifo with bak-buffer.
 *
 * $Id: fifo.h,v 1.7 2003-03-26 23:02:47 ben Exp $
 */

#ifndef _FIFO_H_
#define _FIFO_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_fifo_devel PCM fifo
 *  @ingroup  dcplaya_devel
 *  @brief    PCM fifo.
 *
 *  dcplaya fifo is a thread safe FIFO that include a back buffer capability
 *  in order to get the old PCM for sound analysis.
 *
 *  @author  benjamin gerard
 *  @{
 */

/** Initialize the fifo.
 *  @param  size  Number of sample in fifo (power of 2)
 *  @return error-code
 */
int fifo_init(int size);

/** Change the size of the fifo.
 *  @param  size  Number of sample in fifo (power of 2)
 *  @return size (may be unchanged on failure)
 */
int fifo_resize(int size);

/** Restart the fifo.
 */
int fifo_start(void);

/** Stop the fifo.
 */
void fifo_stop(void);

void fifo_read_lock(int *i1, int *n1, int *i2, int *n2);
void fifo_write_lock(int *i1, int *n1, int *i2, int *n2);
void fifo_unlock(void);

/** Get fifo free (writable) space.
 */
int fifo_free(void);

/** Get fifo used (readale) space.
 */
int fifo_used(void);

/** Get fifo size.
 */
int fifo_size(void);

/** Get bak-buffer size.
 */
int fifo_bak(void);

void fifo_state(int *r, int *w, int *b);
int fifo_fill();

/** Read stereo PCM from fifo.
 */
int fifo_read(int *buf, int n);

/** Read stereo PCM from fifo back-buffer.
 */
int fifo_readbak(int *buf, int n);

/** Write stereo PCM into fifo.
 */
int fifo_write(const int *buf, int n);

/** Write mono PCM into fifo.
 */
int fifo_write_mono(const short *buf, int n);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FIFO_H_ */
