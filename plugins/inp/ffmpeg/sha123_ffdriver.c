
#define NULL 0L

#include "sha123/api.h"
#include "sha123/sha123.h"
#include "istream/istream_def.h"

#include "ffmpeg.h"

#ifndef OLDFF

#define dbmsg (void)

typedef struct context {
  istream_t istream;
  sha123_param_t param;
  sha123_t * decoder;

  int error;

  int gotinfo;

  uint8_t * buf;
  int buf_size;
  int pos;
  int max;
  int total;
} context_t;


static int isopen(istream_t * p)
{
  return 0;
}

static int isclose(istream_t * p)
{
  return 0;
}


static int isread(istream_t * p, void * buf, int len)
{
  context_t * ctx = (context_t *) p;

  dbmsg("read %x %d\n", buf, len);

  if (len > ctx->buf_size - ctx->pos)
    len = ctx->buf_size - ctx->pos;

  if (len) {
    memcpy(buf, ctx->buf + ctx->pos, len);
    ctx->pos += len;
  }

  if (ctx->max < ctx->pos)
    ctx->max = ctx->pos;

  return len;
}

static int istell(istream_t * p)
{
  context_t * ctx = (context_t *) p;

  dbmsg("tell --> %d\n", ctx->pos);

  return ctx->pos; // + ctx->total;
}

static int isseekf(istream_t * p, int pos)
{
  context_t * ctx = (context_t *) p;

  dbmsg("seekf %d\n", pos);

  pos += ctx->pos;
  if (pos > ctx->buf_size)
    return -1; //pos = ctx->buf_size;

  ctx->pos = pos;

  if (ctx->max < ctx->pos)
    ctx->max = ctx->pos;

  return 0;
}

static int isseekb(istream_t * p, int pos)
{
  context_t * ctx = (context_t *) p;

  dbmsg("seekb %d\n", pos);

  pos = ctx->pos + pos;
  if (pos < 0)
    return -1; //pos = 0;

  ctx->pos = pos;

  if (ctx->max < ctx->pos)
    ctx->max = ctx->pos;

  return 0;
}

static int decode_close(AVCodecContext * avctx)
{
  context_t * ctx = (context_t *) avctx->priv_data;

  if (ctx->decoder)
    sha123_stop(ctx->decoder);

  return 0;
}

static int decode_init(AVCodecContext * avctx)
{
  context_t * ctx = (context_t *) avctx->priv_data;

  ctx->error = 0;

  ctx->total = 0;

  ctx->gotinfo = 0;

  ctx->istream.open = isopen;
  ctx->istream.close = isclose;
  ctx->istream.read = isread;
  ctx->istream.tell = istell;
  ctx->istream.seekf = isseekf;
  ctx->istream.seekb = isseekb;

  ctx->param.istream = &ctx->istream;
  ctx->param.loop = 0;

  return 0;

  int res = -1;

  ctx->decoder = sha123_start(&ctx->param);
  if (ctx->decoder == NULL)
    goto error;
  
  res = 0;

 error:
  if (res)
    close(avctx);

  return res;
}

static int decode_frame(AVCodecContext * avctx,
			void *data, int *data_size,
			uint8_t * buf, int buf_size)
{
  context_t * ctx = (context_t *) avctx->priv_data;
  int res = 0;
  int first = 0;

  ctx->buf = buf;
  ctx->buf_size = buf_size;
  ctx->pos = 0;

#ifdef OLDFF
  printf("frame size %d\n", buf_size);
#endif

  if (ctx->decoder == NULL && !ctx->error) {
    ctx->decoder = sha123_start(&ctx->param);
    if (ctx->decoder == NULL)
      ctx->error = 1;
    first = 1;
  }
  if (ctx->decoder) {
    res = sha123_decode(ctx->decoder, data, !first/*AVCODEC_MAX_AUDIO_FRAME_SIZE*/);
    if (res) {
      printf("sha123 : decoder error %d\n", res);
      return -1;
    } else {
      //printf("pcm_point = %d (%d, %d)\n", ctx->decoder->pcm_point, ctx->pos, ctx->max);
      *data_size = ctx->decoder->pcm_point;

#ifdef OLDFF
      if (!ctx->gotinfo) {
	sha123_info_t * info = sha123_info(ctx->decoder);
	if (info) {
	  avctx->channels = info->channels;
	  avctx->sample_rate = info->sampling_rate;
	  printf("channels=%d,rate=%d\n",avctx->channels,avctx->sample_rate);
	  avctx->bit_rate = 0;
	  ctx->gotinfo = 1;
	}
      }
#endif

    }
  }

  ctx->total = ctx->pos;

  return ctx->pos;
}


#if 0
AVCodec shamp2_decoder =
{
    "mp2",
    CODEC_TYPE_AUDIO,
    CODEC_ID_MP2,
    sizeof(context_t),
    decode_init,
    NULL,
    close,
    decode_frame,
    CODEC_CAP_PARSE_ONLY,
};
#endif

AVCodec shamp3_decoder =
{
#ifdef OLDFF
    "mp3",
    CODEC_TYPE_AUDIO,
    CODEC_ID_MP3LAME,
    sizeof(context_t),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
#else
    "mp3",
    CODEC_TYPE_AUDIO,
    CODEC_ID_MP3,
    sizeof(context_t),
    decode_init,
    NULL,
    decode_close,
    decode_frame,
    CODEC_CAP_PARSE_ONLY,
#endif
};

int sha123_ffdriver_init()
{
  return sha123_init();
}

int sha123_ffdriver_shutdown()
{
  sha123_shutdown();
  return 0;
}

#endif
