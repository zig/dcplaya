#include <stdio.h>
#include <errno.h>
#include <dc/spu.h>
#include <dc/asic.h>
#include <dc/g2bus.h>
#include <arch/cache.h>
#include <kos/sem.h>

#include "dma.h"

/* all is integrated in KOS now */
#define extern

extern int bba_dma_mode;

//#define TEST

#define NEW_METHOD

/* below this value, with use normal CPU copy method */
#define COPY_THRESHOLD 64 //(1024+512)

/* Use only one DMA channel */
#define ONE_DMA

/* above this threshold, we use two dma at the same time ! */
#define DMA2_THRESHOLD 1024 // 16*1024 /* == OFF */

static int stat_copy;
static uint64 stat_copy_t;
static int stat_dma_copy;
static uint64 stat_dma_copy_t;
static int stat_dma;
static uint64 stat_dma_t;

/*

Handles the DMA part of the BBA functionality.

Thanks to Bitmaster for the info on BBA DMA, and Roger Cattermole who
got a well-functioning PVR DMA module (helped this a bit).

XXX: Right now this conflicts with PVR DMA but we ought to be able
to fix that by going to another channel.

XXX: This ought to be abstracted out to allow usage with the parallel
port as well.

*/

typedef struct {
	uint32		ext_addr;		/* External address (SPU-RAM or parallel port) */
	uint32		sh4_addr;		/* SH-4 Address */
	uint32		size;			/* Size in bytes; all addresses and sizes must be 32-byte aligned */
	uint32		dir;			/* 0: cpu->ext; 1: ext->cpu */
	uint32		mode;			/* 5 for SPU transfer */
	uint32		ctrl1;			/* b0 */
	uint32		ctrl2;			/* b0 */
	uint32		u1;			/* ?? */
} g2_dma_ctrl_t;

typedef struct {
	uint32		ext_addr;
	uint32		sh4_addr;
	uint32		size;
	uint32		status;
} g2_dma_stat_t;

typedef struct {
	g2_dma_ctrl_t	dma[4];
	uint32		u1[4];			/* ?? */
	uint32		wait_state;
	uint32		u2[10];			/* ?? */
	uint32		magic;
	g2_dma_stat_t	dma_stat[4];
} g2_dma_reg_t;

/* DMA registers */
static vuint32	* const shdma = (vuint32 *)0xffa00000;
static volatile g2_dma_reg_t * const extdma = (g2_dma_reg_t *)0xa05f7800;
static int	chn = 0;	/* 0 for SPU; 1, 2, 3 for EXT */

/* DMAC registers. We use channel 3 here to avoid conflicts with the PVR. */
#define DMAC_CHNL       1 /* VP : configurable channel number */
#define DMAC_SAR	(DMAC_CHNL*0x10 + 0x00)/4
#define DMAC_DAR	(DMAC_CHNL*0x10 + 0x04)/4
#define DMAC_DMATCR	(DMAC_CHNL*0x10 + 0x08)/4
#define DMAC_CHCR	(DMAC_CHNL*0x10 + 0x0c)/4
#define DMAC_DMAOR	0x40/4

typedef spu_dma_callback_t bba_dma_callback_t;

/* Signaling semaphore */
static semaphore_t * dma_done;
static int dma_blocking;   
static bba_dma_callback_t dma_callback;
static ptr_t dma_cbdata;

static void dma_disable() {
	/* Disable the DMA */
	extdma->dma[chn].ctrl1 = 0;
	extdma->dma[chn].ctrl2 = 0;
	shdma[DMAC_CHCR] &= ~1;
}

// VP
#define dbglog(...) (void) 0

