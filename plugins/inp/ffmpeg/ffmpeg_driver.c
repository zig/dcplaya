/*
 * $Id: ffmpeg_driver.c,v 1.1 2004-06-22 20:21:36 vincentp Exp $
 */

#include "dcplaya/config.h"
#include "extern_def.h"

#include <kos.h>

DCPLAYA_EXTERN_C_START
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
DCPLAYA_EXTERN_C_END

typedef unsigned char u8_t;
typedef signed char s8_t;
typedef unsigned int u32_t;
typedef unsigned short u16_t;


#define ARCH_SH4 1
#define TUNECPU generic
#define EMULATE_INTTYPES 1
#define EMULATE_FAST_INT 1
/*#define CONFIG_ENCODERS 1*/
#define CONFIG_DECODERS 1
#define CONFIG_MPEGAUDIO_HP 1
/* #define CONFIG_VIDEO4LINUX 1 */
/* #define CONFIG_DV1394 1 */
/* #define CONFIG_AUDIO_OSS 1 */
/* #define CONFIG_NETWORK 1 */
#undef  HAVE_MALLOC_H
#undef  HAVE_MEMALIGN
#define SIMPLE_IDCT 1
#define CONFIG_RISKY 1
#define restrict __restrict__

#include "avcodec.h"
#include "avformat.h"
#undef fifo_init
#undef fifo_free
#undef fifo_size
#undef fifo_read
#undef fifo_write

#include "playa.h"
#include "fifo.h"
#include "inp_driver.h"
#include "draw/texture.h"

#include "exceptions.h"

#if 0
#define EXPT_GUARD_BEGIN if (1) {
#define EXPT_GUARD_CATCH } else {
#define EXPT_GUARD_END }
#define EXPT_GUARD_RETURN return
#endif

#define AUDIO_SZ ((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2)

volatile static int ready; /**< Ready flag : 1 when music is playing */

static AVFormatContext *ic = NULL;
static texture_t * btexture;
static AVFrame *frame;
static AVInputFormat *file_iformat;
static AVImageFormat *image_format;

static int audio_sample_rate = 44100;
static int audio_bit_rate = 64000;
static int audio_disable = 0;
static int audio_channels = 1;
static int audio_codec_id = CODEC_ID_NONE;

static int frame_width  = 160;
static int frame_height = 128;
static float frame_aspect_ratio = 0;
static enum PixelFormat frame_pix_fmt = PIX_FMT_YUV420P;
static int frame_padtop  = 0;
static int frame_padbottom = 0;
static int frame_padleft  = 0;
static int frame_padright = 0;
static int padcolor[3] = {16,128,128}; /* default to black */
static int frame_topBand  = 0;
static int frame_bottomBand = 0;
static int frame_leftBand  = 0;
static int frame_rightBand = 0;
static int frame_rate = 25;
static int frame_rate_base = 1;

static int workaround_bugs = FF_BUG_AUTODETECT;
static int error_resilience = 2;
static int error_concealment = 3;
static int dct_algo = 0;
static int idct_algo = 0;

static int rate_emu = 0;

static int bitexact = 0;

static int debug = 0;
static int debug_mv = 0;
static int me_threshold = 0;
static int mb_threshold = 0;

static int verbose = 1;

static uint32_t * audio_buf;
static int audio_size;
static int audio_ptr;


typedef struct AVInputStream {
    AVStream *st;
    int discard;             /* true if stream data should be discarded */
    int decoding_needed;     /* true if the packets must be decoded in 'raw_fifo' */
    int64_t sample_index;      /* current sample */

    int64_t       start;     /* time when read started */
    unsigned long frame;     /* current frame */
    int64_t       next_pts;  /* synthetic pts for cases where pkt.pts
                                is not defined */
    int64_t       pts;       /* current pts */
} AVInputStream;

static int nb_istreams;
static AVInputStream **ist_table = NULL;

static AVStream * video_stream;
static AVStream * audio_stream;

static const char *motion_str[] = {
    "zero",
    "full",
    "log",
    "phods",
    "epzs",
    "x1",
    NULL,
};


static kthread_t * vidstream_thd;
static int vidstream_exit;
void vidstream_thread(void *userdata);



