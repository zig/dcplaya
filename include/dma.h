/**
 * @ingroup  dcplaya_dma
 * @file     dma.h
 * @author   vincent penne
 * @date     2004/07/27
 * @brief    dma support
 * 
 * $Id: dma.h,v 1.1 2004-07-31 22:56:46 vincentp Exp $
 */

#ifndef _DMA_H_
#define _DMA_H_

#include <kos/sem.h>

typedef struct dma_chain {
  struct dma_chain * next;

  semaphore_t * sema;

  uint8 * src;
  uint32 dest;
  uint32 count;
  int type;

  kthread_t * waiting_thd;
} dma_chain_t;


#define DMA_TYPE_VRAM    0
#define DMA_TYPE_TA      1
#define DMA_TYPE_SPU     2
#define DMA_TYPE_BBA_RX  3
#define DMA_TYPE_BBA_TX  4

/** initiate a dma transfer 
 *
 * @param type type of transfer
 *
 * @return pointer on a dma_chain_t object that you can wait on
 * with dma_wait.
 *
 */
dma_chain_t * dma_initiate(void * dest, void * src, uint32 length,
			   int type);

/** wait for a dma transfer completion and free the structure */
int dma_wait(dma_chain_t * chain);

/** free a dma transfer structure without waiting */
int dma_free(dma_chain_t * chain);

/** Flush the sh4 write cache in the given area , taking care
 *  of the cache size. If len is greater than the cache size, then
 *  only the end of the given buffer will be flushed, assuming
 *  that it had been written from beginning to end. */
void dma_cache_flush(void * ptr, int len);

#endif /* ifndef _DMA_H_ */
