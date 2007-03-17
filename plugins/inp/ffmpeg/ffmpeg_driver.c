#if 0

#include "ffmpeg_driver.c.orig1"

#else

/*
 * $Id: ffmpeg_driver.c,v 1.9 2007-03-17 14:40:29 vincentp Exp $
 *
 * Author : Vincent Penne
 *
 */

/* Define this for benchmark mode (not to a watch video !) */
//#define BENCH

/* Define this to use dma texture transfert, for faster yuv2rgb transformation */
/* (CANNOT BE TURNED OFF ANYMORE) */
#define USE_DMA

/* Define this to allocate one big DMA texture buffer instead of N small ones */
//#define ONE_DMABUF

#define LUMI_TUNING
//#define CHROMA_TUNING

// note : IDCT_AUTO == IDCT_SH4
#define IDCT_ALGO FF_IDCT_AUTO // fastest but lower quality. FIXED : same quality now
/* #define IDCT_ALGO FF_IDCT_SIMPLE */
/* #define IDCT_ALGO FF_IDCT_INT // slightly faster than SIMPLE and same quality */

/* Define this to have the video decoder into a separate thread */
#define DECODE_THREAD

/* Define this to have two flipping textures. This is pretty useless in 
   DMA mode since the texture transfert happens when render is completed */
//#define TEXTURE_FLIP

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

#if defined(OLDFF) || defined(BENCH)
# define NO_VID_FIFO
#endif

#include "sha123_ffdriver.h"

#include "playa.h"
#include "fifo.h"
#include "inp_driver.h"
#include "draw/texture.h"
#include "draw/ta.h"
#include <arch/timer.h>

#include "exceptions.h"
#include "lef.h"
#include "dma.h"


#define THROW_ERROR(error, message) if (1) { printf("ffmpeg ERROR at '%s' (%d) : %s\n", __FILE__, __LINE__, message); goto error; } else


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
static uint64_t shift_pts;
static uint64_t vshift_pts;
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
static semaphore_t * dma_pics_sema;
#ifdef USE_DMA

/* VP : For some reason, it is not stable when launching the dma for more than
   DMA_BSZ bytes
   UPDATE : problem solved by blocking ta rendering during the dma transfert */
#define DMA_BSZ (2*64*64) /* size of a dma block */

#define DMA_ONESHOT

typedef struct dma_pict {
  uint8_t * buf;
  uint64_t pts;
} dma_pict_t;

static uint32_t dma_tex;
static int dma_sendn;
static int dma_sendoff;
static int dma_sendinc;
static uint8_t * dma_buf;
static uint8_t * dma_texbuf;
static int dma_texbufsz = 1024*1024;
static dma_pict_t dma_pics[32];
static int dma_picmax;
static int dma_picin;
static int dma_picout;
static int dma_picsz;
static int dma_picrealsz;
#endif

static int video_frame_counter;

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
static void vidstream_thread(void *userdata);

#ifdef DECODE_THREAD
static kthread_t * vdecode_thd;
static int vdecode_exit;
static void vdecode_thread(void *userdata);
static semaphore_t * vid_fifo_sema;
#endif

static lef_prog_t * codecs[32];
static int nbcodecs;

static int adaptative_synchro = 1;
static int limit_fifo;

/* from playa.c to force the player priority */
extern int playa_force_prio;


/* from file.c in libavformat */
extern int ff_packet_size;
extern int ff_cur_fd;

/* symbol retrieved via lef symbols */
static const char * (* http_get_header)(file_t f, int i);

playa_info_t *glb_info;
int info_need_update;

uint64_t toto = 1800000000000UL;
uint64_t toto2 = 1600000000010UL;

static yuvinit(float lo, float lm, float c1o, float c1m, float c2o, float c2m);