uint64_t toto = 1800000000000UL;
uint64_t toto2 = 1600000000010UL;

static int init(any_driver_t *d)
{
  int n = 0;
  EXPT_GUARD_BEGIN;

  dbglog(DBG_DEBUG, "ffmpeg : Init [%s]\n", d->name);
  ready = 0;

  av_register_all();



  vidstream_thd = thd_create(vidstream_thread, 0);
  //vidstream_thd->prio2 = 9;



  printf("%d\n", toto > toto2);
  printf("%g\n", (float) toto);
  toto >>= 20;
  dummyf();
  printf("%g\n", (float) toto);
  dummyf();
  printf("%d\n", (uint) (toto));





  EXPT_GUARD_CATCH;

  n = -1;

  EXPT_GUARD_END;
  return n;
}

static int stop(void)
{
  ready = 0;

  decode_stop();

  if (audio_buf) {
    free(audio_buf);
    audio_buf = NULL;
  }

  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  EXPT_GUARD_BEGIN;

  stop();

  vidstream_exit = 1;
  while (vidstream_exit)
    thd_pass();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return 0;
}

static int start(const char *fn, int track, playa_info_t *info)
{
  EXPT_GUARD_BEGIN;

  stop();

  audio_buf = (u32_t *) malloc(AUDIO_SZ);

  //goto error;
  if (decode_start(fn))
    goto error;

  playa_info_bps    (info, ic->bit_rate); 
  {
    char buf[128];
    sprintf(buf, "FFMpeg: %s", ic->iformat->name);
    playa_info_desc(info, buf);
  }
  if (audio_stream) {
    char buf[128];

    avcodec_string(buf, sizeof(buf), &audio_stream->codec, 0);
    playa_info_comments   (info, buf);

    //printf("HZ = %d\n", audio_stream->codec.sample_rate);
    playa_info_frq    (info, audio_stream->codec.sample_rate);
    playa_info_stereo (info, audio_stream->codec.channels > 1? 1:0);
  }
  playa_info_bits   (info, 16);
  playa_info_time   (info, ic->duration * 1000 / AV_TIME_BASE);

  ready = 1;

  EXPT_GUARD_RETURN 0;

 error:
  stop();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return -1;
}

static int decoder(playa_info_t *info)
{
  int n = -1;

  EXPT_GUARD_BEGIN;

  if (!ready) {
    EXPT_GUARD_RETURN INP_DECODE_ERROR;
  }
        
  n = decode_frame();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_RETURN INP_DECODE_ERROR;

  EXPT_GUARD_END;
  return n? 0 : INP_DECODE_CONT;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}

static char * mystrdup(const char *s)
{
  if (!s || !*s) {
    return 0;
  } else {
    return strdup(s);
  }
}

void please_use_av_log()
{
  printf("please use av_log !\n");
}

/* static int info(playa_info_t *info, const char *fname) */
/* { */
/*   EXPT_GUARD_BEGIN; */

/*   if (fname) { */
/*     EXPT_GUARD_RETURN id_info(info, 0); */
/*   } else { */
/*     EXPT_GUARD_RETURN id_info(info, 0); */
/*   } */

/*   EXPT_GUARD_CATCH; */

/*   EXPT_GUARD_END; */
/* } */


/* LUA interface */

#include "luashell.h"

static void print_error(const char * msg, int err)
{
  printf("ERROR(%d) %s \n", err, msg);
}

//long long ll = 32;

