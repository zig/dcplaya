/*
 * $Id: ffmpeg_driver.c,v 1.5 2004-07-13 09:24:19 vincentp Exp $
 */

/* Define this for benchmark mode (not to a watch video !) */
//#define BENCH

/* Define this to use dma texture transfert, for faster yuv2rgb transformation */
#define USE_DMA


#include "dcplaya/config.h"
#include "extern_def.h"

#include <kos.h>

DCPLAYA_EXTERN_C_START
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
DCPLAYA_EXTERN_C_END

#include "ffmpeg.h"

#include "sha123_ffdriver.h"

#include "playa.h"
#include "fifo.h"
#include "inp_driver.h"
#include "draw/texture.h"
#include "draw/ta.h"
#include <arch/timer.h>

#include "exceptions.h"
#include "lef.h"

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
static int btexture_id;
static texture_t * btexture2;
static int btexture2_id;
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
static uint64_t audio_pts;
static uint64_t audio_next_pts;
static int audio_fifo_used;
static int audio_fifo_rate;
static int audio_fc;
static uint64_t audio_time;
static int audio_delay;
static uint64_t oldpts;
static uint64_t realpts;

static char * force_format;

static semaphore_t * dma_done;
#ifdef USE_DMA

/* VP : For some reason, it is not stable when launching the dma for more than
   DMA_BSZ bytes */
#define DMA_BSZ (64*64*2) /* size of a dma block */

typedef struct dma_pict {
  uint8_t * buf;
  uint64_t pts;
} dma_pict_t;

static uint32_t dma_tex;
static int dma_sendn;
static int dma_sendoff;
static int dma_sendinc;
static uint8_t * dma_texbuf;
static int dma_texbufsz = 1024*1024;
static dma_pict_t dma_pics[32];
static int dma_picin;
static int dma_picout;
#endif

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

static int http;

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

static lef_prog_t * codecs[32];
static int nbcodecs;


uint64_t toto = 1800000000000UL;
uint64_t toto2 = 1600000000010UL;

static int init(any_driver_t *d)
{
  int n = 0;
  EXPT_GUARD_BEGIN;

  dbglog(DBG_DEBUG, "ffmpeg : Init [%s]\n", d->name);
  ready = 0;

  av_register_all();

#ifndef OLDFF
  sha123_ffdriver_init();
#endif


  dma_done = sem_create(1);
  vidstream_thd = thd_create(vidstream_thread, 0);
  if (vidstream_thd)
    thd_set_label(vidstream_thd, "Video-decode-thd");
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
    av_free(audio_buf);
    audio_buf = NULL;
  }

  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  int i;

  EXPT_GUARD_BEGIN;

  stop();

  vidstream_exit = 1;
  while (vidstream_exit)
    thd_pass();
  sem_destroy(dma_done);

#ifndef OLDFF
  sha123_ffdriver_shutdown();
#endif

  for (i=0; i<nbcodecs; i++)
    lef_free(codecs[i]);

  if (dma_texbuf)
    free(dma_texbuf);
  
  FreeAll();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_END;
  return 0;
}