static int init(any_driver_t *d)
{
  int n = 0;
  EXPT_GUARD_BEGIN;

  dbglog(DBG_DEBUG, "ffmpeg : Init [%s]\n", d->name);
  ready = 0;

  av_register_all();

#ifndef OLDFFx
  sha123_ffdriver_init();
#endif

  yuvinit(10, 1, 0, 1, 0, 1);

  dma_done = sem_create(0);
  dma_pics_sema = sem_create(0);
  vidstream_thd = thd_create(vidstream_thread, 0);
  if (vidstream_thd)
    thd_set_label(vidstream_thd, "Video-picture-thd");
  //vidstream_thd->prio2 = 9;

#ifdef DECODE_THREAD
  vid_fifo_sema = sem_create(0);
  {
    int old = thd_default_stack_size;
    thd_default_stack_size = 64*1024;
    vdecode_thd = thd_create(vdecode_thread, 0);
    if (vdecode_thd)
      thd_set_label(vdecode_thd, "Video-decode-thd");
    thd_default_stack_size = old;
  }
#endif


#if 0
  /* testing 64 bits integers */
  printf("%d\n", toto > toto2);
  printf("%g\n", (float) toto);
  toto >>= 20;
  dummyf();
  printf("%g\n", (float) toto);
  dummyf();
  printf("%d\n", (uint) (toto));
#endif




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

#ifdef DECODE_THREAD
  vdecode_exit = 1;
  sem_signal(vid_fifo_sema);
  while (vdecode_exit)
    thd_pass();
  sem_destroy(vid_fifo_sema);
#endif

  vidstream_exit = 1;
  sem_signal(dma_pics_sema);
  while (vidstream_exit)
    thd_pass();
  sem_destroy(dma_done);
  sem_destroy(dma_pics_sema);

#ifndef OLDFFx
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

static int info(playa_info_t *info, const char *fname)
{
  return id3_info(info, fname);
}

static int start(const char *fn, int track, playa_info_t *info)
{
  glb_info = info;

  EXPT_GUARD_BEGIN;

  stop();

  if(id3_info(info, fn)<0) {
    playa_info_free(info);
  }

  audio_buf = (u32_t *) av_malloc(AUDIO_SZ);
  if (audio_buf == NULL)
    THROW_ERROR(error, "Insuficient memory");
  memset(audio_buf, 0, AUDIO_SZ);

  //goto error;
  if (decode_start(fn))
    THROW_ERROR(error, "decode_start returned error");

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



  /* if the file is an http stream, then retrieve information from
     the header for possible shoutcast info */

  /* find this function which should be defined in the net plugin */
  http_get_header = lef_find_symbol_all("_http_get_header");

  if (ff_cur_fd >= 0 && http_get_header) {
    int i;
    const char * line;
    char * url[256];

    url[0] = 0;
    i = 0;
    while ( line=http_get_header(ff_cur_fd, i), line ) {
      char * buf[256];
      const char * p = buf;
      strcpy(buf, line);
      while (*p && *p!=':')
	p++;

      if (*p == ':') {
	*p = 0;
	p++;

	//printf("'%s' --> '%s'\n", buf, p);
	  
	if (!stricmp(buf, "icy-name")) {
	  playa_info_title(info, p);
	} else if (!stricmp(buf, "icy-genre")) {
	  playa_info_genre(info, p);
	} else if (!stricmp(buf, "icy-url")) {
	  strcpy(url, p);
	  playa_info_comments(info, p);
	} else if (!stricmp(buf, "icy-br")) {
	  if (audio_stream) {
	    audio_stream->codec.bit_rate = atoi(p)*1000;
	    avcodec_string(buf, sizeof(buf), &audio_stream->codec, 0);
	    playa_info_comments   (info, buf);
	  }
	}

	if (url[0]) {
	  sprintf(buf, "%s - %s", url, playa_info_comments(info, (char *)-1));
	  playa_info_comments   (info, buf);
	}
      }

      i++;
    }
  }

    



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

  glb_info = info;

  EXPT_GUARD_BEGIN;

  if (!ready) {
    EXPT_GUARD_RETURN INP_DECODE_ERROR;
  }
        
  n = 
    decode_frame()/* ||
    decode_frame()*/;

  EXPT_GUARD_CATCH;

  EXPT_GUARD_RETURN INP_DECODE_ERROR;

  EXPT_GUARD_END;
  if (n)
    n = INP_DECODE_END;
#ifndef DECODE_THREAD
  else if (video_stream)
    n = INP_DECODE_CONT;
#endif
  else
    n = 0;

  if (info_need_update) {
    n += INP_DECODE_INFO;
    info_need_update = 0;
  }

  return n;
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

	printf("%s, ", name);
#if 0        
        printf(
            "(%s%s) %s, ", 
            decode ? "D":"", 
            encode ? "E":"", 
            name);
#endif
    }
    printf("\n");

#if 0
    printf("Image formats:\n");
    for(image_fmt = first_image_format; image_fmt != NULL; 
        image_fmt = image_fmt->next) {
        printf(
            "(%s%s) %s, ",
            image_fmt->img_read  ? "D":"",
            image_fmt->img_write ? "E":"",
            image_fmt->name);
    }
    printf("\n");
#endif

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

        printf("%s, ", p2->name);
#if 0
        printf(
            "(%s%s%s%s%s%s) %s, ", 
            decode ? "D": (/*p2->decoder ? "d":*/""), 
            encode ? "E":"", 
            p2->type == CODEC_TYPE_AUDIO ? "A":"V",
            cap & CODEC_CAP_DRAW_HORIZ_BAND ? "S":"",
            cap & CODEC_CAP_DR1 ? "D":"",
            cap & CODEC_CAP_TRUNCATED ? "T":"",
            p2->name);
#endif
       /* if(p2->decoder && decode==0)
            printf(" use %s for decoding", p2->decoder->name);*/
        //printf("\n");
    }
    printf("\n");