static int lua_formats(lua_State * L)
{
  {
    AVInputFormat *ifmt;
    AVOutputFormat *ofmt;
    AVImageFormat *image_fmt;
    URLProtocol *up;
    AVCodec *p, *p2;
    const char **pp, *last_name;

    printf("File formats:\n");
    last_name= "000";
    for(;;){
        int decode=0;
        int encode=0;
        const char *name=NULL;

        for(ofmt = first_oformat; ofmt != NULL; ofmt = ofmt->next) {
            if((name == NULL || strcmp(ofmt->name, name)<0) &&
                strcmp(ofmt->name, last_name)>0){
                name= ofmt->name;
                encode=1;
            }
        }
        for(ifmt = first_iformat; ifmt != NULL; ifmt = ifmt->next) {
            if((name == NULL || strcmp(ifmt->name, name)<0) &&
                strcmp(ifmt->name, last_name)>0){
                name= ifmt->name;
                encode=0;
            }
            if(name && strcmp(ifmt->name, name)==0)
                decode=1;
        }
        if(name==NULL)
            break;
        last_name= name;
        
        printf(
            " %s%s %s\n", 
            decode ? "D":" ", 
            encode ? "E":" ", 
            name);
    }
    printf("\n");

    printf("Image formats:\n");
    for(image_fmt = first_image_format; image_fmt != NULL; 
        image_fmt = image_fmt->next) {
        printf(
            " %s%s %s\n",
            image_fmt->img_read  ? "D":" ",
            image_fmt->img_write ? "E":" ",
            image_fmt->name);
    }
    printf("\n");

    printf("Codecs:\n");
    last_name= "000";
    for(;;){
        int decode=0;
        int encode=0;
        int cap=0;

        p2=NULL;
        for(p = first_avcodec; p != NULL; p = p->next) {
            if((p2==NULL || strcmp(p->name, p2->name)<0) &&
                strcmp(p->name, last_name)>0){
                p2= p;
                decode= encode= cap=0;
            }
            if(p2 && strcmp(p->name, p2->name)==0){
                if(p->decode) decode=1;
                if(p->encode) encode=1;
                cap |= p->capabilities;
            }
        }
        if(p2==NULL)
            break;
        last_name= p2->name;
        
        printf(
            " %s%s%s%s%s%s %s", 
            decode ? "D": (/*p2->decoder ? "d":*/" "), 
            encode ? "E":" ", 
            p2->type == CODEC_TYPE_AUDIO ? "A":"V",
            cap & CODEC_CAP_DRAW_HORIZ_BAND ? "S":" ",
            cap & CODEC_CAP_DR1 ? "D":" ",
            cap & CODEC_CAP_TRUNCATED ? "T":" ",
            p2->name);
       /* if(p2->decoder && decode==0)
            printf(" use %s for decoding", p2->decoder->name);*/
        printf("\n");
    }
    printf("\n");

    printf("Supported file protocols:\n");
    for(up = first_protocol; up != NULL; up = up->next)
        printf(" %s:", up->name);
    printf("\n");
    
    printf("Frame size, frame rate abbreviations:\n ntsc pal qntsc qpal sntsc spal film ntsc-film sqcif qcif cif 4cif\n");
    printf("Motion estimation methods:\n");
    pp = motion_str;
    while (*pp) {
        printf(" %s", *pp);
        if ((pp - motion_str + 1) == ME_ZERO) 
            printf("(fastest)");
        else if ((pp - motion_str + 1) == ME_FULL) 
            printf("(slowest)");
        else if ((pp - motion_str + 1) == ME_EPZS) 
            printf("(default)");
        pp++;
    }
    printf("\n\n");
    printf(
"Note, the names of encoders and decoders dont always match, so there are\n"
"several cases where the above table shows encoder only or decoder only entries\n"
"even though both encoding and decoding are supported for example, the h263\n"
"decoder corresponds to the h263 and h263p encoders, for file formats its even\n"
"worse\n");
  }

}

#include "controler.h"
// big hack, defined in dreamcast68.c
extern controler_state_t controler68;

