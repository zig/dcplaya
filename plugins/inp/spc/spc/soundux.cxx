/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2000 Gary Henderson (gary@daniver.demon.co.uk) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (Ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */

#ifdef __DJGPP__
#include <allegro.h>
#undef TRUE
#endif

extern "C" {
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
}


//#include <errno.h>
//#include <fcntl.h>

#define CLIP16(v) \
if ((v) < -32768) \
    (v) = -32768; \
else \
if ((v) > 32767) \
    (v) = 32767

#define CLIP16_latch(v,l) \
if ((v) < -32768) \
{ (v) = -32768; (l)++; }\
else \
if ((v) > 32767) \
{ (v) = 32767; (l)++; }

#define CLIP24(v) \
if ((v) < -8388608) \
    (v) = -8388608; \
else \
if ((v) > 8388607) \
    (v) = 8388607

#define CLIP8(v) \
if ((v) < -128) \
    (v) = -128; \
else \
if ((v) > 127) \
    (v) = 127

#include "snes9x.h"
#include "soundux.h"
#include "apu.h"
#include "memmap.h"
// #include "cpuexec.h"


extern int Echo [24000];
extern int DummyEchoBuffer [SOUND_BUFFER_SIZE];
extern int MixBuffer [SOUND_BUFFER_SIZE];
extern int EchoBuffer [SOUND_BUFFER_SIZE];
extern int FilterTaps [8];
extern unsigned long Z;
extern int Loop [16];

extern long FilterValues[4][2];
extern int NoiseFreq [32];

#undef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))

#define FIXED_POINT 0x10000UL
#define FIXED_POINT_REMAINDER 0xffffUL
#define FIXED_POINT_SHIFT 16

#define VOL_DIV8  0x8000
#define VOL_DIV16 0x0080
#define ENVX_SHIFT 24

// F is channel's current frequency and M is the 16-bit modulation waveform
// from the previous channel multiplied by the current envelope volume level.
#define PITCH_MOD(F,M) ((F) * ((((M) & 0x7fffff) >> 16) + 1) >> 7)
//#define PITCH_MOD(F,M) ((F) * ((((M) & 0x7fffff) >> 14) + 1) >> 8)

#define LAST_SAMPLE 0xffffff
#define JUST_PLAYED_LAST_SAMPLE(c) ((c)->sample_pointer >= LAST_SAMPLE)

STATIC inline uint8 *S9xGetSampleAddress (int sample_number)
{
  uint32 addr = (((APU.DSP[APU_DIR] << 8) + (sample_number << 2)) & 0xffff);
  return (IAPU.RAM + addr);
}

void S9xAPUSetEndOfSample (int i, Channel *ch)
{
  ch->state = SOUND_SILENT;
  ch->mode = MODE_NONE;
  APU.DSP [APU_ENDX] |= 1 << i;
  APU.DSP [APU_KON] &= ~(1 << i);
  APU.DSP [APU_KOFF] &= ~(1 << i);
  APU.KeyedChannels &= ~(1 << i);
}

void S9xAPUSetEndX (int ch)
{
  APU.DSP [APU_ENDX] |= 1 << ch;
}

void S9xSetEnvRate (Channel *ch, unsigned long rate, int direction, int target)
{
  ch->envx_target = target;

  if (rate == ~0UL)
    {
      ch->direction = 0;
      rate = 0;
    }
  else
    ch->direction = direction;

  static int steps [] =
    {
      //	0, 64, 1238, 1238, 256, 1, 64, 109, 64, 1238
      0, 64, 619, 619, 128, 1, 64, 55, 64, 619
    };

  if (rate == 0 || so.playback_rate == 0)
    ch->erate = 0;
  else
    {
      ch->erate = (unsigned long)
	(((int64) FIXED_POINT * 1000 * steps [ch->state]) /
	 (rate * so.playback_rate));
    }
}

void S9xSetEnvelopeRate (int channel, unsigned long rate, int direction,
			 int target)
{
  S9xSetEnvRate (&SoundData.channels [channel], rate, direction, target);
}

void S9xSetSoundVolume (int channel, short volume_left, short volume_right)
{
  Channel *ch = &SoundData.channels[channel];
  if (!so.stereo)
    volume_left = (ABS(volume_right) + ABS(volume_left)) / 2;

  ch->volume_left = volume_left;
  ch->volume_right = volume_right;
  ch-> left_vol_level = (ch->envx * volume_left) / 128;
  ch->right_vol_level = (ch->envx * volume_right) / 128;
}

void S9xSetMasterVolume (short volume_left, short volume_right)
{
  if (Settings.DisableMasterVolume)
    {
      SoundData.master_volume_left = 127;
      SoundData.master_volume_right = 127;
      SoundData.master_volume [0] = SoundData.master_volume [1] = 127;
    }
  else
    {
      if (!so.stereo)
	volume_left = (ABS (volume_right) + ABS (volume_left)) / 2;
      SoundData.master_volume_left = volume_left;
      SoundData.master_volume_right = volume_right;
      SoundData.master_volume [0] = volume_left;
      SoundData.master_volume [1] = volume_right;
    }
}

void S9xSetEchoVolume (short volume_left, short volume_right)
{
  if (!so.stereo)
    volume_left = (ABS (volume_right) + ABS (volume_left)) / 2;
  SoundData.echo_volume_left = volume_left;
  SoundData.echo_volume_right = volume_right;
  SoundData.echo_volume [0] = volume_left;
  SoundData.echo_volume [1] = volume_right;
}

void S9xSetEchoEnable (uint8 byte)
{
  SoundData.echo_channel_enable = byte;
  if (!SoundData.echo_write_enabled || Settings.DisableSoundEcho)
    byte = 0;
  if (byte && !SoundData.echo_enable)
    {
      memset (Echo, 0, sizeof (Echo));
      memset (Loop, 0, sizeof (Loop));
    }

  SoundData.echo_enable = byte;
  for (int i = 0; i < 8; i++)
    {
      if (byte & (1 << i))
	SoundData.channels [i].echo_buf_ptr = EchoBuffer;
      else
	SoundData.channels [i].echo_buf_ptr = DummyEchoBuffer;
    }
}

void S9xSetEchoFeedback (int feedback)
{
  CLIP8(feedback);
  SoundData.echo_feedback = feedback;
}

void S9xSetEchoDelay (int delay)
{
  SoundData.echo_buffer_size = (512 * delay * so.playback_rate) / 32000;
  if (so.stereo)
    SoundData.echo_buffer_size <<= 1;
  if (SoundData.echo_buffer_size)
    SoundData.echo_ptr %= SoundData.echo_buffer_size;
  else
    SoundData.echo_ptr = 0;
  S9xSetEchoEnable (APU.DSP [APU_EON]);
}

void S9xSetEchoWriteEnable (uint8 byte)
{
  SoundData.echo_write_enabled = byte;
  S9xSetEchoDelay (APU.DSP [APU_EDL] & 15);
}

void S9xSetFrequencyModulationEnable (uint8 byte)
{
  SoundData.pitch_mod = byte & ~1;
}

void S9xSetSoundKeyOff (int channel)
{
  Channel *ch = &SoundData.channels[channel];

  if (ch->state != SOUND_SILENT)
    {
      ch->state = SOUND_RELEASE;
      ch->mode = MODE_RELEASE;
      S9xSetEnvRate (ch, 8, -1, 0);
    }
}

void S9xFixSoundAfterSnapshotLoad ()
{
  SoundData.echo_write_enabled = !(APU.DSP [APU_FLG] & 0x20);
  SoundData.echo_channel_enable = APU.DSP [APU_EON];
  S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
  S9xSetEchoFeedback ((signed char) APU.DSP [APU_EFB]);

  S9xSetFilterCoefficient (0, (signed char) APU.DSP [APU_C0]);
  S9xSetFilterCoefficient (1, (signed char) APU.DSP [APU_C1]);
  S9xSetFilterCoefficient (2, (signed char) APU.DSP [APU_C2]);
  S9xSetFilterCoefficient (3, (signed char) APU.DSP [APU_C3]);
  S9xSetFilterCoefficient (4, (signed char) APU.DSP [APU_C4]);
  S9xSetFilterCoefficient (5, (signed char) APU.DSP [APU_C5]);
  S9xSetFilterCoefficient (6, (signed char) APU.DSP [APU_C6]);
  S9xSetFilterCoefficient (7, (signed char) APU.DSP [APU_C7]);
  for (int i = 0; i < 8; i++)
    {
      SoundData.channels[i].needs_decode = TRUE;
      S9xSetSoundFrequency (i, SoundData.channels[i].hertz);
      SoundData.channels [i].envxx = SoundData.channels [i].envx << ENVX_SHIFT;
      SoundData.channels [i].next_sample = 0;
      SoundData.channels [i].interpolate = 0;
      SoundData.channels [i].latch_noise = 0;
    }
  SoundData.master_volume [0] = SoundData.master_volume_left;
  SoundData.master_volume [1] = SoundData.master_volume_right;
  SoundData.echo_volume [0] = SoundData.echo_volume_left;
  SoundData.echo_volume [1] = SoundData.echo_volume_right;
  IAPU.Scanline = 0;
}

void S9xSetFilterCoefficient (int tap, int value)
{
  FilterTaps [tap & 7] = value;
  SoundData.no_filter = (FilterTaps [0] == 127 || FilterTaps [0] == 0) && 
    FilterTaps [1] == 0   &&
    FilterTaps [2] == 0   &&
    FilterTaps [3] == 0   &&
    FilterTaps [4] == 0   &&
    FilterTaps [5] == 0   &&
    FilterTaps [6] == 0   &&
    FilterTaps [7] == 0;
}

void S9xSetSoundADSR (int channel, int attack_rate, int decay_rate,
		      int sustain_rate, int sustain_level, int release_rate)
{
  SoundData.channels[channel].attack_rate = attack_rate;
  SoundData.channels[channel].decay_rate = decay_rate;
  SoundData.channels[channel].sustain_rate = sustain_rate;
  SoundData.channels[channel].release_rate = release_rate;
  SoundData.channels[channel].sustain_level = sustain_level + 1;

  switch (SoundData.channels[channel].state)
    {
    case SOUND_ATTACK:
      S9xSetEnvelopeRate (channel, attack_rate, 1, 127);
      break;

    case SOUND_DECAY:
      S9xSetEnvelopeRate (channel, decay_rate, -1,
			  (MAX_ENVELOPE_HEIGHT * (sustain_level + 1)) >> 3);
      break;
    case SOUND_SUSTAIN:
      S9xSetEnvelopeRate (channel, sustain_rate, -1, 0);
      break;
    }
}

void S9xSetEnvelopeHeight (int channel, int level)
{
  Channel *ch = &SoundData.channels[channel];

  ch->envx = level;
  ch->envxx = level << ENVX_SHIFT;

  ch->left_vol_level = (level * ch->volume_left) / 128;
  ch->right_vol_level = (level * ch->volume_right) / 128;

  if (ch->envx == 0 && ch->state != SOUND_SILENT && ch->state != SOUND_GAIN)
    {
      S9xAPUSetEndOfSample (channel, ch);
    }
}

int S9xGetEnvelopeHeight (int channel)
{
  if (Settings.SoundEnvelopeHeightReading &&
      SoundData.channels[channel].state != SOUND_SILENT &&
      SoundData.channels[channel].state != SOUND_GAIN)
    {
      return (SoundData.channels[channel].envx);
    }
  return (0);
}

#if 1
void S9xSetSoundSample (int, uint16) 
{
}
#else
void S9xSetSoundSample (int channel, uint16 sample_number)
{
  register Channel *ch = &SoundData.channels[channel];

  if (ch->state != SOUND_SILENT && 
      sample_number != ch->sample_number)
    {
      int keep = ch->state;
      ch->state = SOUND_SILENT;
      ch->sample_number = sample_number;
      ch->loop = FALSE;
      ch->needs_decode = TRUE;
      ch->last_block = FALSE;
      ch->previous [0] = ch->previous[1] = 0;
      uint8 *dir = S9xGetSampleAddress (sample_number);
      ch->block_pointer = READ_WORD (dir);
      ch->sample_pointer = 0;
      ch->state = keep;
    }
}
#endif

void S9xSetSoundFrequency (int channel, int hertz)
{
  if (so.playback_rate)
    {
      if (SoundData.channels[channel].type == SOUND_NOISE)
	hertz = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
      SoundData.channels[channel].frequency = (int)
	(((int64) hertz * FIXED_POINT) / so.playback_rate);
    }
}

void S9xSetSoundHertz (int channel, int hertz)
{
  SoundData.channels[channel].hertz = hertz;
  S9xSetSoundFrequency (channel, hertz);
}

void S9xSetSoundType (int channel, int type_of_sound)
{
  SoundData.channels[channel].type = type_of_sound;
}

bool8 S9xSetSoundMute (bool8 mute)
{
  bool8 old = so.mute_sound;
  so.mute_sound = mute;
  return (old);
}


#define DECODE_2_SPL(dst, src, prev0, prev1, out0, out1) \
	  out0 = (*src++) << (intsz - 8);\
	  out1 = out0 << 4;\
	  out0 &= 0xf0000000;\
	  out0 >>= shift;\
	  out1 >>= shift;\
	  out0 += (prev0 * f0 + prev1 * f1) >> 8;\
	  out0 += 32768;\
	  clip = 65535 - out0;\
	  out0 |= (clip >> (intsz-1));\
	  out0 &= ~ ((65535-clip) >> (intsz-1));\
	  out0 = (out0 & 0xFFFF) - 0x8000;\
	  out1 += (out0 * f0 + prev0 * f1) >> 8;\
	  clip = (unsigned int)(out1+32767) > 65535;\
	  out1 += 32768;\
	  clip = 65535 - out1;\
	  out1 |= (clip >> (intsz-1));\
	  out1 &= ~ ((65535-clip) >> (intsz-1));\
	  out1 = (out1 & 0xFFFF) - 0x8000;\
          *dst++ = (out1<<16) | (out0&0xffff)


static void DecodeBlock(int32 *dst, int8 * compressed, Channel *ch)
{
  int filter = *(unsigned char *)compressed;
  const int intsz  = (sizeof(int)<<3);
  const int shift  = (intsz - 4) - (filter >> 4);
  filter = (filter >> 2) & 3;
  int prv0 = ch->previous [0];
  int prv1 = ch->previous [1];
  const int f0 = FilterValues[filter][0];
  const int f1 = FilterValues[filter][1];

  if ((ch->last_block = filter & 1)) {
    ch->loop = (filter & 2) >> 1;
  }

  int out0, out1, clip;
  ++compressed;
  DECODE_2_SPL(dst, compressed, prv0, prv1, out0, out1);
  DECODE_2_SPL(dst, compressed, out1, out0, prv1, prv0);
  DECODE_2_SPL(dst, compressed, prv0, prv1, out0, out1);
  DECODE_2_SPL(dst, compressed, out1, out0, prv1, prv0);
  DECODE_2_SPL(dst, compressed, prv0, prv1, out0, out1);
  DECODE_2_SPL(dst, compressed, out1, out0, prv1, prv0);
  DECODE_2_SPL(dst, compressed, prv0, prv1, out0, out1);
  DECODE_2_SPL(dst, compressed, out1, out0, prv1, prv0);
  ch->previous [0] = prv0;
  ch->previous [1] = prv1;
}

void DecodeBlock (Channel *ch)
{
  if (ch->block_pointer > 0x10000 - 9) {
    ch->last_block = TRUE;
    ch->loop = FALSE;
    ch->block = (short int *)ch->decoded;
    return;
  }

  int8 *compressed = (int8 *) &IAPU.RAM [ch->block_pointer];

  ch->block = (int16 *)ch->decoded;
  DecodeBlock(ch->decoded, compressed, ch);

  if ((ch->block_pointer += 9) >= 0x10000 - 9) {
    ch->last_block = TRUE;
    ch->loop = FALSE;
    ch->block_pointer -= 0x10000 - 9;
  }
}

static void MixStereoVoice2 (int J, int * wave, int sample_count)
{
#if 0

  Channel *ch = &SoundData.channels[J];
  unsigned long freq0 = ch->frequency;
  int pitch_mod = SoundData.pitch_mod & ~APU.DSP[APU_NON];
  bool8 mod = pitch_mod & (1 << J);
  int32 VL, VR;


  uint block = ch->>block_pointer;

  if (ch->needs_decode) {
  }


  while (sample_count) {
    

    



  if (ch->needs_decode)  {
    DecodeBlock(ch);
    ch->needs_decode = FALSE;
    ch->sample = ch->block[0];
    ch->sample_pointer = freq0 >> FIXED_POINT_SHIFT;
    if (ch->sample_pointer == 0)
      ch->sample_pointer = 1;
    if (ch->sample_pointer > SOUND_DECODE_LENGTH)
      ch->sample_pointer = SOUND_DECODE_LENGTH - 1;
    
    ch->next_sample = ch->block[ch->sample_pointer];
    ch->interpolate = 0;

    if (Settings.InterpolatedSound && freq0 < FIXED_POINT && !mod)
      ch->interpolate = ((ch->next_sample - ch->sample) * 
			 (long) freq0) / (long) FIXED_POINT;
  }

  VL = (ch->sample * ch-> left_vol_level) >> 7;
  VR = (ch->sample * ch->right_vol_level) >> 7;

  for (unsigned int  I = 0; I < (unsigned int) sample_count; I += 2) {
    unsigned long freq = freq0;

    if (mod)
      freq = PITCH_MOD(freq, wave [I / 2]);

    ch->env_error += ch->erate;
    if (ch->env_error >= FIXED_POINT) 
      {
	uint32 step = ch->env_error >> FIXED_POINT_SHIFT;

	switch (ch->state)
	  {
	  case SOUND_ATTACK:
	    ch->env_error &= FIXED_POINT_REMAINDER;
	    ch->envx += step << 1;
	    ch->envxx = ch->envx << ENVX_SHIFT;

	    if (ch->envx >= 126)
	      {
		ch->envx = 127;
		ch->envxx = 127 << ENVX_SHIFT;
		ch->state = SOUND_DECAY;
		if (ch->sustain_level != 8) 
		  {
		    S9xSetEnvRate (ch, ch->decay_rate, -1,
				   (MAX_ENVELOPE_HEIGHT * ch->sustain_level)
				   >> 3);
		    break;
		  }
		ch->state = SOUND_SUSTAIN;
		S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
	      }
	    break;
		
	  case SOUND_DECAY:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx = (ch->envxx >> 8) * 255;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= ch->envx_target)
	      {
		if (ch->envx <= 0)
		  {
		    S9xAPUSetEndOfSample (J, ch);
		    goto stereo_exit;
		  }
		ch->state = SOUND_SUSTAIN;
		S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
	      }
	    break;

	  case SOUND_SUSTAIN:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx = (ch->envxx >> 8) * 255;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;
		    
	  case SOUND_RELEASE:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;
		
	  case SOUND_INCREASE_LINEAR:
	    ch->env_error &= FIXED_POINT_REMAINDER;
	    ch->envx += step << 1;
	    ch->envxx = ch->envx << ENVX_SHIFT;

	    if (ch->envx >= 126)
	      {
		ch->envx = 127;
		ch->envxx = 127 << ENVX_SHIFT;
		ch->state = SOUND_GAIN;
		ch->mode = MODE_GAIN;
		S9xSetEnvRate (ch, 0, -1, 0);
	      }
	    break;

	  case SOUND_INCREASE_BENT_LINE:
	    if (ch->envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4)
	      {
		while (ch->env_error >= FIXED_POINT)
		  {
		    ch->envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
		    ch->env_error -= FIXED_POINT;
		  }
		ch->envx = ch->envxx >> ENVX_SHIFT;
	      }
	    else
	      {
		ch->env_error &= FIXED_POINT_REMAINDER;
		ch->envx += step << 1;
		ch->envxx = ch->envx << ENVX_SHIFT;
	      }

	    if (ch->envx >= 126)
	      {
		ch->envx = 127;
		ch->envxx = 127 << ENVX_SHIFT;
		ch->state = SOUND_GAIN;
		ch->mode = MODE_GAIN;
		S9xSetEnvRate (ch, 0, -1, 0);
	      }
	    break;

	  case SOUND_DECREASE_LINEAR:
	    ch->env_error &= FIXED_POINT_REMAINDER;
	    ch->envx -= step << 1;
	    ch->envxx = ch->envx << ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;

	  case SOUND_DECREASE_EXPONENTIAL:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx = (ch->envxx >> 8) * 255;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;
		
	  case SOUND_GAIN:
	    S9xSetEnvRate (ch, 0, -1, 0);
	    break;
	  }
	ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
	ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
	VL = (ch->sample * ch-> left_vol_level) / 128;
	VR = (ch->sample * ch->right_vol_level) / 128;
      }

    ch->count += freq;
    if (ch->count >= FIXED_POINT)
      {
	VL = ch->count >> FIXED_POINT_SHIFT;
	ch->sample_pointer += VL;
	ch->count &= FIXED_POINT_REMAINDER;

	ch->sample = ch->next_sample;
	if (ch->sample_pointer >= SOUND_DECODE_LENGTH)
	  {
	    if (JUST_PLAYED_LAST_SAMPLE(ch))
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    do
	      {
		ch->sample_pointer -= SOUND_DECODE_LENGTH;
		if (ch->last_block)
		  {
		    if (!ch->loop)
		      {
			ch->sample_pointer = LAST_SAMPLE;
			ch->next_sample = ch->sample;
			break;
		      }
		    else
		      {
			S9xAPUSetEndX (J);
			ch->last_block = FALSE;
			uint8 *dir = S9xGetSampleAddress (ch->sample_number);
			ch->block_pointer = READ_WORD(dir + 2);
		      }
		  }
		DecodeBlock (ch);
	      } while (ch->sample_pointer >= SOUND_DECODE_LENGTH);
	    if (!JUST_PLAYED_LAST_SAMPLE (ch))
	      ch->next_sample = ch->block [ch->sample_pointer];
	  }
	else
	  ch->next_sample = ch->block [ch->sample_pointer];

	if (ch->type == SOUND_SAMPLE)
	  {
	    if (Settings.InterpolatedSound && freq < FIXED_POINT && !mod)
	      {
		ch->interpolate = ((ch->next_sample - ch->sample) * 
				   (long) freq) / (long) FIXED_POINT;
		ch->sample = ch->sample + (((ch->next_sample - ch->sample) * 
					    (long) (ch->count)) / (long) FIXED_POINT);
	      }		  
	    else
	      ch->interpolate = 0;
	  }
	else
	  if (ch->type == SOUND_NOISE) 
	    {
	      for (;VL > 0; VL--)
		if ((so.noise_gen <<= 1) & 0x80000000L)
		  so.noise_gen ^= 0x0040001L;
	      ch->sample = (so.noise_gen << 17) >> 17;
	      ch->interpolate = 0;
	    }
	  else
	    if (ch->type == SOUND_EXTRA_NOISE)
	      {
		static int z = 0x45826444;
		static int r = 0;
		if ((z <<= 1) & 0x80000000)
		  z ^= 0x40001;

		r = z;
		ch->sample = 0x7fff - (r & 0xffff);
		ch->interpolate = 0;
	      }

	VL = (ch->sample * ch-> left_vol_level) / 128;
	VR = (ch->sample * ch->right_vol_level) / 128;
      }
    else
      {
	if (ch->interpolate)
	  {
	    int32 s = (int32) ch->sample + ch->interpolate;
		    
	    CLIP16(s);
	    ch->sample = s;
	    VL = (ch->sample * ch-> left_vol_level) / 128;
	    VR = (ch->sample * ch->right_vol_level) / 128;
	  }
      }

    if (pitch_mod & (1 << (J + 1)))
      wave [I / 2] = ch->sample * ch->envx;

    MixBuffer [I  ] += VL;
    MixBuffer [I+1] += VR;
    ch->echo_buf_ptr [I  ] += VL;
    ch->echo_buf_ptr [I+1] += VR;
  }
 stereo_exit: ;
#endif

}

static void MixStereoVoice (int J, int * wave, int sample_count)
{
  Channel *ch = &SoundData.channels[J];
  unsigned long freq0 = ch->frequency;
  int pitch_mod = SoundData.pitch_mod & ~APU.DSP[APU_NON];
  bool8 mod = pitch_mod & (1 << J);
  int32 VL, VR;

  if (ch->needs_decode)  {
    DecodeBlock(ch);
    ch->needs_decode = FALSE;
    ch->sample = ch->block[0];
    ch->sample_pointer = freq0 >> FIXED_POINT_SHIFT;
    if (ch->sample_pointer == 0)
      ch->sample_pointer = 1;
    if (ch->sample_pointer > SOUND_DECODE_LENGTH)
      ch->sample_pointer = SOUND_DECODE_LENGTH - 1;
    
    ch->next_sample = ch->block[ch->sample_pointer];
    ch->interpolate = 0;

    if (Settings.InterpolatedSound && freq0 < FIXED_POINT && !mod)
      ch->interpolate = ((ch->next_sample - ch->sample) * 
			 (long) freq0) / (long) FIXED_POINT;
  }

  VL = (ch->sample * ch-> left_vol_level) >> 7;
  VR = (ch->sample * ch->right_vol_level) >> 7;

  for (unsigned int  I = 0; I < (unsigned int) sample_count; I += 2) {
    unsigned long freq = freq0;

    if (mod)
      freq = PITCH_MOD(freq, wave [I / 2]);

    ch->env_error += ch->erate;
    if (ch->env_error >= FIXED_POINT) 
      {
	uint32 step = ch->env_error >> FIXED_POINT_SHIFT;

	switch (ch->state)
	  {
	  case SOUND_ATTACK:
	    ch->env_error &= FIXED_POINT_REMAINDER;
	    ch->envx += step << 1;
	    ch->envxx = ch->envx << ENVX_SHIFT;

	    if (ch->envx >= 126)
	      {
		ch->envx = 127;
		ch->envxx = 127 << ENVX_SHIFT;
		ch->state = SOUND_DECAY;
		if (ch->sustain_level != 8) 
		  {
		    S9xSetEnvRate (ch, ch->decay_rate, -1,
				   (MAX_ENVELOPE_HEIGHT * ch->sustain_level)
				   >> 3);
		    break;
		  }
		ch->state = SOUND_SUSTAIN;
		S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
	      }
	    break;
		
	  case SOUND_DECAY:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx = (ch->envxx >> 8) * 255;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= ch->envx_target)
	      {
		if (ch->envx <= 0)
		  {
		    S9xAPUSetEndOfSample (J, ch);
		    goto stereo_exit;
		  }
		ch->state = SOUND_SUSTAIN;
		S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
	      }
	    break;

	  case SOUND_SUSTAIN:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx = (ch->envxx >> 8) * 255;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;
		    
	  case SOUND_RELEASE:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;
		
	  case SOUND_INCREASE_LINEAR:
	    ch->env_error &= FIXED_POINT_REMAINDER;
	    ch->envx += step << 1;
	    ch->envxx = ch->envx << ENVX_SHIFT;

	    if (ch->envx >= 126)
	      {
		ch->envx = 127;
		ch->envxx = 127 << ENVX_SHIFT;
		ch->state = SOUND_GAIN;
		ch->mode = MODE_GAIN;
		S9xSetEnvRate (ch, 0, -1, 0);
	      }
	    break;

	  case SOUND_INCREASE_BENT_LINE:
	    if (ch->envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4)
	      {
		while (ch->env_error >= FIXED_POINT)
		  {
		    ch->envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
		    ch->env_error -= FIXED_POINT;
		  }
		ch->envx = ch->envxx >> ENVX_SHIFT;
	      }
	    else
	      {
		ch->env_error &= FIXED_POINT_REMAINDER;
		ch->envx += step << 1;
		ch->envxx = ch->envx << ENVX_SHIFT;
	      }

	    if (ch->envx >= 126)
	      {
		ch->envx = 127;
		ch->envxx = 127 << ENVX_SHIFT;
		ch->state = SOUND_GAIN;
		ch->mode = MODE_GAIN;
		S9xSetEnvRate (ch, 0, -1, 0);
	      }
	    break;

	  case SOUND_DECREASE_LINEAR:
	    ch->env_error &= FIXED_POINT_REMAINDER;
	    ch->envx -= step << 1;
	    ch->envxx = ch->envx << ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;

	  case SOUND_DECREASE_EXPONENTIAL:
	    while (ch->env_error >= FIXED_POINT)
	      {
		ch->envxx = (ch->envxx >> 8) * 255;
		ch->env_error -= FIXED_POINT;
	      }
	    ch->envx = ch->envxx >> ENVX_SHIFT;
	    if (ch->envx <= 0)
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    break;
		
	  case SOUND_GAIN:
	    S9xSetEnvRate (ch, 0, -1, 0);
	    break;
	  }
	ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
	ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
	VL = (ch->sample * ch-> left_vol_level) / 128;
	VR = (ch->sample * ch->right_vol_level) / 128;
      }

    ch->count += freq;
    if (ch->count >= FIXED_POINT)
      {
	VL = ch->count >> FIXED_POINT_SHIFT;
	ch->sample_pointer += VL;
	ch->count &= FIXED_POINT_REMAINDER;

	ch->sample = ch->next_sample;
	if (ch->sample_pointer >= SOUND_DECODE_LENGTH)
	  {
	    if (JUST_PLAYED_LAST_SAMPLE(ch))
	      {
		S9xAPUSetEndOfSample (J, ch);
		goto stereo_exit;
	      }
	    do
	      {
		ch->sample_pointer -= SOUND_DECODE_LENGTH;
		if (ch->last_block)
		  {
		    if (!ch->loop)
		      {
			ch->sample_pointer = LAST_SAMPLE;
			ch->next_sample = ch->sample;
			break;
		      }
		    else
		      {
			S9xAPUSetEndX (J);
			ch->last_block = FALSE;
			uint8 *dir = S9xGetSampleAddress (ch->sample_number);
			ch->block_pointer = READ_WORD(dir + 2);
		      }
		  }
		DecodeBlock (ch);
	      } while (ch->sample_pointer >= SOUND_DECODE_LENGTH);
	    if (!JUST_PLAYED_LAST_SAMPLE (ch))
	      ch->next_sample = ch->block [ch->sample_pointer];
	  }
	else
	  ch->next_sample = ch->block [ch->sample_pointer];

	if (ch->type == SOUND_SAMPLE)
	  {
	    if (Settings.InterpolatedSound && freq < FIXED_POINT && !mod)
	      {
		ch->interpolate = ((ch->next_sample - ch->sample) * 
				   (long) freq) / (long) FIXED_POINT;
		ch->sample = ch->sample + (((ch->next_sample - ch->sample) * 
					    (long) (ch->count)) / (long) FIXED_POINT);
	      }		  
	    else
	      ch->interpolate = 0;
	  }
	else
	  if (ch->type == SOUND_NOISE) 
	    {
	      for (;VL > 0; VL--)
		if ((so.noise_gen <<= 1) & 0x80000000L)
		  so.noise_gen ^= 0x0040001L;
	      ch->sample = (so.noise_gen << 17) >> 17;
	      ch->interpolate = 0;
	    }
	  else
	    if (ch->type == SOUND_EXTRA_NOISE)
	      {
		static int z = 0x45826444;
		static int r = 0;
		if ((z <<= 1) & 0x80000000)
		  z ^= 0x40001;

		r = z;
		ch->sample = 0x7fff - (r & 0xffff);
		ch->interpolate = 0;
	      }

	VL = (ch->sample * ch-> left_vol_level) / 128;
	VR = (ch->sample * ch->right_vol_level) / 128;
      }
    else
      {
	if (ch->interpolate)
	  {
	    int32 s = (int32) ch->sample + ch->interpolate;
		    
	    CLIP16(s);
	    ch->sample = s;
	    VL = (ch->sample * ch-> left_vol_level) / 128;
	    VR = (ch->sample * ch->right_vol_level) / 128;
	  }
      }

    if (pitch_mod & (1 << (J + 1)))
      wave [I / 2] = ch->sample * ch->envx;

    MixBuffer [I  ] += VL;
    MixBuffer [I+1] += VR;
    ch->echo_buf_ptr [I  ] += VL;
    ch->echo_buf_ptr [I+1] += VR;
  }
 stereo_exit: ;
}

