/* Tryptonite

   sndstream.c
   (c)2000 Dan Potter

   SH-4 support routines for SPU streaming sound driver

   +2002/02/12 variable stream size modification by ben(jamin) gerard
*/

static char id[] = "sndserver $Id: sndstream.c,v 1.3 2003-03-10 22:55:35 ben Exp $";

#include <kos.h>
#include "dcplaya/config.h"
#include "sndstream.h"
#include "sysdebug.h"

#include <arch/types.h>
#include "arm/aica_cmd_iface.h"

#ifndef _BEN_AICA
#error "BAD aica_cmd_iface.h"
#endif

#define SPU_SPL2_ADDR (SPU_SPL1_ADDR + (stream_bytes<<1))

/*

Here we implement a very simple double-buffering, not the rather complex
circular queueing done in most DMA'd sound architectures. This is done for
simplicity's sake. At some point we may upgrade that the circular queue
system.

Basically this poll routine checks to see which buffer is playing, and
whether a new one is available or not. If not, we ask the user routine
for more sound data and load it up. That's about it.

*/

/* What we think is the currently playing buffer */
static int curbuffer = 0;

/* Var to check for channel position */
static vuint32 *cmd = (vuint32*)(0xa0810000);
static volatile aica_channel *chans = (volatile aica_channel*)(0xa0810004);

/* Seperation buffers (for stereo) */
static int16 *sep_buffer[2] = {NULL, NULL};

/* "Get data" callback */
static void* (*str_get_data)(int cnt) = NULL;

/* SPU RAM mutex (to avoid queue overflow) */
static spinlock_t mutex;

/* SPU RAM malloc pointer */
static uint32 ram_base, ram_top;

/* Stereo/mono flag for stream */
static int stereo;

/* In-ram copy of the stream.drv file in case the CD gets swapped */
static int streamdrv_size;
static int stream_bytes = 0;
static int stream_bytes_max = 0;
const int stream_bytes_min = 64;

/* Set "get data" callback */
void stream_set_callback(void *(*func)(int)) {
  spinlock_lock(&mutex);
  str_get_data = func;
  spinlock_unlock(&mutex);
}

/* "Kicks" a channel command */
static void chn_kick(int chn) {
  *cmd = AICA_CMD_KICK | (chn+1);
  spu_write_wait();
}

/* Performs stereo seperation for the two channels; this routine
   has been optimized for the SH-4. */
static void sep_data(void *buffer) {
  register int16  *bufsrc, *bufdst;
  register int  x, y, cnt;

  if (stereo) {
    bufsrc = (int16*)buffer;
    bufdst = sep_buffer[0];
    x = 0; y = 0; cnt = (stream_bytes>>1);
    do {
      *bufdst = *bufsrc;
      bufdst++; bufsrc+=2; cnt--;
    } while (cnt > 0);

    bufsrc = (int16*)buffer; bufsrc++;
    bufdst = sep_buffer[1];
    x = 1; y = 0; cnt = stream_bytes>>1;
    do {
      *bufdst = *bufsrc;
      bufdst++; bufsrc+=2; cnt--;
      x+=2; y++;
    } while (cnt > 0);
  } else {
    /* $$$ The second buffer copy should be remove. But since I do not have 
           stereo with my dreamcast, I can't test it !!! */
    memcpy(sep_buffer[0], buffer, stream_bytes);
    memcpy(sep_buffer[1], buffer, stream_bytes);
  }
}

/* Prefill buffers -- do this before calling start() */
void stream_prefill()
{
  void *buf;

  if (!str_get_data) {
    spinlock_lock(&mutex);
    spu_memset(SPU_SPL1_ADDR, 0, stream_bytes*2);
    spu_memset(SPU_SPL2_ADDR, 0, stream_bytes*2);
    spinlock_unlock(&mutex);
    return;
  }

  /* Load first buffer */
  buf = str_get_data(stream_bytes << stereo);
  sep_data(buf);

  spinlock_lock(&mutex);
  spu_memload(SPU_SPL1_ADDR + stream_bytes*0, (uint8*)sep_buffer[0], stream_bytes);
  spu_memload(SPU_SPL2_ADDR + stream_bytes*0, (uint8*)sep_buffer[1], stream_bytes);
  spinlock_unlock(&mutex);

  /* Load second buffer */
  buf = str_get_data(stream_bytes << stereo);
  sep_data(buf);
  spinlock_lock(&mutex);
  spu_memload(SPU_SPL1_ADDR + stream_bytes*1, (uint8*)sep_buffer[0], stream_bytes);
  spu_memload(SPU_SPL2_ADDR + stream_bytes*1, (uint8*)sep_buffer[1], stream_bytes);
  spinlock_unlock(&mutex);

  /* Start with playing on buffer 0 */
  curbuffer = 0;
}