int decode_start(const char * filename)
{
    AVFormatParameters params, *ap = &params;
    int opened = 0;
    int err, i, ret, rfps, rfps_base;
    //const char * filename = "/pc/kidda.mp3";


  {
    //printf("long long = %f\n", (float) ll);
  }

  audio_size = 0;
  audio_ptr = 0;

    /* get default parameters from command line */
    memset(ap, 0, sizeof(*ap));
    ap->sample_rate = audio_sample_rate;
    ap->channels = audio_channels;
    ap->frame_rate = frame_rate;
    ap->frame_rate_base = frame_rate_base;
    ap->width = frame_width + frame_padleft + frame_padright;
    ap->height = frame_height + frame_padtop + frame_padbottom;
    ap->image_format = image_format;
    ap->pix_fmt = frame_pix_fmt;



    //file_iformat = av_find_input_format("avi");


    /* open the input file with generic libav function */
    err = av_open_input_file(&ic, filename, file_iformat, 0, ap);
    if (err < 0) {
        print_error(filename, err);
        return -1;
    }
    
    /* If not enough info to get the stream parameters, we decode the
       first frames to get it. (used in mpeg case for example) */
    ret = av_find_stream_info(ic);
    if (ret < 0 && verbose >= 0) {
      printf("%s: could not find codec parameters\n", filename);
      goto error;
    }


    //av_seek_frame(ic, -1, 40 * AV_TIME_BASE);

    /* update the current parameters so that they match the one of the input stream */
    video_stream = audio_stream = NULL;
    for(i=0;i<ic->nb_streams;i++) {
      //printf("Stream #%d\n", i);
        AVCodecContext *enc = &ic->streams[i]->codec;
#if defined(HAVE_PTHREADS) || defined(HAVE_W32THREADS)
        if(thread_count>1)
            avcodec_thread_init(enc, thread_count);
	enc->thread_count= thread_count;
#endif
        switch(enc->codec_type) {
        case CODEC_TYPE_AUDIO:
	  //printf("\nInput Audio channels: %d\n", enc->channels);
            audio_channels = enc->channels;
            audio_sample_rate = enc->sample_rate;

	    if (audio_stream == NULL) {
	      printf("Got an audio stream !\n");
	      audio_stream = ic->streams[i];

/* 	      printf("%d, %d, %d, %d\n", 4*sizeof(audio_buf)/2/enc->channels, enc->sample_rate , 255, enc->channels > 1); */
/* 	      stream_start(4*sizeof(audio_buf)/2/enc->channels, enc->sample_rate , 255, enc->channels > 1); */

	    }
            break;
        case CODEC_TYPE_VIDEO:
            frame_height = enc->height;
            frame_width = enc->width;
	    //frame_aspect_ratio = av_q2d(enc->sample_aspect_ratio) * enc->width / enc->height;
	    frame_pix_fmt = enc->pix_fmt;
            rfps      = ic->streams[i]->r_frame_rate;
            rfps_base = ic->streams[i]->r_frame_rate_base;
            enc->workaround_bugs = workaround_bugs;
            enc->error_resilience = error_resilience; 
            enc->error_concealment = error_concealment; 
            enc->idct_algo = idct_algo;
            enc->debug = debug;
            //enc->debug_mv = debug_mv;            
            if(bitexact)
                enc->flags|= CODEC_FLAG_BITEXACT;
            if(me_threshold)
                enc->debug |= FF_DEBUG_MV;

            assert(enc->frame_rate_base == rfps_base); // should be true for now
            if (enc->frame_rate != rfps) { 

                if (verbose >= 0)
                    fprintf(stderr,"\nSeems that stream %d comes from film source: %2.2f->%2.2f\n",
                            i, (float)enc->frame_rate / enc->frame_rate_base,

                    (float)rfps / rfps_base);
            }
            /* update the current frame rate to match the stream frame rate */
            frame_rate      = rfps;
            frame_rate_base = rfps_base;

            enc->rate_emu = rate_emu;

	    if (video_stream == NULL) {
	      printf("Got a video stream ! Format %dx%d (%d)\n",
		     enc->width, enc->height, enc->pix_fmt);
	      video_stream = ic->streams[i];
	    }

            break;
	default:
	  break;
/*         case CODEC_TYPE_DATA: */
/*             break; */
/*         default: */
/* 	  goto error; */
        }
    }
    

    dump_format(ic, 0, filename, 0);

    nb_istreams = ic->nb_streams;
    ist_table = calloc(nb_istreams, sizeof(AVInputStream *));
    if (!ist_table)
        goto error;

    for(i=0;i<nb_istreams;i++) {
        AVInputStream *ist = av_mallocz(sizeof(AVInputStream));
        if (!ist)
            goto error;
        ist_table[i] = ist;

	ist->st = ic->streams[i];
	ist->discard = 1;
	ist->decoding_needed = 
	  (ist->st == audio_stream || ist->st == video_stream);

	if (ist->st->codec.rate_emu) {
	  ist->start = av_gettime();
	  ist->frame = 0;
	}
    }

    /* open each decoder */
    for(i=0;i<nb_istreams;i++) {
        AVInputStream *ist = ist_table[i];
        if (ist->decoding_needed) {
            AVCodec *codec;
	    int res;
            codec = avcodec_find_decoder(ist->st->codec.codec_id);
            if (!codec) {
                fprintf(stderr, "Unsupported codec (id=%d) for input stream #%d\n", 
                        ist->st->codec.codec_id, i);
                continue;
            }
	    
	    EXPT_GUARD_BEGIN;
	    
	    res = avcodec_open(&ist->st->codec, codec);
	    
	    EXPT_GUARD_CATCH;

	    res = -1;
	    
	    EXPT_GUARD_END;

            if (res < 0) {
                fprintf(stderr, "Error while opening codec for input stream #%d\n", 
                        i);
                continue;
            }

	    ist->discard = 0;
	    opened ++;

            //if (ist->st->codec.codec_type == CODEC_TYPE_VIDEO)
            //    ist->st->codec.flags |= CODEC_FLAG_REPEAT_FIELD;
        }
    }

    if (!opened) {
      printf("No audio nor video to play :(\n");
      goto error;
    }

    frame= avcodec_alloc_frame();

    if (video_stream) {
      int texid;

      printf("Got a video stream ! Format %dx%d (%s)\n",
	     video_stream->codec.width, video_stream->codec.height, avcodec_get_pix_fmt_name(video_stream->codec.pix_fmt));

      texid = texture_get("__background__");
      if (texid < 0) {
	texid = texture_create_flat("__background__",1024,512,0xFFFFFFFF);
      }
      if (texid < 0)
	goto error;

      btexture = texture_fastlock(texid, 1);
      if (btexture == NULL)
	goto error;
      btexture->twiddled = 0;  /* Force non twiddle */
      btexture->twiddlable = 0;  /* Force non twiddle */
#define TA_YUV422		((3<<1) | 1)
#define TA_YUV422_TWID		(3<<1)
      //      btexture->format = TA_YUV422;
      texture_release(btexture);

      {
	char buf[128];
	int w, h;
	w = video_stream->codec.width;
	h = video_stream->codec.height;
	memset(btexture->addr, 
	       video_stream->codec.pix_fmt == PIX_FMT_YUV420P? 0x007f007f : 0, 
	       btexture->width*btexture->height*2);
/* 	sprintf(buf, "dl_set_trans(background.dl, mat_scale(1024*640/%d,512*640/%d,1) * mat_trans(0, (480-512*%d/640)/4))", */
/* 		w, w, w); */
	sprintf(buf, "dl_set_trans(background.dl, mat_scale(1024*640/%d,512*640/%d,1))",
		w, w);
	shell_command(buf);
      }
    }


#if 0
  for(i=0;i<ic->nb_streams;i++) {
    int flags;
    char buf[256];
    AVStream *st = ic->streams[i];
    avcodec_string(buf, sizeof(buf), &st->codec, 0);
    av_log(NULL, AV_LOG_DEBUG, "  Stream #%d", i);
    /* the pid is an important information, so we display it */
    /* XXX: add a generic system */
    flags = ic->iformat->flags;
    if (flags & AVFMT_SHOW_IDS) {
      av_log(NULL, AV_LOG_DEBUG, "[0x%x]", st->id);
    }
    //av_log(NULL, AV_LOG_DEBUG, ": %s\n", buf);
    printf(": %s\n", buf);
  }
#endif


    return 0;

 error:
    decode_stop();

    return -1;
}