static void bba_dma_irq(uint32 code) {
	if (shdma[DMAC_DMATCR] != 0)
		dbglog(DBG_INFO, "bba_dma: DMA did not complete successfully\n");

	dma_disable();

	/* VP : changed the order of things so that we can chain dma calls */

	// Signal the calling thread to continue, if any.
	if (dma_blocking) {
		sem_signal(dma_done);
		thd_schedule(1, 0);
		dma_blocking = 0;
	}

	// Call the callback, if any.
	if (dma_callback) {
	        bba_dma_callback_t tmp = dma_callback;
		dma_callback = NULL;
/* 		dma_cbdata = 0; */
		tmp(dma_cbdata);
	}

}

// XXX: Write (and check here) bba_dma_ready()

uint8 bba_dma_buf[16*1024+64] __attribute__((aligned(32)));
uint8 * bba_dma_pbuf = bba_dma_buf;

#ifdef TEST
static int bba_length;
uint8 bba_buf[16*1024] __attribute__((aligned(32)));
#endif

int bba_dma_busy;

extern void (* bba_dma_func) (void *from, uint32 dest, uint32 length, int block,
			      bba_dma_callback_t callback, ptr_t cbdata);

extern void (* bba_dma_wait_func) ();

static struct {
  dma_chain_t * c;
  void * from;
  void * rfrom;
  uint32 l;
} chains[2];
static int ichain;


#ifdef NEW_METHOD
#include <kos/sem.h>
static semaphore_t * dma_sema;
static dma_busy;
static void dma_cb(void * dummy)
{
  sem_signal(dma_sema);
  thd_schedule(1, 0);
}

static semaphore_t * dma2_sema;
static dma2_busy;
static void dma2_cb(void * dummy)
{
  sem_signal(dma2_sema);
  thd_schedule(1, 0);
}
#endif