void MixStereo (int sample_count)
{
  int wave[SOUND_BUFFER_SIZE];

  for (int J = 0; J < NUM_CHANNELS; J++) 
    {
      Channel *ch = &SoundData.channels[J];

      if (ch->state == SOUND_SILENT || !(so.sound_switch & (1 << J)))
	continue;
      
      MixStereoVoice(J, wave, sample_count);
    }
}


// For backwards compatibility with older port specific code
void S9xMixSamples (uint8 *buffer, int sample_count)
{
  S9xMixSamplesO (buffer, sample_count, 0);
}

void S9xMixSamplesO (uint8 *buffer, int sample_count, int byte_offset)
{
  int J;
  int I;

  if (!so.mute_sound)
    {
      memset (MixBuffer, 0, sample_count * sizeof (MixBuffer [0]));
      if (SoundData.echo_enable)
	memset (EchoBuffer, 0, sample_count * sizeof (EchoBuffer [0]));

      if (so.stereo)
	MixStereo (sample_count);
      else
	*(int*)1 = 0xDEAD15da;
    }

  /* Mix and convert waveforms */
  if (so.sixteen_bit)
    {
      int byte_count = sample_count << 1;
	
      // 16-bit sound
      if (so.mute_sound)
	{
	  memset (buffer + byte_offset, 0, byte_count);
	}
      else
	{
	  int O = byte_offset >> 1;
	  if (SoundData.echo_enable && SoundData.echo_buffer_size)
	    {
	      if (so.stereo)
		{
		  // 16-bit stereo sound with echo enabled ...
		  if (SoundData.no_filter)
		    {
		      // ... but no filter defined.
		      for (J = 0; J < sample_count; J++)
			{
			  int E = Echo [SoundData.echo_ptr];

			  Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
			    EchoBuffer [J];

			  if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
			    SoundData.echo_ptr = 0;

			  I = (MixBuffer [J] * 
			       SoundData.master_volume [J & 1] +
			       E * SoundData.echo_volume [J & 1]) / VOL_DIV16;

			  CLIP16(I);
			  ((signed short *) buffer)[J + O] = I;
			}
		    }
		  else
		    {
		      // ... with filter defined.
		      for (J = 0; J < sample_count; J++)
			{
			  int E = Echo [SoundData.echo_ptr];

			  Loop [(Z - 0) & 15] = E;
			  E =  E                    * FilterTaps [0];
			  E += Loop [(Z -  2) & 15] * FilterTaps [1];
			  E += Loop [(Z -  4) & 15] * FilterTaps [2];
			  E += Loop [(Z -  6) & 15] * FilterTaps [3];
			  E += Loop [(Z -  8) & 15] * FilterTaps [4];
			  E += Loop [(Z - 10) & 15] * FilterTaps [5];
			  E += Loop [(Z - 12) & 15] * FilterTaps [6];
			  E += Loop [(Z - 14) & 15] * FilterTaps [7];
			  E /= 128;
			  Z++;

			  Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
			    EchoBuffer [J];

			  if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
			    SoundData.echo_ptr = 0;

			  I = (MixBuffer [J] * 
			       SoundData.master_volume [J & 1] +
			       E * SoundData.echo_volume [J & 1]) / VOL_DIV16;

			  CLIP16(I);
			  ((signed short *) buffer)[J + O] = I;
			}
		    }
		}
	      else
		{
		  // 16-bit mono sound with echo enabled...
		  if (SoundData.no_filter)
		    {
		      // ... no filter defined
		      for (J = 0; J < sample_count; J++)
			{
			  int E = Echo [SoundData.echo_ptr];

			  Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
			    EchoBuffer [J];

			  if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
			    SoundData.echo_ptr = 0;

			  I = (MixBuffer [J] *
			       SoundData.master_volume [0] +
			       E * SoundData.echo_volume [0]) / VOL_DIV16;
			  CLIP16(I);
			  ((signed short *) buffer)[J + O] = I;
			}
		    }
		  else
		    {
		      // ... with filter defined
		      for (J = 0; J < sample_count; J++)
			{
			  int E = Echo [SoundData.echo_ptr];

			  Loop [(Z - 0) & 7] = E;
			  E =  E                  * FilterTaps [0];
			  E += Loop [(Z - 1) & 7] * FilterTaps [1];
			  E += Loop [(Z - 2) & 7] * FilterTaps [2];
			  E += Loop [(Z - 3) & 7] * FilterTaps [3];
			  E += Loop [(Z - 4) & 7] * FilterTaps [4];
			  E += Loop [(Z - 5) & 7] * FilterTaps [5];
			  E += Loop [(Z - 6) & 7] * FilterTaps [6];
			  E += Loop [(Z - 7) & 7] * FilterTaps [7];
			  E /= 128;
			  Z++;

			  Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
			    EchoBuffer [J];

			  if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
			    SoundData.echo_ptr = 0;

			  I = (MixBuffer [J] * SoundData.master_volume [0] +
			       E * SoundData.echo_volume [0]) / VOL_DIV16;
			  CLIP16(I);
			  ((signed short *) buffer)[J + O] = I;
			}
		    }
		}
	    }
	  else
	    {
	      // 16-bit mono or stereo sound, no echo
	      for (J = 0; J < sample_count; J++)
		{
		  I = (MixBuffer [J] * 
		       SoundData.master_volume [J & 1]) / VOL_DIV16;

		  CLIP16(I);
		  ((signed short *) buffer)[J + O] = I;
		}
	    }
	}
    }
  else
    {
      *(int*)1 = 0xDEADBA65;
    }
}