void play_audio()
{
  if (audio_size > 0) {
    int n = -1;
    if (audio_stream->codec.channels == 1)
      //n = fifo_write_mono(audio_buf + audio_ptr, audio_size);
      n = fifo_write_mono(audio_buf + audio_ptr, audio_size);
    else if (audio_stream->codec.channels == 2)
      n = fifo_write(audio_buf + audio_ptr, audio_size);

/*     if (n < audio_size) { */
/*       printf("as = %d, n=%d\n", audio_size, n); */
/*     } */

    if (n < 0) {
      audio_size = 0;
      audio_ptr = 0;
    }
    if (n > 0) {
      audio_size -= n;
      audio_ptr += n;
    }
  }

}


#define VID_FIFO_SZ 8

static int vid_fifo_in;
static int vid_fifo_out;
static AVPacket vid_fifo[VID_FIFO_SZ];

static AVPacket * vid_fifo_get()
{
  if (vid_fifo_in != vid_fifo_out) {
    AVPacket * res = vid_fifo + vid_fifo_out;
    //printf("get packet #%d\n", vid_fifo_out);
    return res;
  } else
    return 0;
}

static AVPacket * vid_fifo_getstep()
{
  vid_fifo_out = (vid_fifo_out+1)&(VID_FIFO_SZ-1);
}