#if 0
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
    printf("\n");
    printf(
"Note, the names of encoders and decoders dont always match, so there are\n"
"several cases where the above table shows encoder only or decoder only entries\n"
"even though both encoding and decoding are supported for example, the h263\n"
"decoder corresponds to the h263 and h263p encoders, for file formats its even\n"
"worse\n");
#endif
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

    limit_fifo = 0;

    audio_size = 0;
    audio_ptr = 0;

    shift_pts = -1;
    vshift_pts = -1;

    video_frame_counter = 0;

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
      THROW_ERROR(error, "Could not find codec parameters");
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
	    enc->idct_algo = IDCT_ALGO;
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
    if (ist_table == NULL)
      THROW_ERROR(error, "Insuficient memory");

    for(i=0;i<nb_istreams;i++) {
        AVInputStream *ist = av_mallocz(sizeof(AVInputStream));
        if (!ist)
	  THROW_ERROR(error, "Insuficient memory");

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
                fprintf(stderr, "ffmpeg : Unsupported codec (id=%d) for input stream #%d\n", 
                        ist->st->codec.codec_id, i);
                goto openerr;
            }
	    
	    EXPT_GUARD_BEGIN;
	    
	    res = avcodec_open(&ist->st->codec, codec);
	    
	    EXPT_GUARD_CATCH;

	    res = -1;
	    
	    EXPT_GUARD_END;

            if (res < 0) {
                fprintf(stderr, "ffmpeg : Error while opening codec for input stream #%d\n", 
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
      THROW_ERROR(error, "No audio nor video to play :(");
    }

    frame = avcodec_alloc_frame();
    if (frame == NULL)
      THROW_ERROR(error, "Insuficient memory");

    if (video_stream) {
      int texid;

      printf("Got a video stream ! Format %dx%d (%s)\n",
	     video_stream->codec.width, video_stream->codec.height, 
	     avcodec_get_pix_fmt_name(video_stream->codec.pix_fmt));


#ifdef TEXTURE_FLIP
      texid = texture_get("__background2__");
#else
      texid = texture_get("__background__");
#endif
      if (texid < 0) {
#ifdef TEXTURE_FLIP
	texid = texture_create_flat("__background2__",1024,512,0xFFFFFFFF);
#else
	texid = texture_create_flat("__background__",1024,512,0xFFFFFFFF);
#endif
      }
      if (texid < 0)
	THROW_ERROR(error, "Could not create secondary texture");
	
      btexture2_id = texid;
      btexture2 = texture_fastlock(texid, 1);
      if (btexture2 == NULL)
	THROW_ERROR(error, "Could not lock secondary texture");
      texture_release(btexture2);


      texid = texture_get("__background__");
      if (texid < 0) {
	texid = texture_create_flat("__background__",1024,512,0xFFFFFFFF);
      }
      if (texid < 0)
	THROW_ERROR(error, "Could not get primary texture");

      btexture_id = texid;
      btexture = texture_fastlock(texid, 1);
      if (btexture == NULL)
	THROW_ERROR(error, "Could not lock primary texture");
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

	uint32_t pix = video_stream->codec.pix_fmt == PIX_FMT_YUV420P? 
	  0x00800080 : 0;

	/* clear textures */
	{
	  uint32_t * p1 = btexture->addr;
	  uint32_t * p2 = btexture2->addr;
	  n = btexture->width*btexture->height/2;
	  for (i=0; i<n; i++) {
	    *p1++ = pix;
#ifdef TEXTURE_FLIP
	    *p2++ = pix;
#endif
	  }
	}

	thd_sleep(4*1000/60);
	
	/* adjust texture size */
	printf("Texture size %dx%d\n", nw, nh);
	btexture->width = nw;
	btexture->height = nh;
	btexture2->width = nw;
	btexture2->height = nh;
	btexture->wlog2 = lw;
	btexture->hlog2 = lh;
	btexture2->wlog2 = lw;
	btexture2->hlog2 = lh;

	btexture->twiddled = 0;  /* Force non twiddle */
	btexture->twiddlable = 0;  /* Force non twiddle */
	//      btexture->format = TA_YUV422;
	btexture2->twiddled = 0;  /* Force non twiddle */
	btexture2->twiddlable = 0;  /* Force non twiddle */
	//      btexture->format = TA_YUV422;

	/* prepare pictures circular buffer */
      again:
	{
#ifdef DMA_ONESHOT
	  int size = (2*nw*h + 63) & (~(63));
#else
	  int size = (2*nw*h + DMA_BSZ - 1) & (~(DMA_BSZ-1));
#endif

	  /* this value is necessary to flush the cache */
	  dma_picrealsz = 2*nw*h;

	  dma_picmax = dma_texbufsz / size;
	  dma_picin = dma_picout = 0;
	  dma_picsz = size;

	  if (dma_picmax < 2) {
	    dma_texbufsz = dma_picsz*2;
	    printf("Increasing picture buffer to %dKb\n", dma_texbufsz/1024);
	    goto again;
	  }

#ifdef ONE_DMABUF
	  dma_texbuf = memalign(64, dma_texbufsz);
	  if (dma_texbuf == NULL) {
	    THROW_ERROR(error, "ffmpeg : Not enough memory for DMA buffer");
	  }
#endif

	  printf("Preparing %d picture buffers of size %gKb\n", 
		 dma_picmax, dma_picsz/1024.0f);

	  for (i=0; i<dma_picmax; i++) {
	    uint32_t * p;
	    int j;

#ifdef ONE_DMABUF
	    dma_pics[i].buf = dma_texbuf + i*size;
#else
	    dma_pics[i].buf = memalign(64, size);
	    if (dma_pics[i].buf == NULL) {
	      THROW_ERROR(error, "Not enough memory for DMA buffers\n");
	    }
#endif

	    p = (uint32_t *) dma_pics[i].buf;
	    //pix = pix ^ 0x80;
	    for (j=0; j<dma_picsz/4; j++) {
	      *p++ = pix;
	    }
	  }
	}

	//playa_force_prio = 6;

	/* Reset background vertexes. */
	shell_command(" dolib ('background')"
/* 		      " background_set_texture(background)" */
/* 		      " background_set_texture('none')" */
		      " set_vertex(background.vtx[1],{ 0, 0, 0, 1, nil, nil, nil, nil, 0, 0 })"
		      " set_vertex(background.vtx[2],{ 1, 0, 0, 1, nil, nil, nil, nil, 1, 0 })"
		      " set_vertex(background.vtx[3],{ 0, 1, 0, 1, nil, nil, nil, nil, 0, 1 })"
		      " set_vertex(background.vtx[4],{ 1, 1, 0, 1, nil, nil, nil, nil, 1, 1 })"
		      " background:draw()"
		      );

	/* Scale the background */
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
      if (audio_next_pts == AV_NOPTS_VALUE)
	printf("AUDIO NO PTS !!\n");
      audio_pts = audio_next_pts 
	- ((int64_t)audio_fifo_used) * AV_TIME_BASE / audio_fifo_rate
	+ ((int64_t)audio_size) * AV_TIME_BASE / audio_fifo_rate
	;

/*       if (shift_pts == -1) */
/* 	shift_pts = audio_pts; */

      audio_pts -= vshift_pts;

      audio_size -= n;
      audio_ptr += n * audio_stream->codec.channels / 2;
    }
    av_unlock();
  }

}


#define VID_FIFO_SZ 256

static int vid_fifo_limit = 32;
static int vid_fifo_in;
static int vid_fifo_out;
static AVPacket vid_fifo[VID_FIFO_SZ];

static int vid_fifo_used()
{
  return (vid_fifo_in - vid_fifo_out) & (VID_FIFO_SZ-1);
}

static int pic_fifo_used()
{
  return (dma_picin - dma_picout + dma_picmax) % dma_picmax;
}

