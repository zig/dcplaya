#include "sha123/header.h"
#include "sha123/debug.h"

/* Bitrate table */
static const unsigned int bitrates[2][3][16] = {
  {
    {  0, 32, 64, 96,128,160,192,224,256,288,320,352,384,416,448},
    {  0, 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384},
    {  0, 32, 40, 48, 56, 64, 80, 96,112,128,160,192,224,256,320}
  },
  {
    {  0, 32, 48, 56, 64, 80, 96,112,128,144,160,176,192,224,256},
    {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160},
    {  0,  8, 16, 24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160}
  }
};

/* Frequency table */
static const unsigned int freqs[9] = {
  44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000
};

const char * sha123_modestr(unsigned int mode)
{
  static const char *mode_str[4] = {
    "stereo", "joint-stereo","dual-channel","mono"
  };
  return (mode < 4) ? mode_str[mode] : "invalid";
}

int sha123_header_check(const sha123_header_t head)
{
  return
    - (0
       || (head.sync7ff != 0x7ff)
       || (head.option < 1)
       || (head.option > 3)
       || (!head.br_index)
       || (head.br_index == 15)
       || (head.sr_index == 3)
       || (head.id && head.option == 3 && head.not_protected)
       );
#if 0
  return
    - (0
       || ((head & 0xffe00000) != 0xffe00000)
       || (!((head >> 17) & 3))
       || (((head >> 12) & 0xf) == 0xf)
       || (!((head >> 12) & 0xf))
       || (((head >> 10) & 0x3) == 0x3)
       || (((head >> 19) & 1) == 1 &&
	   ((head >> 17) & 3) == 3 &&
	   ((head >> 16) & 1) == 1)
       || ((head & 0xffff0000) == 0xfffe0000)
       );
#endif
}

int sha123_decode_header(sha123_header_info_t * info,
			 const sha123_header_t head)
{
  unsigned int frq_idx;
  unsigned int frq_div;
  unsigned int lsf;
  unsigned int layer;
  unsigned int ssize;
  unsigned int framesize;
  unsigned int log2chan;
  unsigned int bitrate;

  lsf = (head.not_mpg25 & head.id) ^ 1; /* Ok */
  frq_idx = head.sr_index + (head.not_mpg25 ? (-lsf&3) : 6); /* Ok */
  layer = 4 - head.option; /* Ok */
  ssize = 0;
  log2chan = head.mode != MPG_MD_MONO;
  bitrate = framesize = bitrates[lsf][layer-1][head.br_index];
  frq_div = freqs[frq_idx];

  switch (layer) {
  case 1:
    framesize *= 12000u;
    framesize /= frq_div;
    framesize = ((framesize + head.pad) << 2) - 4;
    break;
  case 2:
    framesize *= 144000u;
    framesize /= frq_div;
    framesize += head.pad - 4;
    break;
  case 3: {
    static const unsigned int ssize_tbl[4] = { 17, 32, 9, 17 };
    ssize = ssize_tbl[(lsf<<1)+log2chan];
    ssize += (1-head.not_protected) << 1;
    framesize *= 144000u;
    framesize /= frq_div << lsf;
    framesize += head.pad - 4 - ssize;

  } break;
  default:
    sha123_debug("!!! INTERNAL error : wrong layer type (%d)\n", layer);
/*     sha123_set_error(sha123, "sha123_decode_header : invalid layer type"); */
    framesize = 0;
  }

#ifdef SHA123_PARANO
  if (framesize < 0) {
    sha123_debug("sha123_decode_header : framesize=%d\n", framesize);
/*     sha123_set_error(sha123, "sha123_decode_header : invalid frame size"); */
    return -1;
  }
#endif

  info->layer = layer;
  info->frame_bytes = framesize;
  info->lsf = lsf;
  info->log2chan = log2chan;
  info->ssize = ssize;
  info->sampling_rate = frq_div;
  info->bit_rate = bitrate;
  info->sampling_idx = frq_idx;

  return -!framesize;
}

static const char * fourcc(unsigned int v)
{
  static char s[8];
  char * s2;
  int i;
  for (s2=s, i=24; i>=0; i -= 8) {
    int j = (v >> i) & 255;
    *s2++ = (j >= 32 && j < 128) ? j : '?';
  }
  *s2 = 0;
  return s;
}

void sha123_header_dump(const sha123_header_t head)
{
#if SHA123_DEBUG
  static const char * yesno[2] = { "Yes", "No" };
  
  sha123_debug("Dump mpeg header:\n");
  sha123_debug(" fourCC    : '%s'\n", fourcc(*(unsigned int *)&head));
  sha123_debug(" sync7ff   : %03X\n", head.sync7ff);
  sha123_debug(" mpeg25    : %s\n", yesno[head.not_mpg25]);
  sha123_debug(" id        : %u\n", head.id);
  sha123_debug(" option    : %u\n", head.option);
  sha123_debug(" protected : %s\n", yesno[head.not_protected]);
  sha123_debug(" br index  : %u\n", head.br_index);
  sha123_debug(" sr index  : %u\n", head.sr_index);
  sha123_debug(" pad       : %u\n", head.pad);
  sha123_debug(" private   : %s\n", yesno[!head.private]);
  sha123_debug(" mode      : %s\n", sha123_modestr(head.mode));
  sha123_debug(" mode-ext  : %u\n", head.mode_ext);
  sha123_debug(" copyright : %s\n", yesno[!head.copyright]);
  sha123_debug(" original  : %s\n", yesno[!head.original]);
  sha123_debug(" emphasis  : %u\n", head.emphasis);
#endif
}

void sha123_header_info_dump(const sha123_header_info_t * info)
{
#if SHA123_DEBUG
  sha123_debug("Dump mpeg header side info:\n");
  sha123_debug(" layer         : %u\n", info->layer);
  sha123_debug(" frame bytes   : %u\n", info->frame_bytes);
  sha123_debug(" lsf           : %u\n", info->lsf);
  sha123_debug(" channels      : %u\n", 1<<info->log2chan);
  sha123_debug(" ssize         : %u\n", info->ssize);
  sha123_debug(" sampling rate : %u hz [%u]\n",
	       info->sampling_rate, info->sampling_idx);
  sha123_debug(" bit rate      : %u kbps\n", info->bit_rate);
#endif
}