static int vid_fifo_put(AVPacket * pkt)
{
  if (((vid_fifo_in+1)&(VID_FIFO_SZ-1)) != vid_fifo_out) {
    vid_fifo[vid_fifo_in] = *pkt;
    //printf("put packet #%d\n", vid_fifo_in);
    vid_fifo_in = (vid_fifo_in+1)&(VID_FIFO_SZ-1);
    return 0;
  } else
    return -1;
}

static int vidstream_skip;

static spinlock_t av_mutex;
static void av_lock()
{
  //  spinlock_lock(&av_mutex);
}
static void av_unlock()
{
  //  spinlock_unlock(&av_mutex);
}

void vidstream_thread(void *userdata)
{
  int i, w, h;
  int len1, got_picture;

  while (!vidstream_exit) {
    AVPacket * pkt;
    for ( ; 
	  pkt = vid_fifo_get(), pkt ;       
	  vid_fifo_getstep()
	  ) {

      if (vidstream_skip) { /* skip mode : quickly free the packet */
	av_free_packet(pkt);
	continue;
      }

      w = video_stream->codec.width;
      h = video_stream->codec.height;

      if (video_stream->codec.codec_id == CODEC_ID_RAWVIDEO) {
	//printf("CODEC_ID_RAWVIDEO\n");
	avpicture_fill((AVPicture *)frame, pkt->data, 
		       video_stream->codec.pix_fmt,
		       video_stream->codec.width,
		       video_stream->codec.height);
	frame->pict_type = FF_I_TYPE;
	got_picture = 1;
	/*             if (output_picture2(is, frame, pts) < 0) */
	/*                 goto the_end; */
      } else {
	av_lock();
	len1 = avcodec_decode_video(&video_stream->codec, 
				    frame, &got_picture, 
				    pkt->data, pkt->size);
	av_unlock();
	//printf("len = %d\n", len1);
      }
      if (got_picture) {
	/* 	    if (output_picture2(is, frame, pts) < 0) */
	/* 	      goto the_end; */

	vid_border_color(0xff,0xff,0);
	if (video_stream->codec.pix_fmt != PIX_FMT_YUV420P) {
	  static AVPicture pic = { 0, 0, 0, 0, 0, 0, 0, 0 };
	  pic.data[0] = btexture->addr;
	  pic.linesize[0] = btexture->width*2;
	  pic.data[1] = NULL;
	  pic.linesize[1] = 0;
	  pic.data[2] = NULL;
	  pic.linesize[2] = 0;
	  img_convert(&pic, PIX_FMT_RGB565,
		      //img_convert(&pic, PIX_FMT_YUV422P,
		      frame, video_stream->codec.pix_fmt, 
		      w, h);
	} else { /* my custom yuv420p to yuv422 converter */
#if 1

#if 1
	  btexture->format = TA_PAL8BPP;
	  btexture->twiddled = 1;  /* Force twiddle */
	  btexture->twiddlable = 1;  /* Force twiddle */

	  char * src = (char *) frame->data[0];
	  char * dst = (char *) btexture->addr;
	  int stride = btexture->width;
	  int ls = w;
	  int srcs = frame->linesize[0];

	  for (i=0; i<h; i++) {
	    memcpy(dst, src, ls);
	    src += srcs;
	    dst += stride;
	  }
#else
	  u8_t * srcY1 = (u8_t *) frame->data[0];
	  u8_t * srcY2 = (u8_t *) frame->data[0];
	  u8_t * srcU = (s8_t *) frame->data[2]; 
	  u8_t * srcV = (s8_t *) frame->data[1];
	  u32_t * dst1 = (u32_t *) btexture->addr;
	  u32_t * dst2 = (u32_t *) btexture->addr;
	  int stride = btexture->width;
	  int srcs = frame->linesize[0];
	  int srcsU = frame->linesize[1];
	  int srcsV = frame->linesize[2];
	  int ww = w/2;

	  int  C1 = 1.403f * (1<<16);
	  int  C2 = 0.344f * (1<<16);
	  int  C3 = 0.714f * (1<<16);
	  int  C4 = 1.770f * (1<<16);

	  btexture->format = TA_YUV422;

	  dst2 += stride / 2;
	  srcY2 += srcs;
	  srcs *= 2;

	  for (i=0; i<h; i += 2) {
	    int j;
	    for (j=0; j<ww; j++) {
	      uint Y1 = *srcY1++;
	      uint Y0 = *srcY1++;
	      uint Y3 = *srcY2++;
	      uint Y2 = *srcY2++;
/* 	  uint U = *srcU++ ^0x80; */
/* 	  uint V = *srcV++ ^0x80; */
	      /*   R = Y + C1 * (V - 128) */
	      /*   G = Y - C2 * (V - 128) - C3 * (U - 128) */
	      /*   B = Y + C4 * (U - 128) */

#if 0
	      uint U = *srcU++ - 128;
	      uint V = *srcV++ - 128;
	      int B = C1 * (V) >> 17;
	      int G = - (C2 * (V) + C3 * (U) >> 17);
	      int R = C4 * (U) >> 17;
	      //R=G=B=0;

	      *dst1++ = 
		(((Y0+R)&0xf8) << 8) | (((Y0+G)&0xfc) << 3) | (((Y0+B) >> 3)) |
		(((Y1+R)&0xf8) << 24) | (((Y1+G)&0xfc) << 19) | (((Y1+B)&0xf8) << 13);
	      *dst2++ = 
		(((Y2+R)&0xf8) << 8) | (((Y2+G)&0xfc) << 3) | (((Y2+B) >> 3)) |
		(((Y3+R)&0xf8) << 24) | (((Y3+G)&0xfc) << 19) | (((Y3+B)&0xf8) << 13);
#else
/* 	    uint U = 0x80 */
/* 	    uint V = 0x80 */
	      uint U = *srcU++;
	      uint V = *srcV++;
	      /* 		*dst1++ = (Y0) | (U<<8) | (Y1<<16) | (V<<24); */
	      /* 		*dst2++ = (Y2) | (U<<8) | (Y3<<16) | (V<<24); */
	      *dst1++ = (Y0<<24) | (U<<16) | (Y1<<8) | (V);
	      *dst2++ = (Y2<<24) | (U<<16) | (Y3<<8) | (V);
#endif
	    }

	    srcY1 += srcs - w;
	    srcY2 += srcs - w;
	    /*  	      srcY += srcs - w; */
	    srcU += srcsU - ww;
	    srcV += srcsV - ww;
	    dst1 += stride - ww;
	    dst2 += stride - ww;
	  }
#endif

#else
	  char * src = (char *) frame->data[0];
	  char * dst = (char *) btexture->addr;
	  int stride = btexture->width*2;
	  int ls = w*3;
	  int srcs = frame->linesize[0];

	  for (i=0; i<h; i++) {
	    memcpy(dst, src, ls);
	    src += srcs;
	    dst += stride;
	  }
#endif
	}

	vid_border_color(0,0,0);

	/*       if (n < 3) */
	/* 	printf("TEXTURE %dx%d", btexture->width, btexture->height); */
  
      }

      av_free_packet(pkt);
      
    }

    thd_pass();
  }

  printf("ffmpeg: vidstream_thread exit\n");
  vidstream_exit = 0;
}