void S9xResetSound (bool8 full)
{
  for (int i = 0; i < 8; i++)
    {
      SoundData.channels[i].state = SOUND_SILENT;
      SoundData.channels[i].mode = MODE_NONE;
      SoundData.channels[i].type = SOUND_SAMPLE;
      SoundData.channels[i].volume_left = 0;
      SoundData.channels[i].volume_right = 0;
      SoundData.channels[i].hertz = 0;
      SoundData.channels[i].count = 0;
      SoundData.channels[i].loop = FALSE;
      SoundData.channels[i].envx_target = 0;
      SoundData.channels[i].env_error = 0;
      SoundData.channels[i].erate = 0;
      SoundData.channels[i].envx = 0;
      SoundData.channels[i].envxx = 0;
      SoundData.channels[i].left_vol_level = 0;
      SoundData.channels[i].right_vol_level = 0;
      SoundData.channels[i].direction = 0;
      SoundData.channels[i].attack_rate = 0;
      SoundData.channels[i].decay_rate = 0;
      SoundData.channels[i].sustain_rate = 0;
      SoundData.channels[i].release_rate = 0;
      SoundData.channels[i].sustain_level = 0;
      SoundData.channels[i].latch_noise = 0;
      SoundData.echo_ptr = 0;
      SoundData.echo_feedback = 0;
      SoundData.echo_buffer_size = 1;
    }
  FilterTaps [0] = 127;
  FilterTaps [1] = 0;
  FilterTaps [2] = 0;
  FilterTaps [3] = 0;
  FilterTaps [4] = 0;
  FilterTaps [5] = 0;
  FilterTaps [6] = 0;
  FilterTaps [7] = 0;
  so.mute_sound = TRUE;
  so.noise_gen = 1;
  so.sound_switch = 255;
  so.samples_mixed_so_far = 0;
  so.play_position = 0;
  so.err_counter = 0;
  if (so.playback_rate)
    so.err_rate = (uint32) (FIXED_POINT * SNES_SCANLINE_TIME / (1.0 / so.playback_rate));
  else
    so.err_rate = 0;
  SoundData.no_filter = TRUE;
}

