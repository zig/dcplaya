
#include <kos.h>
#include <math.h>

#include "dcplaya/config.h"
#include "inp_driver.h"
#include "pcm_buffer.h"
#include "fifo.h"
#include "sysdebug.h"

/* PCM buffer: for storing data going out to the SPU */
/*static short pcm_buffer[2*44100/441]; // a LA 441 :) */
static int pcm_count = 44100/441;
static int pcm_stereo;

//static int32 last_read=0; /* number of bytes the sndstream driver grabbed at last callback */

static int sndtst_init(any_driver_t * d)
{
  SDDEBUG("%s([%s]) := [0]\n", __FUNCTION__, d->name);
  return 0;
}


/* Start playback (implies song load) */
static int sndtst_start(const char *fn, int track, playa_info_t *info)
{
  int err = -1;
  int i;

  SDDEBUG("%s([%s])\n", fn);
  SDINDENT;


  for (i=0; i<pcm_count; i++) {
    pcm_buffer[2*i] = 3000.0f*sin(2*i*2*3.14159f/pcm_count);
    pcm_buffer[2*i+1] = 3000.0f*sin(i*2*3.14159f/pcm_count);
  }


  playa_info_bits(info,16);
  playa_info_stereo(info, 1); /* 1 = stereo ? */
  playa_info_frq(info,44100);
  playa_info_bps(info,128);
  playa_info_bytes(info,pcm_count*2); /* ? */
  playa_info_time(info, 100000);

  playa_info_desc(info, "0.0");
/*  sndtst_info(info, 0);*/

  pcm_stereo = 1;
  err = 0;

 error:
  SDUNINDENT;
  SDDEBUG("%s() := [%d]\n", __FUNCTION__, err);
  return err;
}

/* sndtstvorbis_stop()
 *
 * function to stop the current playback and set the thread back to
 * STATUS_READY mode.
 */
static int sndtst_stop(void)
{
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDDEBUG("<< %s()\n", __FUNCTION__);
  return 0;
}

/* Shutdown the player */
static int sndtst_shutdown(any_driver_t * d)
{
  SDDEBUG("%s(%s) := [0]\n", __FUNCTION__, d->name);
  sndtst_stop();
  return 0;
}

static int sndtst_decoder(playa_info_t * info)
{
  /* If we have some pcm , send them to fifo */
  fifo_write((int *) pcm_buffer, pcm_count);
  //    n = fifo_write_mono(pcm_buffer, pcm_count);
  return -(1>0) & INP_DECODE_CONT;
}

static char *StrDup(const char *s)
{
  if (!s) return 0;
  return strdup(s);
}


static int sndtst_info(playa_info_t *info, const char *fname)
{
  return 0;
}

static driver_option_t * sndtst_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

static inp_driver_t tst_driver = {
  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    INP_DRIVER,           /* Driver type */
    0x100,                /* Version */
    "sndtst",             /* Driver name */
    "Vincent Penne",      /* Authors */
    "sound test",         /* Description */
    0,                    /* Dll */
    sndtst_init,          /* Init */
    sndtst_shutdown,      /* Shutdown */
    sndtst_options        /* Options */
  },

  /* Input driver specific */
  0,                      /* User Id */
  ".sndtst\0",            /* Extension list */

  sndtst_start,
  sndtst_stop,
  sndtst_decoder,
  sndtst_info,
};

EXPORT_DRIVER(tst_driver)