int decode_frame()
{
  static int n;
  int i;
  int len1;

  AVInputStream *ist;

  AVPacket pkt;

  play_audio();

  if (audio_size > 0)
    return 0;
	
  if (av_read_frame(ic, &pkt) < 0) {
    return -1;
  }

/*   if ((n&15) == 0) */
/*     av_pkt_dump(stdout, &pkt, 0); */
/*   n++; */

  ist = ist_table[pkt.stream_index];
  if (ist->discard)
    goto discard_packet;

  //printf("packet %x %d\n", pkt.data, pkt.size);

  if (ist->st == audio_stream) {
    int data_size;
    av_lock();
    len1 = avcodec_decode_audio(&ist->st->codec, 
				(int16_t *)audio_buf, &data_size, 
				pkt.data, pkt.size);
    av_unlock();

    //printf("len = %d, data_size = %d\n", len1, data_size);
    if (len1 > 0 && data_size > 0) {
      if (audio_stream->codec.channels == 1)
	audio_size = data_size / 2;
      else if (audio_stream->codec.channels == 2)
	audio_size = data_size / 4;
      else
	audio_size = 0;
      audio_ptr = 0;

      play_audio();
    }
  }

  if (ist->st == video_stream) {
    if (!vid_fifo_put(&pkt)) {
      vidstream_thd->prio2 = 4;
      return 0;
    } else {
      vidstream_thd->prio2 = 9;
    }
  }

 discard_packet:
  av_free_packet(&pkt);

  return 0;
}

