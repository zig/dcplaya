
#include <kos.h>
#include "math_float.h"

#include "dcplaya/config.h"
#include "inp_driver.h"
#include "pcm_buffer.h"
#include "fifo.h"
#include "sysdebug.h"
#include "filename.h"

static int pcm_count = 44100/441;
static int pcm_current;

//static int32 last_read=0; /* number of bytes the sndstream driver grabbed at last callback */

static int sndtst_init(any_driver_t * d)
{
  SDDEBUG("%s([%s]) := [0]\n", __FUNCTION__, d->name);
  return 0;
}

static int get_frq(const char *fn)
{
  int frq = 440;

  fn = fn_basename(fn);
  if (fn) {
    int c;
    if (c = *fn++, (c>='0' && c<='9')) {
      frq = 0;
      do {
	frq = frq * 10 + c - '0';
      } while (c = *fn++, (c>='0' && c<='9'));
    }
  }
  if (frq < 20) frq = 20;
  else if (frq > 44100/2) frq = 44100/2;
  return frq;
}

/* Start playback (implies song load) */
static int sndtst_start(const char *fn, int track, playa_info_t *info)
{
  int err = -1;
  int i, frq;
  float a, stp;
  const float amp = 14000;
  char tmp[128];

  SDDEBUG("%s([%s])\n", fn);
  SDINDENT;

  frq = get_frq(fn);
  pcm_current = 0;
  pcm_count = 44100 / frq;
  if (pcm_buffer_init(pcm_count, 0)) {
    goto error;
  }

  for (i=0, a=0, stp=2.0*MF_PI/(float)pcm_count; i<pcm_count; i++, a += stp) {
    pcm_buffer[i] = Sin(a) * amp;
  }

  playa_info_bits(info,16);
  playa_info_stereo(info, 0); /* 1 = stereo ? BeN : yes that's it :) */
  playa_info_frq(info,44100);
#if 0
  playa_info_bps(info,4*44100);
  playa_info_bytes(info,pcm_count*4); /* ? BeN: only for streaming,
					 but anyway if you want to set it :
					 x4 (x2:short x2:stereo */
  playa_info_time(info, 100000);
#endif
  playa_info_genre(info, "head-ache");
  playa_info_comments(info, "What a nice sound , isn't it ?");
  
  sprintf(tmp,"%dhz sinus generator", (int)frq);
  playa_info_desc(info, tmp);

  sprintf(tmp,"%dhz sinus", (int)frq);
  playa_info_title(info, tmp);

/*  sndtst_info(info, 0);*/

  err = 0;

 error:
  if (err) {
    pcm_buffer_shutdown();
  }
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
  pcm_buffer_shutdown();
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
  int n;

  n = fifo_write_mono(pcm_buffer + pcm_current, pcm_count - pcm_current);
  if (n < 0) {
    return INP_DECODE_ERROR;
  }
  pcm_current += n;
  if (pcm_current >= pcm_count) {
    pcm_current = 0;
  }

  return -(n>0) & INP_DECODE_CONT;
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
    "441hz sinus "
    "generator",          /* Description */
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