// Change globale playback replay frequency ...
void S9xSetPlaybackRate (uint32 playback_rate)
{
  so.playback_rate = playback_rate;
  so.err_rate = (uint32) (SNES_SCANLINE_TIME * FIXED_POINT / (1.0 / (double) so.playback_rate));
  S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
  for (int i = 0; i < 8; i++)
    S9xSetSoundFrequency (i, SoundData.channels [i].hertz);
}


bool8 S9xSetSoundMode (int channel, int mode)
{
  Channel *ch = &SoundData.channels[channel];

  switch (mode)
    {
    case MODE_RELEASE:
      if (ch->mode != MODE_NONE)
	{
	  ch->mode = MODE_RELEASE;
	  return (TRUE);
	}
      break;
	
    case MODE_DECREASE_LINEAR:
    case MODE_DECREASE_EXPONENTIAL:
    case MODE_GAIN:
      if (ch->mode != MODE_RELEASE)
	{
	  ch->mode = mode;
	  if (ch->state != SOUND_SILENT)
	    ch->state = mode;

	  return (TRUE);
	}
      break;

    case MODE_INCREASE_LINEAR:
    case MODE_INCREASE_BENT_LINE:
      if (ch->mode != MODE_RELEASE)
	{
	  ch->mode = mode;
	  if (ch->state != SOUND_SILENT)
	    ch->state = mode;

	  return (TRUE);
	}
      break;

    case MODE_ADSR:
      if (ch->mode == MODE_NONE || ch->mode == MODE_ADSR)
	{
	  ch->mode = mode;
	  return (TRUE);
	}
    }

  return (FALSE);
}