int decode_stop()
{
  int i;

  vidstream_skip = 1;
  while (vid_fifo_in != vid_fifo_out) {
    thd_pass();
  }
  vidstream_skip = 0;

  if (ic) {
    av_close_input_file(ic);
    ic = NULL;
  }

  if (ist_table) {
      
    for(i=0;i<nb_istreams;i++) {
      AVInputStream *ist = ist_table[i];

      avcodec_close(&ist->st->codec);

      av_free(ist);
    }

    free(ist_table);

    ist_table = NULL;
  }

#define TA_RGB565		((1<<1) | 1)
  if (btexture) {
    memset(btexture->addr, 0, btexture->width*btexture->height*2);
    btexture->format = TA_RGB565;
    btexture = NULL;
  }

  if (frame) {
    av_free(frame);
    frame = NULL;
  }

    
  return 0;
}

static int lua_toto(lua_State * L)
{
  int n;

  if (decode_start(lua_tostring(L, 1)))
    return 0;

  for (n=0; /*n<5000 &&*/ !(controler68.buttons & CONT_START); n++)
    if (decode_frame())
      break;

  decode_stop();

  return 0;
}

extern int kos_lazy_cd;

static int lua_lazycd(lua_State * L)
{
  int old = kos_lazy_cd;
  kos_lazy_cd = lua_tonumber(L, 1);
  lua_settop(L, 0);
  lua_pushnumber(L, old);
  return 1;
}

static luashell_command_description_t commands[] = {
  {
    "ff_toto", 0, "ffmpeg",     /* long name, short name, topic */
    "ff_toto() : toto",  /* usage */
    SHELL_COMMAND_C, lua_toto      /* function */
  },

  {
    "ff_formats", 0, "ffmpeg",                  /* long name, short name, topic */
    "ff_formats() : display supported formats", /* usage */
    SHELL_COMMAND_C, lua_formats                /* function */
  },

  {
    "ff_lazycd", 0, "ffmpeg",                  /* long name, short name, topic */
    "ff_lazycd(mode) : if mode is not nil or 0, then \n"
    "set lazy CD on, return old status",        /* usage */
    SHELL_COMMAND_C, lua_lazycd                /* function */
  },

  /* end of the command list */
  {0},
};


static inp_driver_t ffmpeg_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /**< Next driver (see any_driver.h)  */
    INP_DRIVER,           /**< Driver type */      
    0x0100,               /**< Driver version */
    "ffmpeg",             /**< Driver name */
    "Vincent Penne, "     /**< Driver authors */
    "Fabrice Bellard",
    "FFMpeg video player",/**< Description */
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
    commands,             /**< Lua shell commands */
  },
  
  /* Input driver specific */
  
  0,                      /**< User Id */
  ".wmv\0.rm\0.mpg\0.mpeg\0.mpc\0.mp3\0.avi\0",   /**< EXtension list */

  start,
  stop,
  decoder,
/*   info, */
};

EXPORT_DRIVER(ffmpeg_driver)
