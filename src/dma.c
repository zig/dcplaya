/**
 * @ingroup  dcplaya_dma
 * @file     dma.c
 * @author   vincent penne
 * @date     2004/07/27
 * @brief    dma support
 * 
 * $Id: dma.c,v 1.1 2004-07-31 22:56:46 vincentp Exp $
 */


#include <malloc.h>
#include <assert.h>
#include "dc/ta.h"
#include <dc/pvr.h>
#include <dc/spu.h>
#include <arch/cache.h>
#include <dc/asic.h>
#include "dma.h"
#include "console.h"
#include "draw/ta.h"

#define vid_border_color(r, g, b) (void)0 /* nothing */

/* maximal size of cache flushing we need to do */
#define CACHE_SZ (16*1024)

/* from libs/ta/ta.c */
extern int ta_block_render;
extern void (* ta_render_done_cb) ();

static dma_chain_t * spu_chain_head;
static dma_chain_t * chain_head;

static int spu_transfering;
int dma_transfering;

int bba_dma_mode;

int (*old_printk_func)(const uint8 *data, int len, int xlat);
#define printf(a) (void) 0 // old_printk_func(a, strlen(a), 0)

//#define printf(a) csl_putstring(csl_main_console, a)

#define SPU_MAX_SZ (128*1024)

static void renderdone_cb();


#define BUFSZ 8
static dma_chain_t buf[BUFSZ];
static dma_chain_t * free_head;
static int ibuf;
static spinlock_t mutex;


static dma_chain_t * alloc_chain()
{
  static int init;
  dma_chain_t * res;

  spinlock_lock(&mutex);

  if (!init) {
    int i;
    for (i=0; i<BUFSZ; i++) {
      buf[i].sema = sem_create(0);
      buf[i].next = (i<BUFSZ-1)? buf+i+1 : NULL;
    }
    free_head = buf;
    init = 1;
  }

  res = free_head;
  if (res) {
    free_head = res->next;
    res->next = NULL;
  } else
    panic("running out of dma chain structure !\n");
  spinlock_unlock(&mutex);
  return res;
}

static void free_chain(dma_chain_t * chain)
{
  spinlock_lock(&mutex);
  chain->next = free_head;
  free_head = chain;
  spinlock_unlock(&mutex);
}



static void spu_cb(void * p)
{
  if (p) {
    dma_chain_t * chain = (dma_chain_t *) p;

    if (chain->src == NULL) {
      free_chain(chain);
    } else {
      sem_signal(chain->sema);
      if (chain->waiting_thd)
	thd_schedule_next(chain->waiting_thd);
      chain->dest = 0; /* mark dma completed */
    }
    
    if (spu_chain_head == NULL)
      ;//draw_unlock();

    vid_border_color(0, 0, 0);
    spu_transfering = 0;
  } else
    ;//bba_lock();
    //asic_evt_disable(ASIC_EVT_EXP_PCI, ASIC_IRQB);

  if (spu_chain_head) {
    dma_chain_t * chain = spu_chain_head;

    spu_transfering = 1;

    vid_border_color(0, 255, 0);
    switch(chain->type) {
/*     case DMA_TYPE_VRAM: */
/*       pvr_txr_load_dma(chain->src, chain->dest,  */
/* 		       chain->count, 0, spu_cb, chain); */
/*       break; */
    case DMA_TYPE_SPU:
      dma_cache_flush(chain->src, chain->count);
      spu_dma_transfer(chain->src, chain->dest, chain->count, 0, 
		       spu_cb, chain);
      break;

    case DMA_TYPE_BBA_RX:
      g2_dma_transfer(chain->dest, chain->src, chain->count, 0, 
		      spu_cb, chain, 1, bba_dma_mode, 1, 3);
      break;
    default:
      panic("got bad dma chain type in spu_cb\n");
    }
    spu_chain_head = chain->next;
  } else {
    //asic_evt_enable(ASIC_EVT_EXP_PCI, ASIC_IRQB);
    ;//bba_unlock();
  }
}