void bba_dma_transfer(void *from, uint32 dest, uint32 length, int block,
	bba_dma_callback_t callback, ptr_t cbdata)
{
  if (!length)
    return;

  //uint64 t = timer_micro_gettime64();
  /* dma is not worth using below a certain threshold */
  if (length < COPY_THRESHOLD) {
  cpucopy:
    stat_copy += length;
    //g2_read_block_32(from, dest, length/4);
    memcpy(from, dest, length);
    //stat_copy_t += timer_micro_gettime64() - t;
    return;
  }

  tx_lock();

#ifndef TEST

#ifdef NEW_METHOD
  if (!dma_sema) {
    dma_sema = sem_create(0);
    dma2_sema = sem_create(0);
  }

#ifndef ONE_DMA
  if (dma2_busy)
#endif
    bba_dma_wait();

  vid_border_color(255, 128, 128);
  dcache_inval_range(from, length);

#ifdef ONE_DMA
  dma_busy = 1;
  g2_dma_transfer(from, dest, length, 0, 
		  dma_cb, 0, 1, bba_dma_mode, 1, 1);
#else
  uint32 l = (length/2)&~31;
  if (dma_busy) {
    dma2_busy = 1;
    g2_dma_transfer(from, dest, length, 0, 
		    dma2_cb, 0, 1, 3, 2, 0);
  } else if (length < DMA2_THRESHOLD || !l || !(length-l)) {
    dma_busy = 1;
    g2_dma_transfer(from, dest, length, 0, 
		    dma_cb, 0, 1, bba_dma_mode, 1, 1);
  } else {
    dma_busy = 1;
    dma2_busy = 1;
    g2_dma_transfer(from, dest, l, 0, 
		    dma2_cb, 0, 1, 3, 2, 0);
    g2_dma_transfer(from+l, dest+l, length-l, 0, 
		    dma_cb, 0, 1, bba_dma_mode, 1, 1);
  }
#endif

#else
  void * rfrom = from;

  if ( ((int)from) & 31 ) {
    stat_dma_copy += length;
    from = bba_dma_pbuf;
    bba_dma_pbuf += (length+63)&~31;
    chains[ichain].from = from;
    chains[ichain].rfrom = rfrom;
    chains[ichain].l = length;
  } else
    stat_dma += length;

  dcache_inval_range(from, length);

  /* is that necessary ? LOOKS LIKE IT IS ! */
  bba_dma_busy = 1;

  chains[ichain++].c = dma_initiate(from, dest, length, DMA_TYPE_BBA_RX);
  if (!chains[ichain-1].c) {
    ichain--;
    goto cpucopy;
  }
#endif // NEW_METHOD

#else
	bba_dma_func = 0;

	dcache_inval_range(bba_dma_buf, length);

	void * rfrom = from;

	from = bba_dma_buf;

#if 0
        asic_evt_disable(ASIC_EVT_SPU_DMA, ASIC_IRQ_DEFAULT);

	uint32 val;
	/* Check alignments */
	if ( ((uint32)from) & 31 ) {
	  panic("from alignement");
		dbglog(DBG_ERROR, "bba_dma: unaligned source DMA %p\n", from);
		errno = EFAULT;
		return -1;
	}
	if ( ((uint32)dest) & 31 ) {
	  panic("dest alignement");
		dbglog(DBG_ERROR, "bba_dma: unaligned dest DMA %p\n", (void *)dest);
		errno = EFAULT;
		return -1;
	}
	length = (length + 0x1f) & ~0x1f;
	length += 32;

	/* Adjust destination to SPU RAM */
	//dest += 0x00800000;

	val = shdma[DMAC_CHCR];

	// DE bit set so we must clear it?
	if (val & 1)
		shdma[DMAC_CHCR] = val | 1;
	// TE bit set so we must clear it?
	if (val & 2)
		shdma[DMAC_CHCR] = val | 2;

	/* Setup the SH-4 channel */
	shdma[DMAC_SAR] = 0;		/* SAR = 0 */
	shdma[DMAC_DMATCR] = 0;	        /* DMATCR = 0 */
	shdma[DMAC_CHCR] = 0x12c0;	/* CHCR = 0x12c0; 32-byte block transfer,
					   burst mode, external request, single address mode,
					   source address incremented; */

	val = shdma[DMAC_DMAOR];
	if ((val & 0x8007) != 0x8001) {
		dbglog(DBG_ERROR, "bba_dma: failed DMAOR check\n");
		errno = EIO;
		return -1;
	}

	dma_blocking = block;
	dma_callback = callback;
	dma_cbdata = cbdata;

	/* Start the DMA transfer */
	extdma->dma[chn].ctrl1 = 0;
	extdma->dma[chn].ctrl2 = 0;
	extdma->dma[chn].ext_addr = dest & 0x1fffffe0;
	extdma->dma[chn].sh4_addr = ((uint32)from) & 0x1fffffe0;
	extdma->dma[chn].size = (length & ~31) | 0x80000000;
	extdma->dma[chn].dir = 1;
	extdma->dma[chn].mode = bba_dma_mode;	/* SPU == 5 */
	extdma->dma[chn].ctrl1 = 1;
	extdma->dma[chn].ctrl2 = 1;

	/* Wait for us to be signaled */
/* 	if (block) */
/* 		sem_wait(dma_done); */

#endif

	g2_dma_transfer(from, dest, length, 1, 
			0, 0, 1, bba_dma_mode, 1, 1);
	

/* 	{ */
/* 	  int i; */
/* 	  for (i=0; i<length*15; i++) */
/* 	    *((int *)0) = i; */
/* 	} */

/* 	memcpy(rfrom, bba_dma_buf, length); */
	bba_length = length;
	memcpy(rfrom, dest, length);
	memcpy(bba_buf, dest, length);

#endif

  tx_unlock();
}