static int start(const char *fn, int track, playa_info_t *info)
{
  EXPT_GUARD_BEGIN;

  stop();

  audio_buf = (u32_t *) av_malloc(AUDIO_SZ);

  //goto error;
  if (decode_start(fn))
    goto error;

#ifndef OLDFF
  playa_info_bps    (info, ic->bit_rate); 
#endif
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
#ifndef OLDFF
  playa_info_time   (info, ic->duration * 1000 / AV_TIME_BASE);
#endif

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
        
  n = 
    decode_frame() ||
    decode_frame();

  EXPT_GUARD_CATCH;

  EXPT_GUARD_RETURN INP_DECODE_ERROR;

  EXPT_GUARD_END;
  return n? INP_DECODE_ERROR : INP_DECODE_CONT;
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

/* Get power of 2 greater or equal to a value or -1 */
static int greaterlog2(int v)
{
  int i;
  for (i=0; i<(sizeof(int)<<3); ++i) {
    if ((1<<i) >= v) {
      return i;
    }
  }
  return -1;
}

#include "controler.h"
// hack, defined in dreamcast68.c
extern controler_state_t controler68;

/* time statistics */
static int adecode_j, aplay_j, readstream_j, vdecode_j, vconv_j;


int decode_start(const char * filename)
{
    AVFormatParameters params, *ap = &params;
    int opened = 0;
    int err, i, ret, rfps, rfps_base;
    //const char * filename = "/pc/kidda.mp3";


    http = strstr(filename, "/http/") == filename;
#ifdef BENCH
    http = 1;
#endif

    audio_size = 0;
    audio_ptr = 0;

    /* get default parameters from command line */
    memset(ap, 0, sizeof(*ap));
/*     ap->sample_rate = audio_sample_rate; */
/*     ap->channels = audio_channels; */
/*     ap->frame_rate = frame_rate; */
/*     ap->frame_rate_base = frame_rate_base; */
/*     ap->width = frame_width + frame_padleft + frame_padright; */
/*     ap->height = frame_height + frame_padtop + frame_padbottom; */
/*     ap->image_format = image_format; */
/*     ap->pix_fmt = frame_pix_fmt; */



    if (force_format)
      file_iformat = av_find_input_format(force_format);
    else
      file_iformat = NULL;


    /* open the input file with generic libav function */
    err = av_open_input_file(&ic, filename, file_iformat, 0, ap);
    if (err < 0) {
        print_error(filename, err);
        return -1;
    }
    
    /* If not enough info to get the stream parameters, we decode the
       first frames to get it. (used in mpeg case for example) */
    ret = av_find_stream_info(ic);
    if (ret < 0) {
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

	//enc->idct_algo = FF_IDCT_SIMPLE;
    

        switch(enc->codec_type) {
        case CODEC_TYPE_AUDIO:
	  //printf("\nInput Audio channels: %d\n", enc->channels);
#ifdef BENCH
	    break;
#endif
	    if (audio_stream == NULL) {
	      audio_channels = enc->channels;
	      audio_sample_rate = enc->sample_rate;
	      audio_fifo_rate = enc->sample_rate; // * enc->channels / 2;

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
    ist_table = av_mallocz(nb_istreams * sizeof(AVInputStream *));
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
                goto openerr;
            }
	    
	    EXPT_GUARD_BEGIN;
	    
	    res = avcodec_open(&ist->st->codec, codec);
	    
	    EXPT_GUARD_CATCH;

	    res = -1;
	    
	    EXPT_GUARD_END;

            if (res < 0) {
                fprintf(stderr, "Error while opening codec for input stream #%d\n", 
                        i);
                goto openerr;
            }

	    ist->discard = 0;
	    opened ++;

            //if (ist->st->codec.codec_type == CODEC_TYPE_VIDEO)
            //    ist->st->codec.flags |= CODEC_FLAG_REPEAT_FIELD;

	openerr:
	    if (ist->discard) {
	      if (ist->st == video_stream)
		video_stream = NULL;
	      if (ist->st == audio_stream)
		audio_stream = NULL;
	    }
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
	     video_stream->codec.width, video_stream->codec.height, 
	     avcodec_get_pix_fmt_name(video_stream->codec.pix_fmt));


      texid = texture_get("__background2__");
      if (texid < 0) {
	texid = texture_create_flat("__background2__",1024,512,0xFFFFFFFF);
      }
      if (texid < 0)
	goto error;

      btexture2_id = texid;
      btexture2 = texture_fastlock(texid, 1);
      if (btexture2 == NULL)
	goto error;
      btexture2->twiddled = 0;  /* Force non twiddle */
      btexture2->twiddlable = 0;  /* Force non twiddle */
      //      btexture->format = TA_YUV422;
      texture_release(btexture2);


      texid = texture_get("__background__");
      if (texid < 0) {
	texid = texture_create_flat("__background__",1024,512,0xFFFFFFFF);
      }
      if (texid < 0)
	goto error;

      btexture_id = texid;
      btexture = texture_fastlock(texid, 1);
      if (btexture == NULL)
	goto error;
      btexture->twiddled = 0;  /* Force non twiddle */
      btexture->twiddlable = 0;  /* Force non twiddle */
      //      btexture->format = TA_YUV422;
      texture_release(btexture);

      {
	char buf[128];
	int i, n;
	int w, h;
	int nw, nh, lw, lh;
	w = video_stream->codec.width;
	h = video_stream->codec.height;

	lw = greaterlog2(w);
	nw = 1<<lw;
	lh = greaterlog2(h);
	nh = 1<<lh;

	printf("Texture size %dx%d\n", nw, nh);
	btexture->width = nw;
	btexture->height = nh;
	btexture2->width = nw;
	btexture2->height = nh;
	btexture->wlog2 = lw;
	btexture->hlog2 = lh;
	btexture2->wlog2 = lw;
	btexture2->hlog2 = lh;

#ifdef USE_DMA
	dma_texbuf = memalign(64, dma_texbufsz);
#endif

	{
	  uint32_t pix = video_stream->codec.pix_fmt == PIX_FMT_YUV420P? 
	    0x00800080 : 0;
	  uint32_t * p1 = btexture->addr;
	  uint32_t * p2 = btexture2->addr;
#ifdef USE_DMA
	  uint32_t * p  = (uint32_t *) dma_texbuf;
#endif
	  n = btexture->width*btexture->height/2;
	  for (i=0; i<n; i++) {
	    *p1++ = pix;
	    *p2++ = pix;
#ifdef USE_DMA
	    *p++ = pix;
#endif
	  }
	}
	sprintf(buf, 
		"dl_set_trans(background.dl, "
		"mat_scale(%d*640/%d,%d*640/%d,1)"
		"*mat_trans(0, (480-%d*640/%d)/2, 0))",
		nw, w, nh, w, h, w);
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

  
    adecode_j = aplay_j = readstream_j = vdecode_j = vconv_j = 0;
  

    return 0;

 error:
    decode_stop();

    return -1;
}


static spinlock_t av_mutex;
static void av_lock()
{
  spinlock_lock(&av_mutex);
}
static void av_unlock()
{
  spinlock_unlock(&av_mutex);
}



void play_audio()
{
  if (audio_size > 0) {
    int n = -1;
    av_lock();
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
      //printf("audio pts = %d\n", (int) (audio_pts*100/AV_TIME_BASE));

      audio_fifo_used = fifo_used();
      audio_fc = ta_state.frame_counter;
      audio_time = timer_ms_gettime64();
      audio_pts = audio_next_pts 
	- ((int64_t)audio_fifo_used) * AV_TIME_BASE / audio_fifo_rate
	+ ((int64_t)audio_size) * AV_TIME_BASE / audio_fifo_rate
	;

      audio_size -= n;
      audio_ptr += n;
    }
    av_unlock();
  }

}


#define VID_FIFO_SZ 32

static int vid_fifo_in;
static int vid_fifo_out;
static AVPacket vid_fifo[VID_FIFO_SZ];

static int vid_fifo_used()
{
  return (vid_fifo_in - vid_fifo_out) & (VID_FIFO_SZ-1);
}

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


static uint lumi=256, chroma1=256, chroma2=256;
static uint lt[256], ct1[256], ct2[256];


/* from dcdivx, not faster than my code */
void yuv2rgb_565(uint8_t *y, int stride_y, 
		 uint8_t *cb, uint8_t *cr, int stride_uv, 
		 uint8_t *puc_out, int w, int h,int stride_dest) 
     //, int Dither) //,int Brightness,uint8_t *puc_yp,uint8_t *puc_up,uint8_t *puc_vp, int slowopt, int invert) 
{
  int i,j;
  unsigned int *texp;
  unsigned int p1,p2;
  texp = (unsigned int*) puc_out; 

  for(j =  h>>1; j ; j--) 
    {
      for(i = 0; i < w>>1; i++) 
	{
	  p1=(*(cb) | (((unsigned short)*(y++))<<8));
	  p2=(*(cr) | (((unsigned short)*(y++))<<8));

	  texp[i] = p1|(p2<<16); 
	  p1=(*(cb++) | (((unsigned short)*(y+(stride_y)-2))<<8));
	  p2=(*(cr++) | (((unsigned short)*(y+(stride_y)-1))<<8));
	  texp[i+(stride_dest>>1)]=p1|(p2<<16);

	}
      texp += stride_dest;
      y+=(stride_y<<1)-w;
      cb+=stride_uv-(w>>1);
      cr+=stride_uv-(w>>1);
    }
}


static yuvinit(float lo, float lm, float c1o, float c1m, float c2o, float c2m)
{
  int i;
  
  for (i=0; i<256; i++) {
    int v;

    v = (i-lo)*lm;
    if (v<0) v = 0;
    if (v>255) v=255;
    lt[i] = v;

    v = i-128;
    if (v>0) {
      v -= c1o;
      if (v < 0)
	v = 0;
    } else {
      v += c1o;
      if (v > 0)
	v = 0;
    }
    v = v*c1m+128;
    if (v<0) v = 0;
    if (v>255) v=255;
    ct1[i] = v;

    v = i-128;
    if (v>0) {
      v -= c2o;
      if (v < 0)
	v = 0;
    } else {
      v += c2o;
      if (v > 0)
	v = 0;
    }
    v = v*c2m+128;
    if (v<0) v = 0;
    if (v>255) v=255;
    ct2[i] = v;
  }
}



extern uint32 render_counter;
extern uint32 render_counter2;
extern void (* ta_render_done_cb) ();

void totocb()
{
  if (btexture && btexture2) {
  {
    uint32 tmp;
    tmp = btexture->ta_tex; 
    btexture->ta_tex = btexture2->ta_tex;
    btexture2->ta_tex = tmp;
  } {
    void * tmp;
    tmp = btexture->addr; 
    btexture->addr = btexture2->addr;
    btexture2->addr = tmp;
  }
  }
}

void render_done_cb()
{
  static int code = 0;

  memset(btexture2->addr, code, btexture->width*btexture->height);
  code = 0x80-code;

/*   { */
/*     uint32 tmp; */
/*     tmp = btexture->ta_tex;  */
/*     btexture->ta_tex = btexture2->ta_tex; */
/*     btexture2->ta_tex = tmp; */
/*   } { */
/*     void * tmp; */
/*     tmp = btexture->addr;  */
/*     btexture->addr = btexture2->addr; */
/*     btexture2->addr = tmp; */
/*   } */

  ta_render_done_cb = NULL;
  //ta_render_done_cb = totocb;
}


#include <dc/pvr.h>

#ifdef USE_DMA
static void dma_cb(void * p)
{
  if (dma_sendn > 0) {
    pvr_txr_load_dma(dma_texbuf+dma_sendoff, dma_tex+dma_sendoff, 
		     dma_sendinc, 0, dma_cb, 0);
    dma_sendn -= dma_sendinc;
    dma_sendoff += dma_sendinc;
    vid_border_color(dma_sendn/64, dma_sendoff/128, 128);
  } else {
    dma_sendn = 0;
    sem_signal(dma_done);
    thd_schedule(1, 0);
    vid_border_color(0, 0, 0);
  }
}
#endif

static void yuv420pto422(int w, int h)
{
#ifdef USE_DMA
  /* wait for previous transfert completion */
  sem_wait(dma_done);
#endif


  int i;
  u32_t * srcY1 = (u8_t *) frame->data[0];
  u32_t * srcY2 = (u8_t *) frame->data[0];
  u32_t * srcU = (s8_t *) frame->data[2]; 
  u32_t * srcV = (s8_t *) frame->data[1];
#ifdef USE_DMA
  u32_t * dst1 = (u32_t *) dma_texbuf;
  u32_t * dst2 = (u32_t *) dma_texbuf;
#else
  u32_t * dst1 = (u32_t *) btexture2->addr;
  u32_t * dst2 = (u32_t *) btexture2->addr;
#endif
  int stride = btexture->width;
  int srcs = frame->linesize[0];
  int srcsU = frame->linesize[1]>>2;
  //int srcsV = frame->linesize[2]>>2;
  int ww = w/8;

  uint l = lumi, c1 = chroma1, c2 = chroma2;

#if 0
  register uint32 m24=0xff000000;
  register uint32 m16=0xff0000;
  register uint32 m8=0xff00;
#else
#define m24 0xff000000
#define m16 0xff0000
#define m8 0xff00
#endif

  dst2 += stride / 2;
  srcY2 += srcs/4;
  srcs *= 2;

  i = h/2;
  while (i--) {
    int j;
    j = ww;
    while(j--) {

#if 1

#if 1

#if 1

      /* with luminosity and chroma tables, and, surprise, it's just as fast
	 as the non-tunnable version ! */
      uint Y0, Y1, U, V;
      uint u, v;

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      U = *srcU++;
      V = *srcV++;

      u = (ct1[(U&0xff)])<<16;
      v = (ct2[(V&0xff)]);
      *dst1++ = ((lt[(Y0>>8)&0xff]<<24)) | (u) | ((lt[(Y0&0xff)]<<8)) | (v);
      *dst2++ = ((lt[(Y1>>8)&0xff]<<24)) | (u) | ((lt[(Y1&0xff)]<<8)) | (v);

      u = (ct1[(U>>8)&0xff])<<16;
      v = (ct2[(V>>8)&0xff]);
      *dst1++ = (((lt[(Y0)>>24]))<<24) | u | ((lt[(Y0>>16)&0xff]<<8)) | (v);
      *dst2++ = (((lt[(Y1)>>24]))<<24) | u | ((lt[(Y1>>16)&0xff]<<8)) | (v);



      Y0 = *srcY1++;
      Y1 = *srcY2++;
      u = (ct1[(U>>16)&0xff])<<16;
      v = (ct2[(V>>16)&0xff]);
      *dst1++ = ((lt[(Y0>>8)&0xff]<<24)) | (u) | ((lt[(Y0&0xff)]<<8)) | (v);
      *dst2++ = ((lt[(Y1>>8)&0xff]<<24)) | (u) | ((lt[(Y1&0xff)]<<8)) | (v);

      u = ((ct1[(U)>>24]))<<16;
      v = ct2[(V)>>24];
      *dst1++ = (((lt[(Y0)>>24]))<<24) | u | ((lt[(Y0>>16)&0xff]<<8)) | (v);
      *dst2++ = (((lt[(Y1)>>24]))<<24) | u | ((lt[(Y1>>16)&0xff]<<8)) | (v);

#else


      /* with luminosity and chroma (slow version with multiplications) */
      uint Y0, Y1, U, V;
      uint u, v;

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      U = *srcU++;
      V = *srcV++;

      u = ((U&0xff)*c1)&m8;
      v = ((V&0xff)*c2)&m8;
      *dst1++ = (((Y0&m8)*l<<8)&m24) | (u<<8) | (((Y0&0xff)*l)&m8) | (v>>8);
      *dst2++ = (((Y1&m8)*l<<8)&m24) | (u<<8) | (((Y1&0xff)*l)&m8) | (v>>8);

      u = ((U&m8)*c1)&m16;
      v = ((V&m8)*c2)&m16;
      *dst1++ = ((((Y0&m24)>>8)*l)&m24) | u | (((Y0&m16)*l>>16)&m8) | (v>>16);
      *dst2++ = ((((Y1&m24)>>8)*l)&m24) | u | (((Y1&m16)*l>>16)&m8) | (v>>16);



      Y0 = *srcY1++;
      Y1 = *srcY2++;
      u = ((U&m16)*c1)&m24;
      v = ((V&m16)*c2);//&m24;
      *dst1++ = (((Y0&m8)*l<<8)&m24) | (u>>8) | (((Y0&0xff)*l)&m8) | (v>>24);
      *dst2++ = (((Y1&m8)*l<<8)&m24) | (u>>8) | (((Y1&0xff)*l)&m8) | (v>>24);

      u = (((U&m24)>>16)*c1)&m16;
      v = (((V&m24)>>8)*c2);//&m16;
      *dst1++ = ((((Y0&m24)>>8)*l)&m24) | u | (((Y0&m16)*l>>16)&m8) | (v>>24);
      *dst2++ = ((((Y1&m24)>>8)*l)&m24) | u | (((Y1&m16)*l>>16)&m8) | (v>>24);

#endif

#else
      /* no luminosity/chroma tuning, not faster than with it */
      uint Y0, Y1, U, V;

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      U = *srcU++;
      V = *srcV++;

      *dst1++ = ((Y0&m8)<<16) | ((U&0xff)<<16) | ((Y0&0xff)<<8) | (V&0xff);
      *dst2++ = ((Y1&m8)<<16) | ((U&0xff)<<16) | ((Y1&0xff)<<8) | (V&0xff);

      *dst1++ = ((Y0&m24)) | ((U&m8)<<8) | ((Y0&m16)>>8) | ((V&m8)>>8);
      *dst2++ = ((Y1&m24)) | ((U&m8)<<8) | ((Y1&m16)>>8) | ((V&m8)>>8);

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      *dst1++ = ((Y0&m8)<<16) | ((U&m16)) | ((Y0&0xff)<<8) | ((V&m16)>>16);
      *dst2++ = ((Y1&m8)<<16) | ((U&m16)) | ((Y1&0xff)<<8) | ((V&m16)>>16);

      *dst1++ = ((Y0&m24)) | ((U&m24)>>8) | ((Y0&m16)>>8) | (V>>24);
      *dst2++ = ((Y1&m24)) | ((U&m24)>>8) | ((Y1&m16)>>8) | (V>>24);
#endif

#else

      uint Y0, Y1, Uc, Vc, U, V;

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      Uc = *srcU++;
      Vc = *srcV++;

      U = Uc&0xff;
      V = Vc&0xff;
      *dst1++ = ((Y0&m8)<<16) | (U<<16) | ((Y0&0xff)<<8) | (V);
      *dst2++ = ((Y1&m8)<<16) | (U<<16) | ((Y1&0xff)<<8) | (V);

      Uc >>= 8;
      Vc >>= 8;
      U = Uc&0xff;
      V = Vc&0xff;
      *dst1++ = ((Y0&m24)) | (U<<16) | ((Y0&m16)>>8) | (V);
      *dst2++ = ((Y1&m24)) | (U<<16) | ((Y1&m16)>>8) | (V);

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      Uc >>= 8;
      Vc >>= 8;
      U = Uc&0xff;
      V = Vc&0xff;
      *dst1++ = ((Y0&m8)<<16) | (U<<16) | ((Y0&0xff)<<8) | (V);
      *dst2++ = ((Y1&m8)<<16) | (U<<16) | ((Y1&0xff)<<8) | (V);

      Uc >>= 8;
      Vc >>= 8;
      U = Uc&0xff;
      V = Vc&0xff;
      *dst1++ = ((Y0&m24)) | (U<<16) | ((Y0&m16)>>8) | (V);
      *dst2++ = ((Y1&m24)) | (U<<16) | ((Y1&m16)>>8) | (V);
#endif
    }

    srcY1 += (srcs - w) >>2;
    srcY2 += (srcs - w) >>2;
    srcU += srcsU - ww;
    srcV += srcsU - ww;
    dst1 += stride - (ww<<2);
    dst2 += stride - (ww<<2);
  }

#ifdef USE_DMA
  dma_sendn = btexture2->width * h * 2;
  dma_sendoff = 0;
  //dma_sendinc = (dma_sendn + 31) & (~31);
  dma_sendinc = DMA_BSZ; 
  dma_tex = btexture2->ta_tex;
  dma_cb(0);

  return;
#endif

}

#include "controler.h"
extern controler_state_t controler68;

void vidstream_thread(void *userdata)
{
  int i, w, h;
  int len1, got_picture = 0;
  static int lj;
  int nj;
  static int fc;

  yuvinit(15, 1.2, 0, 1, 0, 1);

  while (!vidstream_exit) {
    AVPacket * pkt;
    lj = thd_current->jiffies;
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
      } else {

	if (audio_stream == NULL || http)
	  video_stream->codec.hurry_up = 0;
	else {
	  static int add;
	  int used = vid_fifo_used() - add;
	  add = 1-add;
	  if (used < 1)
	    video_stream->codec.hurry_up = 5;
	  else if (used < 3)
	    video_stream->codec.hurry_up = 2;
	  else if (used < 4)
	    video_stream->codec.hurry_up = 1;
	  else
	    video_stream->codec.hurry_up = 0;
	}
	video_stream->codec.hurry_up = 0;

	len1 = avcodec_decode_video(&video_stream->codec, 
				    frame, &got_picture, 
				    pkt->data, pkt->size);
	//printf("len = %d\n", len1);
      }
      nj = thd_current->jiffies;
      vdecode_j += nj-lj;
      lj = nj;

      if (audio_stream && got_picture) {
	
/* 	printf("%4d  %4d\n", (int) (pkt->pts * 60 / AV_TIME_BASE - */
/* 	       audio_pts * 60 / AV_TIME_BASE), */
/* 	       audio_fifo_used * 60 / audio_fifo_rate */
/* 	       //- audio_fc + ta_state.frame_counter */
/* 	       ); */

	/* time synchronisation */
	for ( ; ; ) {
	  int used;
	  av_lock();
	  used = fifo_used();
	  if (used == 0) {
	    //printf("SYNCHRO BUG : audio fifo empty\n");
	    break;
	  }
#if 0
	  if (pkt->pts * audio_fifo_rate / AV_TIME_BASE <= 
	      audio_pts * audio_fifo_rate / AV_TIME_BASE 
	      - /*audio_fifo_used + */used 
	      + 6*audio_fifo_rate/60 /* two screen frames */
	      )
#else

#if 0
	  if (pkt->pts * 60 / AV_TIME_BASE <= 
	      audio_pts * 60 / AV_TIME_BASE 
	      - audio_fc + ta_state.frame_counter 
	      + 3 /* six screen blanking */
	      )
#else
	  static uint64_t oldtime;
	  static float diffavg;
	  static float diffavg2;
	  static int nbdiffs, idiffs;
	  static uint64_t olddiff;
	  uint64_t time;
	  time = timer_ms_gettime64();
	  if (oldpts) {
	    uint64_t diff, dtime;
	    //static int coef = (1.055) * (1<<16);
	    static int coef = (1<<16);
#define NBDIFFS 10
	    static float diffs[NBDIFFS];
	    static float diffs2[NBDIFFS];
	    dtime = (time - oldtime) * coef >>16;
	    if (pkt->pts / (AV_TIME_BASE/1000) <= 
		oldpts / (AV_TIME_BASE/1000)
		- dtime
		) {
	      //oldpts += ( (time - oldtime) * coef >>16) * (AV_TIME_BASE/1000);

	      diff = 
		pkt->pts / (AV_TIME_BASE/1000) -
		(audio_pts / (AV_TIME_BASE/1000)
		 - audio_time + time 
		 + 3*1000/60)
		+ 125 /* empiric value */
		+ audio_delay
		;

/* 	      if (controler68.buttons & CONT_X) */
/* 		diff += 500; */

	      if (nbdiffs == NBDIFFS) {
		diffavg -= diffs[idiffs];
		diffavg2 -= diffs2[idiffs];
	      }

	      int dt = time - oldtime;
	      if (dt <= 0)
		dt = 1;
	      diffs[idiffs] = ((int)diff)/1000.0f;
	      diffs2[idiffs] = ((int)(diff-olddiff))/((float)dt);
	      olddiff = diff;
	      diffavg += diffs[idiffs];
	      diffavg2 += diffs2[idiffs];

	      if (nbdiffs < NBDIFFS)
		nbdiffs++;

	      idiffs++;
	      if (idiffs == NBDIFFS)
		idiffs = 0;

	      float avg = diffavg/nbdiffs;
	      float avg2 = diffavg2/nbdiffs;

/* 	      static toto; toto++; if ((toto&7)==0) */
/* 	      printf("avg %g avg2 %g coeff %g dt %d\n",  */
/* 		     avg, avg2, ((float) coef)/(1<<16), (int)dt); */

/* 	      avg = (1 - 0.1*avg); */
/* 	      coef *= avg*avg; */

/* 	      avg2 = (1 - 0.1*avg2); */
/* 	      coef *= avg2; */

	      coef -= (10*avg*avg*avg + avg2*avg2*avg2) * (1<<16);

	      if (coef < 1<<15)
		coef = 1<<15;
	      if (coef > 2<<16)
		coef = 2<<16;

	      oldtime = time;

	      break;
	    }
	  } else {
	    if (pkt->pts / (AV_TIME_BASE/1000) <= 
		audio_pts / (AV_TIME_BASE/1000)
		- audio_time + time 
		+ 3*1000/60 /* three screen blanking */
		- 125 /* empiric value */
		- audio_delay
		) {
	      diffavg = 0;
	      diffavg2 = 0;
	      nbdiffs = idiffs = 0;
	      olddiff = 0;

	      oldtime = time;
	      oldpts = pkt->pts;
	      break;
	    }
	  }
	  if (0)
#endif

#endif
	    break;

	  av_unlock();

	  thd_pass();
	}
	av_unlock();

      }

      if (got_picture) {

	while (render_counter < fc)
	  thd_pass();

	lj = thd_current->jiffies;

	//vid_border_color(0xff,0xff,0);
	if (video_stream->codec.pix_fmt == PIX_FMT_YUV422) {
	  btexture->format = TA_YUV422;

	  char * src = (char *) frame->data[0];
	  char * dst = (char *) btexture2->addr;
	  int stride = btexture->width;
	  int ls = w;
	  int srcs = frame->linesize[0];

	  for (i=0; i<h; i++) {
	    memcpy(dst, src, ls);
	    src += srcs;
	    dst += stride;
	  }
	} else if (video_stream->codec.pix_fmt != PIX_FMT_YUV420P) {
	  static AVPicture pic = { 0, 0, 0, 0, 0, 0, 0, 0 };
	  pic.data[0] = btexture2->addr;
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
#if 0
	  btexture->format = TA_PAL8BPP;
	  btexture->twiddled = 1;  /* Force twiddle */
	  btexture->twiddlable = 1;  /* Force twiddle */

	  char * src = (char *) frame->data[0];
	  char * dst = (char *) btexture2->addr;
	  int stride = btexture->width;
	  int ls = w;
	  int srcs = frame->linesize[0];

	  for (i=0; i<h; i++) {
	    memcpy(dst, src, ls);
	    src += srcs;
	    dst += stride;
	  }
#else
	  btexture->format = TA_YUV422;

/* 	  yuv2rgb_565(frame->data[0], frame->linesize[0],  */
/* 		      frame->data[1], frame->data[2], frame->linesize[1],  */
/* 		      btexture2->addr, w, h,btexture->width); */

#ifdef BENCH
	  static int toto; toto++; if ((toto&15)) got_picture = 0; else
#endif
	  yuv420pto422(w, h);
	  //render_done_cb();

	  //ta_render_done_cb = render_done_cb;
#endif
	}

	//vid_border_color(0,0,0);

	/*       if (n < 3) */
	/* 	printf("TEXTURE %dx%d", btexture->width, btexture->height); */

	nj = thd_current->jiffies;
	vconv_j += nj-lj;
	lj = nj;
  
      }

      realpts = pkt->pts;
	      
      av_free_packet(pkt);
      
      /* Swap textures */
      if (got_picture) {

# define STOPIRQ \
 	if (!irq_inside_int()) { oldirq = irq_disable(); } else

# define STARTIRQ \
	if (!irq_inside_int()) { irq_restore(oldirq); } else

	int oldirq;

/* 	btexture2 = texture_fastlock(btexture2_id, 1); */
/* 	btexture = texture_fastlock(btexture_id, 1); */

	STOPIRQ;
	{
	  uint32 tmp;
	  tmp = btexture->ta_tex; 
	  btexture->ta_tex = btexture2->ta_tex;
	  btexture2->ta_tex = tmp;
	} {
	  void * tmp;
	  tmp = btexture->addr; 
	  btexture->addr = btexture2->addr;
	  btexture2->addr = tmp;
	}
	STARTIRQ;

/* 	texture_release(btexture); */
/* 	texture_release(btexture2); */

	fc = render_counter;
      }
      
      nj = thd_current->jiffies;
      //vdecode_j += nj-lj;
      lj = nj;

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
  int lj, nj;

  AVInputStream *ist;

  AVPacket pkt;

  lj = thd_current->jiffies;

  play_audio();
  nj = thd_current->jiffies;
  aplay_j += nj-lj;
  lj = nj;

  if (audio_size > 0)
    return 0;

  if (av_read_frame(ic, &pkt) < 0) {
    if (vid_fifo_used() > 0)
      return 0;

    return -1;
  }

  nj = thd_current->jiffies;
  readstream_j += nj-lj;
  lj = nj;

/*   if ((n&15) == 0) */
/*     av_pkt_dump(stdout, &pkt, 0); */
/*   n++; */

  ist = ist_table[pkt.stream_index];
  if (ist->discard)
    goto discard_packet;

  //printf("packet %x %d\n", pkt.data, pkt.size);

  if (ist->st == audio_stream) {
    int data_size;
    uint8_t * data = pkt.data;
    int size = pkt.size;
    len1 = avcodec_decode_audio(&ist->st->codec, 
				(int16_t *)audio_buf, &data_size, 
				pkt.data, pkt.size);

    if (data_size < 0)
      data_size = 0;
#ifdef OLDFF
    while (len1 > 0 && size > 0) {
      int s;
      data += len1;
      size -= len1;
      len1 = avcodec_decode_audio(&ist->st->codec, 
				  ((uint8_t *)audio_buf) + data_size, 
				  &s, 
				  data, size);

      if (s < 0)
	s = 0;

      if (len1 > 0)
	data_size += s;
    }
#endif

    nj = thd_current->jiffies;
    adecode_j += nj-lj;
    lj = nj;

    //printf("len = %d, data_size = %d\n", len1, data_size);
    if (len1 > 0 && data_size > 0) {
      if (audio_stream->codec.channels == 1)
	audio_size = data_size / 2;
      else if (audio_stream->codec.channels == 2)
	audio_size = data_size / 4;
      else
	audio_size = 0;
      audio_ptr = 0;
      audio_next_pts = pkt.pts;

      play_audio();

      nj = thd_current->jiffies;
      aplay_j += nj-lj;
      lj = nj;

    }
  }

  if (ist->st == video_stream) {
    if (1||audio_stream == NULL) { // No sound --> never skip video frame
      vidstream_thd->prio2 = 6;
      while (vid_fifo_put(&pkt)) {
	thd_pass();
      }
      return 0;
    } else {
      if (!vid_fifo_put(&pkt)) {
	vidstream_thd->prio2 = 6;
	return 0;
      } else {
	//printf("droped video frame\n");
	vidstream_thd->prio2 = 9;
      }
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

  if (ist_table) {
    if (video_stream)
      avcodec_flush_buffers(&video_stream->codec);
    if (audio_stream)
      avcodec_flush_buffers(&audio_stream->codec);
      
    for(i=0;i<nb_istreams;i++) {
      AVInputStream *ist = ist_table[i];

      if (!ist->discard)
	avcodec_close(&ist->st->codec);

      av_free(ist);
    }

    av_free(ist_table);

    ist_table = NULL;
  }

  if (ic) {
    av_close_input_file(ic);
    ic = NULL;
  }

  if (btexture) {
    btexture->format = TA_RGB565;
    memset(btexture->addr, 0, btexture->width*btexture->height*2);
    memset(btexture2->addr, 0, btexture->width*btexture->height*2);

    /* restore original size */
    btexture->width = 1024;
    btexture->height = 512;
    btexture->wlog2 = 10;
    btexture->hlog2 = 9;

    btexture = NULL;

  }

#ifdef USE_DMA
  if (dma_texbuf) {
    free(dma_texbuf);
    dma_texbuf = NULL;
  }
#endif

  if (frame) {
    av_free(frame);
    frame = NULL;
  }


  oldpts = 0;
    
  return 0;
}

static int lua_toto(lua_State * L)
{
  int n;

  if (decode_start(lua_tostring(L, 1)))
    return 0;

#if 0
  for (n=0; /*n<5000 &&*/ !(controler68.buttons & CONT_START); n++)
    if (decode_frame())
      break;
#endif

  decode_stop();

  return 0;
}

static int lua_fifo_size(lua_State * L)
{
  if (ready) {
    lua_pushnumber(L, VID_FIFO_SZ);
    return 1;
  } else
    return 0;
}

static int lua_fifo_used(lua_State * L)
{
  if (ready) {
    lua_pushnumber(L, (vid_fifo_in - vid_fifo_out) & (VID_FIFO_SZ-1));
    return 1;
  } else
    return 0;
}

extern int av_nballocs;
static int lua_stats(lua_State * L)
{
  int total = adecode_j + aplay_j + readstream_j + vdecode_j + vconv_j;

#define P(a)  printf(#a" = %3d\n", a##_j * 1000 / total)
  P(readstream);
  P(adecode);
  P(aplay);
  P(vdecode);
  P(vconv);
#undef P

  return 0;
}

static int lua_checkmem(lua_State * L)
{

  CheckUnfrees();

  printf("Nb allocs = %d\n", av_nballocs);

  return 0;
}

static int lua_codec_load(lua_State * L)
{
  lef_prog_t * p;

  p = lef_load(lua_tostring(L, 1));
  if (p) {
    int (* main)();

    main = lef_find_symbol(p, "_codec_main");
    printf("codec_main = %x\n", main);
    if (main == NULL || main())
      lef_free(p);
    else
      codecs[nbcodecs++] = p;
  }

  return 0;
}


static int lua_force_format(lua_State * L)
{
  char * f = lua_tostring(L, 1);

  if (force_format)
    free(force_format);
  if (f) {
    force_format = strdup(f);
    printf("ffmpeg: Force to format '%s' (on next opened file)\n", force_format);
  } else {
    force_format = NULL;
    printf("ffmpeg: Format guessed (on next opened file)\n", force_format);
  }

  return 0;
}

static int lua_delay(lua_State * L)
{
  audio_delay = lua_tonumber(L, 1);

  return 0;
}

static int lua_settings(lua_State * L)
{
  yuvinit(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
  return 0;
}

static int lua_picbuf_size(lua_State * L)
{
  dma_texbufsz = ((int)lua_tonumber(L, 1)) * 1024;
  if (dma_texbufsz <= 0)
    dma_texbufsz = 1024*1024;

  printf("Picture buffer set to %dKb\n", dma_texbufsz/1024);

  return 0;
}

static int lua_vtime(lua_State * L)
{
  lua_settop(L, 0);
  lua_pushnumber(L, ((int)(realpts/1000))/1000.0f);
  lua_pushnumber(L, timer_ms_gettime64()/1000.0f);
  return 2;
}

static luashell_command_description_t commands[] = {
#ifndef RELEASE
  {
    "ff_toto", 0, "ffmpeg",                    /* long name, short name, topic */
    "ff_toto(filename) : toto",                /* usage */
    SHELL_COMMAND_C, lua_toto                  /* function */
  },
#endif

  {
    "ff_formats", 0, "ffmpeg",                 /* long name, short name, topic */
    "ff_formats() : display supported formats",/* usage */
    SHELL_COMMAND_C, lua_formats               /* function */
  },

  {
    "ff_fifo_size", 0, "ffmpeg",               /* long name, short name, topic */
    "ff_fifo_size() : return video fifo size \n", /* usage */
    SHELL_COMMAND_C, lua_fifo_size             /* function */
  },

  {
    "ff_fifo_used", 0, "ffmpeg",               /* long name, short name, topic */
    "ff_fifo_used() : return video fifo used \n", /* usage */
    SHELL_COMMAND_C, lua_fifo_used             /* function */
  },

  {
    "ff_stats", 0, "ffmpeg",                   /* long name, short name, topic */
    "ff_stats() : display CPU usage statistics \n", /* usage */
    SHELL_COMMAND_C, lua_stats                 /* function */
  },

  {
    "ff_checkmem", 0, "ffmpeg",                /* long name, short name, topic */
    "ff_checkmem() : display unfreed memory \n", /* usage */
    SHELL_COMMAND_C, lua_checkmem              /* function */
  },

  {
    "ff_force_format", "ff_ff", "ffmpeg",      /* long name, short name, topic */
    "ff_force_format(format) : force given format \n", /* usage */
    SHELL_COMMAND_C, lua_force_format          /* function */
  },

  {
    "ff_delay", 0, "ffmpeg",                   /* long name, short name, topic */
    "ff_delay(ms) : set audio delay in milliseconds \n", /* usage */
    SHELL_COMMAND_C, lua_delay                 /* function */
  },

  {
    "ff_settings", 0, "ffmpeg",                /* long name, short name, topic */
    "ff_settings(lo,lm,c1o, c1m, c2o, c2m) : "
    "image luminosity/chroma settings \n",     /* usage */
    SHELL_COMMAND_C, lua_settings              /* function */
  },

  {
    "ff_vtime", 0, "ffmpeg",                   /* long name, short name, topic */
    "ff_vtime() : "
    "return current image's time \n",          /* usage */
    SHELL_COMMAND_C, lua_vtime                 /* function */
  },

  {
    "ff_picbuf_size", "ff_pbs", "ffmpeg",      /* long name, short name, topic */
    "ff_picbuf_size(kb) : "
    "set the size of the picture buffer in Kb \n", /* usage */
    SHELL_COMMAND_C, lua_picbuf_size           /* function */
  },

  {
    "codec_load", "cl", "ffmpeg",              /* long name, short name, topic */
    "codec_load(name) : load a codec plugin \n", /* usage */
    SHELL_COMMAND_C, lua_codec_load            /* function */
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
  "*\0.wmv\0.wma\0.rm\0.mpg\0.mpeg\0.mpc\0.mp3\0.avi\0",   /**< EXtension list */

  start,
  stop,
  decoder,
/*   info, */
};

EXPORT_DRIVER(ffmpeg_driver)