/* Initialize stream system */
int stream_init(void* (*callback)(int), int samples)
{
  uint32  hnd;
  int bytes;
  char *streamdrv = 0;

  SDDEBUG("[%s] : %d\n",  __FUNCTION__ , samples);

  /* Load the AICA driver. Do this once. */
  hnd = fs_open("/rd/stream.drv", O_RDONLY);
  if (!hnd) {
    SDERROR("[%s] : can't open sound driver\n", __FUNCTION__);
    return -1;
  }
  streamdrv_size = fs_total(hnd);
  streamdrv = fs_mmap(hnd);
  if (!streamdrv) {
    SDERROR("[%s] : driver alloc failed\n", __FUNCTION__);
    fs_close(hnd);
    return -1;
  }

  samples = (samples+15) & -16;
  bytes   = (samples << 1);
  if (bytes < stream_bytes_min) {
    bytes = stream_bytes_min;
  } else if (bytes > SPU_STREAM_MAX) {
    bytes = SPU_STREAM_MAX;
  }
  stream_bytes      = 0;
  stream_bytes_max  = bytes;

  /* Create stereo seperation buffers */
  sep_buffer[0] = realloc(sep_buffer[0], stream_bytes_max);
  sep_buffer[1] = realloc(sep_buffer[1], stream_bytes_max);

  if (!sep_buffer[0] || !sep_buffer[1]) {
    SDERROR("[%s] : Separate buffer allocation failed\n",__FUNCTION__);
    if (sep_buffer[0]) free(sep_buffer[0]);
    if (sep_buffer[1]) free(sep_buffer[1]);
    sep_buffer[0] = sep_buffer[1] = 0;
    return -1;
  }
  SDDEBUG("[%s] : Sound buffers allocated (2  %d bytes)\n",
		  __FUNCTION__, stream_bytes_max);

  /* Finish loading the stream driver */
  spu_disable();
  spu_memset(0, 0, SPU_TOP_RAM);
  spu_memload(0, streamdrv, streamdrv_size);
  spu_enable();
  SDDEBUG("SPU enabled\n");

  if (hnd) fs_close(hnd);

  /* Setup a mem load mutex */
  spinlock_init(&mutex);
  ram_base = ram_top = SPU_TOP_RAM;

  /* Setup the callback */
  stream_set_callback(callback);

  return 0;
}

/* Shut everything down and free mem */
void stream_shutdown()
{
  SDDEBUG("[%s]\n", __FUNCTION__);

//  stream_stop();

  if (sep_buffer[0]) {
    free(sep_buffer[0]);  sep_buffer[0] = NULL;
    free(sep_buffer[1]);  sep_buffer[1] = NULL;
  }
  str_get_data = 0;
  streamdrv_size = 0;
  stream_bytes = 0;
  stream_bytes_max = 0;
}

/* Start streaming */
void stream_start(int samples, uint32 freq, int vol, int st)
{
  SDDEBUG("[%s] : (spl:%d,frq:%d,vol:%d,stereo:%d)\n",
		  __FUNCTION__,samples, freq, vol, st);

  if (!str_get_data || !stream_bytes_max) {
	SDERROR("[%s] : init has failed ?\n", __FUNCTION__);
    return;
  }

  /* Select "observation" channel */
  /* *chsel = 0; */

  samples <<= 1;
  if (samples < stream_bytes_min) {
    samples = stream_bytes_min;
  } else if (samples > stream_bytes_max) {
    samples = stream_bytes_max;
  }
  stream_bytes = samples;
  stereo = (st != 0);

  /* Prefill buffers */
  stream_prefill();
  
  /* Start streaming */
  chans[0].cmd    = AICA_CMD_START;
  chans[0].freq   = freq;
  chans[0].length = stream_bytes;
  chans[0].vol    = vol&255;
  /* chans[0].pos    = SPU_SPL1_ADDR; */
  chn_kick(0);
}

/* Stop streaming */
void stream_stop() {
  if (!str_get_data) return;

  stream_bytes = 0;
  /* Stop stream */
  chans[0].cmd = AICA_CMD_STOP;
  chn_kick(0);
}

/* Poll streamer to load more data if neccessary
 * return number of sample send or -1
 */
int stream_poll() {
  int realbuffer;
  uint32  val;
  void  *data;

  if (!str_get_data) return -1;

  /* Get "real" buffer */
  val = chans[0].pos;
  spu_write_wait();
  realbuffer = !(val < (stream_bytes>>1));

  /* Has the channel moved on from the "current" buffer? */
  if (curbuffer != realbuffer) {
    /* Yep, adjust "current" buffer and initiate a load */

    data = str_get_data(stream_bytes << stereo);
    if (data == NULL) {
      /* Fill the "other" buffer with zeros */
      spinlock_lock(&mutex);
      spu_memset(SPU_SPL1_ADDR + stream_bytes*curbuffer, 0, stream_bytes);
      spu_memset(SPU_SPL2_ADDR + stream_bytes*curbuffer, 0, stream_bytes);
      spinlock_unlock(&mutex);
      return -1;
    } else {
      sep_data(data);
      spinlock_lock(&mutex);
      spu_memload(SPU_SPL1_ADDR + stream_bytes*curbuffer, (uint8*)sep_buffer[0], stream_bytes);
      spu_memload(SPU_SPL2_ADDR + stream_bytes*curbuffer, (uint8*)sep_buffer[1], stream_bytes);
      spinlock_unlock(&mutex);
      curbuffer = realbuffer;
    }
    return stream_bytes>>1;
  }
  return 0;
}

/* Set the volume on the streaming channels */
void stream_volume(int vol) {
  spinlock_lock(&mutex);
  chans[0].cmd = AICA_CMD_VOL;
  chans[0].vol = vol;
  chn_kick(0);
  spinlock_unlock(&mutex);
}

void stream_frq(int frq) {
  spinlock_lock(&mutex);
  chans[0].cmd = AICA_CMD_FRQ;
  chans[0].freq = frq;
  chn_kick(0);
  spinlock_unlock(&mutex);
}

/* 0:mono 1:stereo 2:invert-stereo */
void stream_stereo(int stereo) {
  spinlock_lock(&mutex);
  chans[0].cmd = AICA_CMD_MONO + stereo;
  chn_kick(0);
  spinlock_unlock(&mutex);
}



