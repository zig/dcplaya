/* Streaming sound driver
 *
 * (c)2000 Dan Potter
 *
 * This slightly more complicated version allows for sound effect channels,
 * and full sampling rate, panning, and volume control for each. The two
 * streaming channels are still always first and always occur at
 * SPU_SPL1_ADDR and SPU_SPL2_ADDR. All other sample data can begin at
 * SPU_TOP_RAM. "pos" only works for input on the two streaming channels
 * (which will always have the same "pos" value).
 * 
 */

#include "aica.h"
#include "aica_cmd_iface.h"

/****************** Timer *******************************************/

extern volatile int timer;

void timer_wait(int jiffies) {
  int fin = timer + jiffies;
  while (timer <= fin)
    ;
}

/****************** Main Program ************************************/

/* Set channel id at 0x80280d (byte), read position at 0x802814 (long) */

volatile uint32 *cmd = (volatile uint32 *)0x10000;
volatile aica_channel *chans = (volatile aica_channel *)0x10004;

static int stream_invert       = 0;  /* 0 or 255 */
static int stream_channel_mask = 3;  /* bit 0:left bit 1:right */


static void start_streaming_channels(void) {
  uint32 pos = SPU_SPL1_ADDR;
  uint32 len = chans[0].length;
  uint32 pan = stream_invert;

  /* Stop any current streaming */
  aica_stop(0);
  aica_stop(1);
		
  /* Left channel only is MONO mode : center panning */
  if (stream_channel_mask==1) {
    pan = 128;
  }
		
  /* Start wanted channels with an optionnal panning inversion */
  if (stream_channel_mask & 1) {
    aica_play(0, pos, SM_16BIT, 0, len, chans[0].freq, chans[0].vol, pan, 1);
  }
  if (stream_channel_mask & 2) {
    aica_play(1, pos+(len<<1), SM_16BIT, 0, len, chans[0].freq, chans[0].vol, 255-pan, 1);
  }
		
  /* Do the real start after aica_play() calls setup channel parameters.
   * By the way, both channels should be really synchronized ...
   */
  aica_stream_start(pos, pos+(len<<1), stream_channel_mask, SM_16BIT);
}

static void start_channel(int chn) {
  if (chn < 2) {
    start_streaming_channels();
  } else {
    aica_play(chn, chans[chn].pos, SM_16BIT, 0, chans[chn].length,
	      chans[chn].freq, chans[chn].vol, chans[chn].pan, 0);
  }
}

static void stop_channel(int chn) {
  if (chn == 0) {
    aica_stop(0);
    aica_stop(1);
  } else {
    aica_stop(chn);
  }
}

static void vol_channel(int chn) {
  if (chn == 0) {
    aica_vol(0, chans[chn].vol);
    aica_vol(1, chans[chn].vol);
  } else {
    aica_vol(chn, chans[chn].vol);
  }
}

static void frq_channel(int chn) {
  if (chn == 0) {
    aica_freq(0, chans[chn].freq);
    aica_freq(1, chans[chn].freq);
  } else {
    aica_freq(chn, chans[chn].freq);
  }
}

void process_cmd(uint32 cmd) {
  /* cmd is channel to look at +1 */
  cmd--;
  switch(chans[cmd].cmd) {
  case AICA_CMD_NONE:
    break;
  case AICA_CMD_START:
    start_channel(cmd);
    break;
  case AICA_CMD_STOP:
    stop_channel(cmd);
    break;
  case AICA_CMD_VOL:
    vol_channel(cmd);
    break;
  case AICA_CMD_FRQ:
    frq_channel(cmd);
    break;
  case AICA_CMD_MONO:
    stream_invert = 0;
    stream_channel_mask = 1;
    break;
  case AICA_CMD_STEREO:
    stream_invert = 0;
    stream_channel_mask = 3;
    break;
  case AICA_CMD_INVERSE:
    stream_invert = 255;
    stream_channel_mask = 3;
    break;
  case AICA_CMD_PARM:
    vol_channel(cmd);
    frq_channel(cmd);
    break;
  }
}

int arm_main() {
  int cmdl;

  /* Default : streaming OFF on both channel */
  stream_channel_mask = 3;
  stream_invert = 0;
  
  /* Initialize the AICA part of the SPU */
  aica_init();

  /* Observe channel 0 */
  SNDREG8(0x280d) = 0;

  /* Wait for a command */
  while(1) {
    /* Check for a command */
    cmdl = *cmd;
    if (cmdl & AICA_CMD_KICK) {
      *cmd = 0;
      process_cmd(cmdl & ~AICA_CMD_KICK);
    }

    /* Update position counters */
    chans[0].pos = SNDREG32(0x2814);

    /* Little delay to prevent memory lock */
    timer_wait(10);
  }
}