static void bba_dma_wait()
{
#ifdef NEW_METHOD
  vid_border_color(0, 0, 0);
  if (dma_busy) {
    sem_wait(dma_sema);
    dma_busy = 0;
  }
  if (dma2_busy) {
    sem_wait(dma2_sema);
    dma2_busy = 0;
  }
  vid_border_color(255, 0, 255);
#else
  int i;
/*   for (i=0; i<length*100; i++) */
/*     *((int *)0) = i; */
  vid_border_color(0, 0, 0);
  //dma_wait(chain);
  for (i=0; i<ichain; i++) {
    dma_wait(chains[i].c);
    if (chains[i].rfrom) {
      memcpy(chains[i].rfrom, chains[i].from, chains[i].l);
      chains[i].rfrom = NULL;
    }
  }
  bba_dma_pbuf = bba_dma_buf;
  ichain = 0;
  vid_border_color(0, 0, 255);
  bba_dma_busy = 0;
/*   if (rfrom != from) { */
/*     memcpy(rfrom, bba_dma_buf, length); */
/*     stat_dma_copy_t += timer_micro_gettime64() - t; */
/*   } else */
/*     stat_dma_t += timer_micro_gettime64() - t; */
#endif
}


//#define ASIC_EVT_EXP_8BIT ASIC_EVT_SPU_DMA

#if 0
int bba_dma_init() {
	/* Create an initially blocked semaphore */
	dma_done = sem_create(0);
	dma_blocking = 0;
	dma_callback = NULL;
	dma_cbdata = 0;

	// Hook the interrupt
	//asic_evt_set_handler(ASIC_EVT_EXP_8BIT, bba_dma_irq);
	//asic_evt_enable(ASIC_EVT_EXP_8BIT, ASIC_IRQ_DEFAULT);

	/* Setup the DMA transfer on the external side */
	extdma->wait_state = 27;
	extdma->magic = 0x4659404f;

	return 0;
}

void bba_dma_shutdown() {
	// Unhook the G2 interrupt
	//asic_evt_disable(ASIC_EVT_EXP_8BIT, ASIC_IRQ_DEFAULT);
	//asic_evt_set_handler(ASIC_EVT_EXP_8BIT, NULL);

	/* Destroy the semaphore */
	sem_destroy(dma_done);

	/* Turn off any remaining DMA */
	dma_disable();
}
#endif

void dump(uint32 * addr)
{
  int i, j;
  for (i=0; i<1; i++) {
    for (j=0; j<6; j++)
      printf("%0.8x ", *addr++);
    printf("\n");
  }
  printf("\n");
}

void dma_test(int mode)
{
  if (mode >= 0) {
#ifdef TESTx
    chn = mode;
    bba_dma_mode = 0;
#else
    bba_dma_mode = mode;
#endif
    bba_dma_func = bba_dma_transfer;
    bba_dma_wait_func = bba_dma_wait;
    
    printf("Setting BBA in dma mode %d ...\n", mode);
  } else {
    bba_dma_func = 0;
    bba_dma_wait_func = 0;
    
    printf("Shutting down BBA dma mode ...\n");
    return;
  }

#ifdef TEST
  //printf("Testing BBA DMA transfer ...\n");

  while (bba_dma_func)
    thd_pass();

  printf("dumping DMA buf :\n");
  dump(bba_dma_buf);
  printf("dumping CPU buf :\n");
  dump(bba_buf);

  if (!memcmp(bba_dma_buf, bba_buf, bba_length)) {
    printf("Bravo !!\n");
  }
	
#endif

  dma_stat();
}

void dma_stat()
{
  printf("stat_copy :     %0.5d Kb (%5d Kb/s)\n", 
	 stat_copy/1024,
	 stat_copy*1000000LL/1024/stat_copy_t);
  printf("stat_dma_copy : %0.5d Kb (%5d Kb/s)\n", 
	 stat_dma_copy/1024,
	 stat_dma_copy*1000000LL/1024/stat_dma_copy_t);
  printf("stat_dma :      %0.5d Kb (%5d Kb/s)\n", 
	 stat_dma/1024,
	 stat_dma*1000000LL/1024/stat_dma_t);

  memset(&stat_copy, 0, 4*9);
}