void S9xSetSoundControl (int sound_switch)
{
  so.sound_switch = sound_switch;
}

void S9xPlaySample (int channel)
{
  Channel *ch = &SoundData.channels[channel];
    
  ch->state = SOUND_SILENT;
  ch->mode = MODE_NONE;
  ch->envx = 0;
  ch->envxx = 0;

  S9xFixEnvelope (channel,
		  APU.DSP [APU_GAIN  + (channel << 4)], 
		  APU.DSP [APU_ADSR1 + (channel << 4)],
		  APU.DSP [APU_ADSR2 + (channel << 4)]);

  ch->sample_number = APU.DSP [APU_SRCN + channel * 0x10];
  ch->latch_noise = FALSE;
  if (APU.DSP [APU_NON] & (1 << channel))
    ch->type = SOUND_NOISE;
  else
    ch->type = SOUND_SAMPLE;

  S9xSetSoundFrequency (channel, ch->hertz);
  ch->loop = FALSE;
  ch->needs_decode = TRUE;
  ch->last_block = FALSE;
  ch->previous [0] = ch->previous[1] = 0;
  uint8 *dir = S9xGetSampleAddress (ch->sample_number);
  ch->block_pointer = READ_WORD (dir);
  ch->sample_pointer = 0;
  ch->env_error = 0;
  ch->next_sample = 0;
  ch->interpolate = 0;

  switch (ch->mode)
    {
    case MODE_ADSR:
      if (ch->attack_rate == 0)
	{
	  if (ch->decay_rate == 0 || ch->sustain_level == 8)
	    {
	      ch->state = SOUND_SUSTAIN;
	      ch->envx = (MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3;
	      S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
	    }
	  else
	    {
	      ch->state = SOUND_DECAY;
	      ch->envx = MAX_ENVELOPE_HEIGHT;
	      S9xSetEnvRate (ch, ch->decay_rate, -1, 
			     (MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3);
	    }
	  ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
	  ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
	}
      else
	{
	  ch->state = SOUND_ATTACK;
	  ch->envx = 0;
	  ch->left_vol_level = 0;
	  ch->right_vol_level = 0;
	  S9xSetEnvRate (ch, ch->attack_rate, 1, MAX_ENVELOPE_HEIGHT);
	}
      ch->envxx = ch->envx << ENVX_SHIFT;
      break;

    case MODE_GAIN:
      ch->state = SOUND_GAIN;
      break;

    case MODE_INCREASE_LINEAR:
      ch->state = SOUND_INCREASE_LINEAR;
      break;

    case MODE_INCREASE_BENT_LINE:
      ch->state = SOUND_INCREASE_BENT_LINE;
      break;

    case MODE_DECREASE_LINEAR:
      ch->state = SOUND_DECREASE_LINEAR;
      break;

    case MODE_DECREASE_EXPONENTIAL:
      ch->state = SOUND_DECREASE_EXPONENTIAL;
      break;

    default:
      break;
    }

  S9xFixEnvelope (channel,
		  APU.DSP [APU_GAIN  + (channel << 4)], 
		  APU.DSP [APU_ADSR1 + (channel << 4)],
		  APU.DSP [APU_ADSR2 + (channel << 4)]);
}