static void dma_cb(void * p)
{
  if (p) {
    dma_chain_t * chain = (dma_chain_t *) p;

    if (chain->count > 0) {
/*       vid_border_color(0, 0, 0); */
/*       ta_block_render = 0; */
/*       ta_render_done_cb = renderdone_cb; */
/*       return; */
    } else {

      vid_border_color(0, 255, 0);
    
      printf("\n!!!!!! dma done !\n");
      if (chain->sema == NULL) {
	free_chain(chain);
      } else {
	sem_signal(chain->sema);
	chain->dest = 0; /* mark dma completed */
      }

      dma_transfering = 0;
    }
  }

  if (chain_head) {
    dma_chain_t * chain = chain_head;
    ta_block_render = 1;

    static int toto;
    toto += 64;
    vid_border_color(255, toto&255, toto&255);
    dma_transfering = 1;

    printf("\n!!!!!! dma transfer ! \n");
    switch(chain->type) {
    case DMA_TYPE_VRAM:
      pvr_txr_load_dma(chain->src, chain->dest, 
		       chain->count, 0, dma_cb, chain);
      break;
/*     case DMA_TYPE_SPU: */
/*       if (chain->count > SPU_MAX_SZ) { */
/* 	//panic("chain->count > SPU_MAX_SZ\n"); */
/* 	dma_cache_flush(chain->src, chain->count); */
/* 	spu_dma_transfer(chain->src, chain->dest, SPU_MAX_SZ, 0,  */
/* 			 (spu_dma_callback_t) dma_cb,  */
/* 			 (ptr_t) chain); */
/* 	chain->count -= SPU_MAX_SZ; */
/* 	chain->dest += SPU_MAX_SZ; */
/* 	chain->src += SPU_MAX_SZ; */
/* 	return; */
/*       } else { */
/* 	spu_dma_transfer(chain->src, chain->dest, chain->count,  */
/* 			 0,  */
/* 			 (spu_dma_callback_t) dma_cb,  */
/* 			 (ptr_t) chain); */
/*       } */
      
/*       break; */
    default:
      assert(0 && "got bad dma chain type in dma_cb");
    }

    chain->count = 0;
    chain_head = chain->next;

  } else {
    printf("\n!!!!!! all dma done !\n");
    ta_block_render = 0;
    vid_border_color(0, 0, 0);
  }
}

static void renderdone_cb()
{
  printf("\n!!!!! renderdone_cb !\n");
  vid_border_color(255, 0, 0);
  if (!ta_block_render) {
    dma_cb(0);
  }
  ta_render_done_cb = NULL;
}

static int nospudma = 0;
dma_chain_t * dma_initiate(void * dest, void * src, uint32 length,
			   int type)
{
  int irq = 0;

  dma_chain_t * chain = alloc_chain();

  if (chain == NULL)
    return NULL;

  chain->dest = (uint32) dest;
  chain->src = (uint8 *) src;
  chain->count = length;
  chain->type = type;
  chain->waiting_thd = 0;

  if (type == DMA_TYPE_SPU || type == DMA_TYPE_BBA_RX) {
    if (type == DMA_TYPE_SPU && (0||nospudma)) {
	vid_border_color(0, 0, 255);
      spu_memload(chain->dest, chain->src, chain->count);
	vid_border_color(0, 0, 0);
      sem_signal(chain->sema);
      return chain;
    } else {
      //draw_lock();
      vid_border_color(0, 0, 255);

      if (!irq_inside_int())
	irq = irq_disable();

      if (type == DMA_TYPE_BBA_RX) {
/* 	if (spu_chain_head && spu_chain_head->type == DMA_TYPE_SPU) { */
/* 	  if (!irq_inside_int()) */
/* 	    irq_restore(irq); */
/* 	  free_chain(chain); */
/* 	  return NULL; */
/* 	} */
	chain->waiting_thd = thd_current;
      }

      chain->next = spu_chain_head;
      spu_chain_head = chain;

      if (!spu_transfering)
	spu_cb(0);

      if (!irq_inside_int())
	irq_restore(irq);

      return chain;
    }
  } else
    nospudma = 0;

  if (!irq_inside_int())
    irq = irq_disable();
  chain->next = chain_head;
  chain_head = chain;
  ta_render_done_cb = renderdone_cb;
  //vid_border_color(0, 0, 255);
  if (!irq_inside_int())
    irq_restore(irq);

  return chain;
}

//#undef printf
int dma_wait(dma_chain_t * chain)
{
  if (chain == NULL)
    return -1;

  if (irq_inside_int()) {
    panic("dma_wait called inside an irq !\n");
    dma_free(chain);
    return -1;
  }

  printf("\n !!!!!!!!!! waiting ... \n");
  sem_wait(chain->sema);
/*   if (sem_wait_timed(chain->sema, 1000)) { */
/*     printf("dma timeout...\n"); */
/*     dma_free(chain); */
/*     return -1; */
/*   } */
  printf("\n !!!!!!!!!! done !\n");
  free_chain(chain);
  return 0;
}

int dma_free(dma_chain_t * chain)
{
  int irq = 0;

  if (chain == NULL)
    return -1;

  if (!irq_inside_int())
    irq = irq_disable();

  /* mark we won't wait on the semaphore */
  chain->src = NULL;

  if (!chain->dest) {
    /* the semaphore has been signaled already */
    assert(!sem_trywait(chain->sema));
    free_chain(chain);
  }
    
  if (!irq_inside_int())
    irq_restore(irq);
  return 0;
}

void dma_cache_flush(void * ptr, int len)
{
  uint8 * p = (uint8 *) ptr;

  /* flush the cache */
  if (len > CACHE_SZ)
    dcache_flush_range(p + len - CACHE_SZ, 
		       CACHE_SZ);
  else
    dcache_flush_range(p, len);
}
