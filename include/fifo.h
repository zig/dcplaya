/** @ingroup dcplaya_devel
 *  @file    fifo.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @brief   PCM fifo with bak-buffer.
 *
 * $Id: fifo.h,v 1.5 2003-03-17 15:36:21 ben Exp $
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
 */

/** Initialize the fifo.
 *  @ingroup  dcplaya_fifo_devel
 *  @param  size  Number of sample in fifo (power of 2)
 *  @return error-code
 */
int fifo_init(int size);

/** Change the size of the fifo.
 *  @ingroup  dcplaya_fifo_devel
 *  @param  size  Number of sample in fifo (power of 2)
 *  @return size (may be unchanged on failure)
 */
int fifo_resize(int size);

/** Restart the fifo.
 *  @ingroup  dcplaya_fifo_devel
 */
int fifo_start(void);

/** Stop the fifo.
 *  @ingroup  dcplaya_fifo_devel
 */
void fifo_stop(void);

void fifo_read_lock(int *i1, int *n1, int *i2, int *n2);
void fifo_write_lock(int *i1, int *n1, int *i2, int *n2);
void fifo_unlock(void);

/** Get fifo free (writable) space.
 *  @ingroup  dcplaya_fifo_devel
 */
int fifo_free(void);

/** Get fifo used (readale) space.
 *  @ingroup  dcplaya_fifo_devel
 */
int fifo_used(void);

/** Get fifo size.
 *  @ingroup  dcplaya_fifo_devel
 */
int fifo_size(void);

/** Get bak-buffer size.
 *  @ingroup  dcplaya_fifo_devel
 */
int fifo_bak(void);

void fifo_state(int *r, int *w, int *b);
int fifo_fill();
int fifo_read(int *buf, int n);
int fifo_readbak(int *buf, int n);
int fifo_write(const int *buf, int n);
int fifo_write_mono(const short *buf, int n);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FIFO_H_ */