static AVPacket * vid_fifo_get()
{
#ifdef DECODE_THREAD
  sem_wait(vid_fifo_sema);
#endif
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

static int vid_fifo_room()
{
  return vid_fifo_used() < vid_fifo_limit;
  //  return ((vid_fifo_in+1)&(VID_FIFO_SZ-1)) != vid_fifo_out;
}

static int vid_fifo_put(AVPacket * pkt)
{
  if (((vid_fifo_in+1)&(VID_FIFO_SZ-1)) != vid_fifo_out) {
    //vid_fifo[vid_fifo_in] = *pkt;
    //printf("put packet #%d\n", vid_fifo_in);
    vid_fifo_in = (vid_fifo_in+1)&(VID_FIFO_SZ-1);
#ifdef DECODE_THREAD
    sem_signal(vid_fifo_sema);
    thd_schedule_next(vdecode_thd);
#endif
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
extern int ta_block_render;
static void dma_cb(void * p)
{
  if (dma_sendn > 0) {
    int sz = (dma_sendn + 63)&(~63);
#ifndef DMA_ONESHOT
    //if (sz > dma_sendinc)
      sz = dma_sendinc;
#endif
    pvr_txr_load_dma(dma_buf+dma_sendoff, dma_tex+dma_sendoff, 
		     sz, 0, dma_cb, 0);
    dma_sendn -= sz;
    dma_sendoff += sz;
    //vid_border_color(dma_sendn/64, dma_sendoff/128, 128);

    ta_block_render = 1;
  } else {
    dma_sendn = 0;

    /* swap textures */
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

    ta_block_render = 0;

    //vid_border_color(0, 0, 0);
    sem_signal(dma_done);
    thd_schedule(1, 0);
  }
}

void renderdone_cb()
{
  dma_cb(0);
  ta_render_done_cb = NULL;
}

void cleartex_cb()
{
  memset(btexture->addr, 0, btexture->width*btexture->height*2);
#ifdef TEXTURE_FLIP
  memset(btexture2->addr, 0, btexture->width*btexture->height*2);
#endif
  ta_render_done_cb = NULL;

  sem_signal(dma_done);
  thd_schedule(1, 0);
}

#endif

static int got_band;
static void bandyuv420p(struct AVCodecContext *s,
			const AVFrame *src, int offset[4],
			int y, int type, int height)
{
  int i;
  int w = s->width;
  int h = height;
  u32_t * srcY1 = (u8_t *) (frame->data[0]+offset[0]);
  u32_t * srcY2 = (u8_t *) (frame->data[0]+offset[0]);
  u32_t * srcU = (s8_t *) (frame->data[2]+offset[2]); 
  u32_t * srcV = (s8_t *) (frame->data[1]+offset[1]);
#ifdef USE_DMA
  u32_t * dst1 = (u32_t *) dma_pics[dma_picin].buf;
  u32_t * dst2 = (u32_t *) dma_pics[dma_picin].buf;
#else
  u32_t * dst1 = (u32_t *) btexture2->addr;
  u32_t * dst2 = (u32_t *) btexture2->addr;
#endif
  int stride = btexture->width;
  int srcs = (frame->linesize[0]+3)&~3;
  int srcsU = (frame->linesize[1]+3)&~3;
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

  got_band++;
  dst1 += (y+h-2)*stride / 2;
  dst2 += (y+h-1)*stride / 2;
  srcs *= 2;

  srcY1 = (u32_t *) ( ((u8_t *) srcY1) + (h/2-1)*srcs );
  srcY2 = (u32_t *) ( ((u8_t *) srcY2) + (h/2-1)*srcs + srcs/2 );
  srcU =  (u32_t *) ( ((u8_t *) srcU)  + (h/2-1)*srcsU );
  srcV =  (u32_t *) ( ((u8_t *) srcV)  + (h/2-1)*srcsU );

  i = h/2;
  while (i--) {
    int j;
    j = ww;
    while(j--) {

#if 1

#ifdef LUMI_TUNING

#if 1

      /* with luminosity and chroma tables, and, surprise, it's just as fast
	 as the non-tunnable version !
	 UPDATE : not true anymore when using DMA, the non-tunable version is now faster
	 because main memory writing is faster. However with luminosity only, the difference is
	 small. */
      uint Y0, Y1, U, V;
      uint u, v;

      Y0 = *srcY1++;
      Y1 = *srcY2++;
      U = *srcU++;
      V = *srcV++;

#ifdef CHROMA_TUNING
      u = (ct1[(U&0xff)])<<16;
      v = (ct2[(V&0xff)]);
#else
      u = ((U&0xff))<<16;
      v = ((V&0xff));
#endif
      *dst1++ = ((lt[(Y0>>8)&0xff]<<24)) | (u) | ((lt[(Y0&0xff)]<<8)) | (v);
      *dst2++ = ((lt[(Y1>>8)&0xff]<<24)) | (u) | ((lt[(Y1&0xff)]<<8)) | (v);

#ifdef CHROMA_TUNING
      u = (ct1[(U>>8)&0xff])<<16;
      v = (ct2[(V>>8)&0xff]);
#else
      u = ((U<<8)&m16);
      v = ((V>>8)&0xff);
#endif
      *dst1++ = (((lt[(Y0)>>24]))<<24) | u | ((lt[(Y0>>16)&0xff]<<8)) | (v);
      *dst2++ = (((lt[(Y1)>>24]))<<24) | u | ((lt[(Y1>>16)&0xff]<<8)) | (v);



      Y0 = *srcY1++;
      Y1 = *srcY2++;
#ifdef CHROMA_TUNING
      u = (ct1[(U>>16)&0xff])<<16;
      v = (ct2[(V>>16)&0xff]);
#else
      u = ((U)&m16);
      v = ((V>>16)&0xff);
#endif
      *dst1++ = ((lt[(Y0>>8)&0xff]<<24)) | (u) | ((lt[(Y0&0xff)]<<8)) | (v);
      *dst2++ = ((lt[(Y1>>8)&0xff]<<24)) | (u) | ((lt[(Y1&0xff)]<<8)) | (v);

#ifdef CHROMA_TUNING
      u = ((ct1[(U)>>24]))<<16;
      v = ct2[(V)>>24];
#else
      u = (((U)>>8)&m16);
      v = (V)>>24;
#endif
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
      /* no luminosity/chroma tuning, not faster than with it
	 UPDATE : actually , it is faster in DMA mode */
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

    srcY1 = (u32_t *) ( ((u8_t *) srcY1) + (-srcs - (ww<<3) ) );
    srcY2 = (u32_t *) ( ((u8_t *) srcY2) + (-srcs - (ww<<3) ) );
    srcU  = (u32_t *) ( ((u8_t *) srcU)  + (-srcsU - (ww<<2) ) );
    srcV  = (u32_t *) ( ((u8_t *) srcV)  + (-srcsU - (ww<<2) ) );
    dst1 += -stride - (ww<<2);
    dst2 += -stride - (ww<<2);
  }
}

#include "controler.h"
extern controler_state_t controler68;

#if 0
#include "dsputil.h"

DSPContext * cur_dspcontext;
DSPContext fast_dspcontext;
DSPContext precise_dspcontext;

static int qfast_idct = 1;

static void fast_idct(DSPContext* c)
{
  //return;
  c->idct_put = fast_dspcontext.idct_put;
  c->idct_add = fast_dspcontext.idct_add;
  c->idct     = fast_dspcontext.idct;
  c->idct_permutation_type= FF_NO_IDCT_PERM;
  if (!qfast_idct) {
    qfast_idct = 1;
    printf("Switched to fast IDCT (SH4 matrix version)\n");
  }
}

static void precise_idct(DSPContext* c)
{
  return;
  c->idct_put= precise_dspcontext.idct_put;
  c->idct_add= precise_dspcontext.idct_add;
  c->idct    = precise_dspcontext.idct;
  c->idct_permutation_type= FF_NO_IDCT_PERM;
/*   c->idct_put= simple_idct_put; */
/*   c->idct_add= simple_idct_add; */
/*   c->idct    = simple_idct; */
/*   c->idct_permutation_type= FF_NO_IDCT_PERM; */
/*   c->idct_put= ff_jref_idct_put; */
/*   c->idct_add= ff_jref_idct_add; */
/*   c->idct     = j_rev_dct; */
/*   c->idct_permutation_type= FF_LIBMPEG2_IDCT_PERM; */
  if (qfast_idct) {
    qfast_idct = 0;
    printf("Switched to precise IDCT (integer version)\n");
  }
}
#endif


static int vidstream_decode(AVPacket * pkt)
{
  int i, w, h;
  int len1, got_picture = 0;
  static int lj;
  int nj;

  lj = thd_current->jiffies;

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

#if 0
    {
      int pic_used = pic_fifo_used();

      if (pic_used < 2 * dma_picmax / 3) {
	fast_idct(cur_dspcontext);
      } else {
	precise_idct(cur_dspcontext);
      }
    }
#endif

    uint8_t * data;
    uint32_t size;
    data = pkt->data;
    size = pkt->size;
    do {
      got_band = 0;
      if (video_stream->codec.pix_fmt == PIX_FMT_YUV420P) {
	video_stream->codec.draw_horiz_band = bandyuv420p;
      } else {
	video_stream->codec.draw_horiz_band = 0;
      }

      len1 = avcodec_decode_video(&video_stream->codec, 
				  frame, &got_picture, 
				  data, size);
#ifdef OLDFF
      {
	static uint64_t pts;
	pkt->pts = pts;
	pts += AV_TIME_BASE * video_stream->codec.frame_rate_base / video_stream->codec.frame_rate;
      }
#endif
/*       printf("video : size = %d, len1 = %d, got_picture = %d, pts = %d, fps = %d\n",  */
/* 	     size, len1, got_picture, (int)(pkt->pts/1000),  */
/* 	     video_stream->codec.frame_rate / video_stream->codec.frame_rate_base); */
      if (len1 > 0) {
	data += len1;
	size -= len1;
      }
    } while (len1 > 0 && size > 0);
    //printf("len = %d\n", len1);
  }
  nj = thd_current->jiffies;
  vdecode_j += nj-lj;
  lj = nj;

  if (got_picture) {

    lj = thd_current->jiffies;

    //vid_border_color(0xff,0xff,0);
    if (video_stream->codec.pix_fmt == PIX_FMT_YUV422) {
      btexture->format = TA_YUV422;

      char * src = (char *) frame->data[0];
#ifdef USE_DMA
      char * dst = (u32_t *) dma_pics[dma_picin].buf;
#else
      char * dst = (char *) btexture2->addr;
#endif
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
#ifdef USE_DMA
      pic.data[0] = dma_pics[dma_picin].buf;
#else
      pic.data[0] = btexture2->addr;
#endif
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
	if (!got_band) {
	  static int offsets[4] = {0,0,0,0};
	  bandyuv420p(&video_stream->codec, frame, offsets, 0, 0, h);
	  //yuv420pto422(w, h);
	}

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
	      
  nj = thd_current->jiffies;
  //vdecode_j += nj-lj;
  lj = nj;

  return got_picture;
}

static void vidstream_thread(void *userdata)
{
  while (!vidstream_exit) {
    sem_wait(dma_pics_sema);
    if (vidstream_exit)
      break;
    assert (dma_picin != dma_picout);
    {
      dma_pict_t * pic = dma_pics + dma_picout;
      uint64_t pts = pic->pts;
      
      /* time synchronisation */
      while ( audio_stream != NULL && pts && 
	      !vidstream_exit && !vidstream_skip ) {
	int used;
	av_lock();
	used = fifo_used();
	if (used == 0) {
	  //printf("SYNCHRO BUG : audio fifo empty\n");
	  break;
	}
#if 0
	if (pts * audio_fifo_rate / AV_TIME_BASE <= 
	    audio_pts * audio_fifo_rate / AV_TIME_BASE 
	    - /*audio_fifo_used + */used 
	    + 6*audio_fifo_rate/60 /* two screen frames */
	    )
#else

#if 0
	  if (pts * 60 / AV_TIME_BASE <= 
	      audio_pts * 60 / AV_TIME_BASE 
	      - audio_fc + ta_state.frame_counter 
	      + 3 /* six screen blanking */
	      )
#else

	int iaudio_pts = audio_pts/1000;
	int ipts = pts/1000;
	int ioldpts = oldpts/1000;
#define oldpts ioldpts
#define pts ipts
#define audio_pts iaudio_pts
#define audio_time ((int)audio_time)
#define uint64 uint32
#define uint64_t uint32_t
#define int64 int32
#define int64_t int32_t
#define AV_TIME_BASE 1000

        static uint64_t oldtime;
	static float diffavg;
	static float diffavg2;
	static int nbdiffs, idiffs;
	static int64_t olddiff;
	static uint64_t last_second;
	uint64_t time;
	time = timer_ms_gettime64();
	
/* 	static uint64_t last_second2; */
/* 	if (time-last_second2 >= 100) { */
/* 	  last_second2 = time; */
/* 	  printf("A/V %g, A %g, V %g\n",  */
/* 		 (1/1000.0f)*((pts-audio_pts)), */
/* 		 (1/1000.0f)*((audio_pts)), */
/* 		 (1/1000.0f)*((pts))); */
/* 	} */

	if ((1||adaptative_synchro || (time-last_second < 1000)) && oldpts) {
	  int64_t diff;
	  uint64_t dtime;
	  static int coef = (1.055) * (1<<16);
	  //static int coef = (1<<16);
	  static int rcoef = (1<<16);
	  static int oldcoef = (1<<16);
#define NBDIFFS 10
	  static float diffs[NBDIFFS];
	  static float diffs2[NBDIFFS];

	  if (!adaptative_synchro)
	    ;//rcoef = (1.055) * (1<<16);

	  dtime = (time - oldtime) * rcoef >>16;

	  if ( pts / (AV_TIME_BASE/1000) <= 
	       oldpts / (AV_TIME_BASE/1000)
	       - dtime
	      ) {
	    //oldpts += ( (time - oldtime) * rcoef >>16) * (AV_TIME_BASE/1000);

	    diff = 
	      ((uint64_t)(pts/* - audio_pts*/)) / (AV_TIME_BASE/1000) -
	      ( audio_pts / (AV_TIME_BASE/1000)
	       + time - audio_time 
	       + 3*1000/60)
	      + 125 /* empirical value */
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

/* 	    static uint64 opts = 0; */
/* 	    if (!opts) opts = pts; */

	    static toto; toto++; if ((toto&7)==0)
/* 	      printf("pts %g avg %g avg2 %g coeff %g dt %d\n",  */
/* 		     (1/1000.0f)*((pts-audio_pts)/(AV_TIME_BASE/1000)), avg, avg2,  */
/* 		     ((float) coef)/(1<<16), (int)dt); */

	    /* 	      avg = (1 - 0.1*avg); */
	    /* 	      coef *= avg*avg; */

	    /* 	      avg2 = (1 - 0.1*avg2); */
	    /* 	      coef *= avg2; */

	    coef -= (10*avg*avg*avg + avg2*avg2*avg2) * (1<<16);

	    const int coefmin = 0.8*(1<<16);
	    const int coefmax = 1.3*(1<<16);
	    if (coef < coefmin)
	      coef = coefmin;
	    if (coef > coefmax)
	      coef = coefmax;

	    rcoef = coef; //(coef + oldcoef) / 2;
	    oldcoef = coef;

	    oldtime = time;

	    break;
	  }

#undef oldpts
#undef pts
#undef audio_pts
#undef audio_time
#undef uint64
#undef uint64_t
#define AV_TIME_BASE 1000000

	} else {
 	  last_second = time;
	  if (pts / (AV_TIME_BASE/1000) <= 
	      audio_pts / (AV_TIME_BASE/1000)
	      - audio_time + time 
	      + 3*1000/60 /* three screen blanking */
	      - 125 /* empirical value */
	      - audio_delay
	      ) {
	    diffavg = 0;
	    diffavg2 = 0;
	    nbdiffs = idiffs = 0;
	    olddiff = 0;

	    printf("A/V %g, A %g, V %g\n", 
		   (1/1000.0f)*(pts/1000-audio_pts/1000),
		   (1/1000.0f)*(audio_pts/1000),
		   (1/1000.0f)*(pts/1000));

	    oldtime = time;
	    oldpts = pts;

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

      /* launch dma */
#if 1
      {
	dma_chain_t * chain;

	dma_cache_flush(dma_pics[dma_picout].buf, dma_picrealsz);
	chain = dma_initiate(btexture2->ta_tex, 
			     dma_pics[dma_picout].buf,
			     dma_picsz, DMA_TYPE_VRAM);
	dma_wait(chain);
      }
#else
      dma_sendn = dma_picsz;
      dma_sendoff = 0;
      dma_buf = dma_pics[dma_picout].buf;
      //dma_sendinc = (dma_sendn + 31) & (~31);
      dma_sendinc = DMA_BSZ; 
      dma_tex = btexture2->ta_tex;
      ta_render_done_cb = renderdone_cb;
      //dma_cb(0);

      /* wait for transfert completion */
      sem_wait(dma_done);
#endif

      dma_picout = ((dma_picout+1)%dma_picmax);
      video_frame_counter++;
    }
  }

  printf("ffmpeg: vidstream_thread exit\n");
  vidstream_exit = 0;
}


#ifdef DECODE_THREAD
static void vdecode_thread(void * dummy)
{
  while (!vdecode_exit) {
    int got_picture;
    AVPacket * pkt = vid_fifo_get();

    if (pkt == NULL) {
      thd_pass();
      continue;
    }

    if (!vidstream_skip)
      got_picture = vidstream_decode(pkt);
    else
      got_picture = 0;

    if (got_picture) {
      while (((dma_picin+1)%dma_picmax) == dma_picout)
	thd_pass();

      if (pkt->pts == AV_NOPTS_VALUE)
	printf("VIDEO NO PTS !!\n");

      if (vshift_pts == -1)
	vshift_pts = pkt->pts;

      dma_pics[dma_picin].pts = pkt->pts - vshift_pts;
      dma_picin = ((dma_picin+1)%dma_picmax);
      sem_signal(dma_pics_sema);
      thd_schedule_next(vidstream_thd);
    }

    av_free_packet(pkt);
    
    vid_fifo_getstep();
  }

  printf("ffmpeg: vdecode_thread exit\n");
  vdecode_exit = 0;
}
#endif


int decode_frame()
{
/*   static int n; */
/*   int i; */
  int len1;
  int lj, nj;
  int audio_used;

  audio_used = fifo_used();


  AVInputStream *ist;

  //AVPacket pkt;
  AVPacket * ppkt;

  lj = thd_current->jiffies;

#ifdef DECODE_THREAD
  if (video_stream) {
    int used = pic_fifo_used();
    int size = dma_picmax;

    if (used <= size/4)
      vdecode_thd->prio2 = 7;
    else if (used <= size/2)
      vdecode_thd->prio2 = 5;
    else
      vdecode_thd->prio2 = 3;

/*     vdecode_thd->prio2 = thd_current->prio2; */
  } else
      vdecode_thd->prio2 = 1;
#endif

#if !defined(NO_VID_FIFO) && !defined(DECODE_THREAD)
  if (//vid_fifo_used() > VID_FIFO_SZ/2 &&
      (!vid_fifo_room() || pic_fifo_used() <= dma_picmax/2) && 
      ((dma_picin+1)%dma_picmax) != dma_picout &&
      vid_fifo_get()) {

    AVPacket * pkt = vid_fifo_get();
    int got_picture;

    got_picture = vidstream_decode(pkt);

    if (got_picture) {
      dma_pics[dma_picin].pts = pkt->pts;
      dma_picin = ((dma_picin+1)%dma_picmax);
      sem_signal(dma_pics_sema);
    }

    av_free_packet(pkt);
    
    vid_fifo_getstep();
    return 0;
  }
#endif

#define pkt (*ppkt)
  play_audio();
  nj = thd_current->jiffies;
  aplay_j += nj-lj;
  lj = nj;

  if (audio_size > 0)
    return 0;

  if (!vid_fifo_room())
    return 0;

  ppkt = vid_fifo + vid_fifo_in;

  if (limit_fifo && fifo_used() >= limit_fifo)
    return 0;

  if (av_read_frame(ic, &pkt) < 0) {
    if (vid_fifo_used() > 0)
      return 0;

    return 1;
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

  if (ist->st == audio_stream && pkt.size > 0) {
    int data_size = 0;
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
m
      if (len1 > 0)
	data_size += s;
    }

    static uint64_t pts;
    pkt.pts = pts;
    pts += ((uint64_t)data_size) * AV_TIME_BASE / 
      (audio_sample_rate * 2 * audio_stream->codec.channels);

/*     printf("audio : len1 = %d, data_size = %d, pts = %d\n",  */
/* 	   len1, data_size, (int)(pkt.pts/1000)); */
#endif

    nj = thd_current->jiffies;
    adecode_j += nj-lj;
    lj = nj;

    //printf("len = %d, data_size = %d\n", len1, data_size);
    if (/*len1 > 0 && */data_size > 0) {
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
    vidstream_thd->prio2 = 6;

#if defined(NO_VID_FIFO) && !defined(DECODE_THREAD)
    for ( ; ; ) {
      if (((dma_picin+1)%dma_picmax) != dma_picout)
	break;
      thd_pass();
    }
    
    int got_picture = vidstream_decode(&pkt);

    if (got_picture) {
      dma_pics[dma_picin].pts = pkt.pts;
      dma_picin = ((dma_picin+1)%dma_picmax);
      sem_signal(dma_pics_sema);
    }
#else
    if (vid_fifo_put(&pkt))
      printf("BUG ! no more room in video packet fifo\n");
    else
      return 0;
#endif
  }

 discard_packet:
  av_free_packet(&pkt);
#undef pkt

  return 0;
}

int decode_stop()
{
  int i;

  vidstream_skip = 1;

#ifdef DECODE_THREAD
/*   int used = vid_fifo_used(); */
/*   int fc = ta_state.frame_counter; */
  while (vid_fifo_in != vid_fifo_out) {
    thd_pass();

#if 0
    if (fc != ta_state.frame_counter) {
      int w = video_stream->codec.width;
      int h = video_stream->codec.height;
      char buf[256];

      /* Scale the background */
      sprintf(buf, 
	      "dl_set_trans(background.dl, "
	      "mat_scale(%d*640/%d,%d*640/%d,1)"
	      "*mat_trans(0, (480-%d*640/%d)/2, 0))",
	      btexture->width, w * vid_fifo_used() / used, btexture->height, w, h, w);
      shell_command(buf);

      fc = ta_state.frame_counter;
    }
#endif
  }
#endif

  while (dma_picin != dma_picout)
    thd_pass();

  vidstream_skip = 0;

#ifndef DECODE_THREAD
  while (vid_fifo_get()) {
    av_free_packet(vid_fifo_get());
    vid_fifo_getstep();
  }
#endif

  dma_picin = dma_picout = 0;

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
    uint32_t pix = video_stream->codec.pix_fmt == PIX_FMT_YUV420P? 
      0x00800080 : 0;

    /* clear textures */
    {
      uint32_t * p1 = btexture->addr;
      uint32_t * p2 = btexture2->addr;
      int n;
      n = 1024*512/2;
      for (i=0; i<n; i++) {
	*p1++ = pix;
#ifdef TEXTURE_FLIP
	*p2++ = pix;
#endif
      }
    }

    /* restore original size and pixel format */
    btexture->width = 1024;
    btexture->height = 512;
    btexture->wlog2 = 10;
    btexture->hlog2 = 9;

    btexture->format = TA_RGB565;

    thd_sleep(4*1000/60);

#if 0 && defined(USE_DMA) && !defined(TEXTURE_FLIP)
    ta_render_done_cb = cleartex_cb;

    /* wait for clear completion */
    sem_wait(dma_done);
#else
    memset(btexture->addr, 0, btexture->width*btexture->height*2);
#ifdef TEXTURE_FLIP
    memset(btexture2->addr, 0, btexture->width*btexture->height*2);
#endif
#endif

    shell_command("dl_set_trans(background.dl, mat_scale(640,480,1)) "
		  "dl_set_trans(background_dl, mat_trans(0,0,0.0001))");

    btexture->twiddled = 1;  /* Force twiddle */
    btexture->twiddlable = 1;  /* Force twiddle */

    btexture = NULL;

  }

#ifdef USE_DMA

#ifdef ONE_DMABUF
  if (dma_texbuf) {
    free(dma_texbuf);
    dma_texbuf = NULL;
  }
#else
  for (i=0; i<dma_picmax; i++) {
    free(dma_pics[i].buf);
    dma_pics[i].buf = NULL;
  }
#endif

#endif

  if (frame) {
    av_free(frame);
    frame = NULL;
  }

  oldpts = 0;
  realpts = 0;

  playa_force_prio = 0;

    
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
    lua_pushnumber(L, vid_fifo_limit + dma_picmax);
    return 1;
  } else
    return 0;
}

static int lua_fifo_used(lua_State * L)
{
  if (ready) {
    lua_pushnumber(L, vid_fifo_used() + pic_fifo_used());
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

static int lua_packet_size(lua_State * L)
{
  ff_packet_size = ((int)lua_tonumber(L, 1)) * 1024;
  if (ff_packet_size <= 0)
    ff_packet_size = 16*1024;

  printf("Packet buffer set to %dKb\n", ff_packet_size/1024);

  return 0;
}

static int lua_limit_fifo(lua_State * L)
{
  limit_fifo = ((int)lua_tonumber(L, 1)) * 1024;
  if (limit_fifo <= 0)
    limit_fifo = 0;

  printf("Sound fifo limit set to %dKb\n", limit_fifo/1024);

  return 0;
}

static int lua_vid_limit_fifo(lua_State * L)
{
  vid_fifo_limit = ((int)lua_tonumber(L, 1));
  if (vid_fifo_limit <= 5)
    vid_fifo_limit = 5;
  if (vid_fifo_limit > VID_FIFO_SZ-1)
    vid_fifo_limit = VID_FIFO_SZ-1;

  printf("Video fifo limit set to %d packets\n", vid_fifo_limit);

  return 0;
}

static int lua_ff_as(lua_State * L)
{
  adaptative_synchro = lua_tonumber(L, 1);

  if (adaptative_synchro)
    printf("adaptative_synchro ON\n");
  else
    printf("adaptative_synchro OFF\n");

  return 0;
}

static int lua_vtime(lua_State * L)
{
  lua_settop(L, 0);
  lua_pushnumber(L, ((int)(realpts/1000))/1000.0f);
  lua_pushnumber(L, timer_ms_gettime64()/1000.0f);
  return 2;
}

static int lua_getframe(lua_State * L)
{
  lua_settop(L, 0);
  lua_pushnumber(L, video_frame_counter);
  return 1;
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
    "ff_packet_size", "ff_ps", "ffmpeg",       /* long name, short name, topic */
    "ff_packet_size(kb) : "
    "set the size of the packet buffer in Kb \n", /* usage */
    SHELL_COMMAND_C, lua_packet_size           /* function */
  },

  {
    "ff_getframe", NULL, "ffmpeg",             /* long name, short name, topic */
    "ff_getframe() : "
    "return the video frame counter.\n",       /* usage */
    SHELL_COMMAND_C, lua_getframe              /* function */
  },

  {
    "ff_adaptative_synchro", "ff_as", "ffmpeg",/* long name, short name, topic */
    "ff_adaptative_synchro(value) : "
    "Set on or off adaptative audio/video synchro. "
    "Usually better to turn this off when watching "
    "a TV streaming.\n",                       /* usage */
    SHELL_COMMAND_C, lua_ff_as                 /* function */
  },

  {
    "ff_limit_fifo", "ff_lf", "ffmpeg",        /* long name, short name, topic */
    "ff_limit_fifo(kb) : "
    "Limit the sound fifo size. Try this when "
    "streaming from the internet if it sounds "
    "strange after a while. \n",               /* usage */
    SHELL_COMMAND_C, lua_limit_fifo            /* function */
  },

  {
    "ff_video_limit_fifo", "ff_vlf", "ffmpeg", /* long name, short name, topic */
    "ff_video_limit_fifo(kb) : "
    "Limit the video fifo size. ",             /* usage */
    SHELL_COMMAND_C, lua_vid_limit_fifo        /* function */
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
    "Player : Vincent Penne, "     /**< Driver authors */
    "ffmpeg lib : Fabrice Bellard "
    "and Bero for sh4 optimisations",
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
  info,
};

EXPORT_DRIVER(ffmpeg_driver)

#endif
