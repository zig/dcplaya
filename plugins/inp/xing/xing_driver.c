/* Tryptonite

   sndmp3.c
   (c)2000 Dan Potter

   An MP3 player using sndstream and XingMP3
*/

static char id[] = "TRYP $Id: xing_driver.c,v 1.2 2002-09-04 18:54:11 ben Exp $";


/* This library is designed to be called from another program in a thread. It
   expects an input filename, and it will do all the setup and playback work.
  
   This requires a working math library for m4-single-only (such as newlib).
   
 */

/*

  xing-mp3 driver - DreamMp3 version by ben

*/

#define VCOLOR(R,G,B) //vid_border_color(R,G,B)

#include <kos.h>

//#include "sndstream.h"
//#include "sndmp3.h"
#include "inp_driver.h"
#include "pcm_buffer.h"
#include "fifo.h"
#include "id3.h"

/************************************************************************/
#include "mhead.h"		/* From xingmp3 */
#include "port.h"		  /* From xingmp3 */

/* Conversion codes: generally you won't play with this stuff */
#define CONV_NORMAL	    0
#define CONV_MIXMONO	  1
#define CONV_DROPRIGHT	2
#define CONV_DROPLEFT	  3
#define CONV_8BIT	      8 

/* Reduction codes: once again, generally won't play with these */
#define REDUCT_NORMAL	  0
#define REDUCT_HALF	    1
#define REDUCT_QUARTER	2

/* Bitstream buffer: this is for input data */
static char *bs_ptr;
static int bs_count;

/* PCM buffer: for storing data going out to the SPU */
static short * pcm_ptr;
static int pcm_count;
static int pcm_stereo;

/* MPEG file */
static uint32     mp3_fd;
static int		    frame_bytes;
static MPEG		    mpeg;
static MPEG_HEAD	head;
static int		    bitrate;
static DEC_INFO		decinfo;

/* Checks to make sure we have some data available in the bitstream
   buffer; if there's less than a certain "water level", shift the
   data back and bring in some more. */
static int bs_fill()
{
  int n = -1;

  /* Make sure we don't underflow */
  if (bs_count < 0) bs_count = 0;
  
  /* Pull in some more data if we need it */
  if (bs_count < frame_bytes) {
    /* Shift everything back */
    memmove(bs_buffer, bs_ptr, bs_count);
    
    if (mp3_fd) {
      n = fs_read(mp3_fd, bs_buffer+bs_count, BS_SIZE - bs_count);
    }
		
    if (n <= 0) {
      return -1;
    }

    /* Shift pointers back */
    bs_count += n; bs_ptr = bs_buffer;
  }

  return 0;
}


static int decode_frame()
{
  IN_OUT	x;
	
  pcm_ptr = pcm_buffer;

  /* Is there enought bytes in mp3-buffer */
  if (bs_count >= frame_bytes) {
    int pcm_align_mask = (1<<(pcm_stereo+1)) - 1;
  
    /* Decode a frame */
    x = audio_decode(&mpeg, bs_ptr, (short *) pcm_buffer);
    if (x.in_bytes <= 0) {
      dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Bad sync in MPEG file\n");
      return INP_DECODE_ERROR;
    }
    bs_ptr      += x.in_bytes;
    bs_count    -= x.in_bytes;

    /* Check output ... */	  
    if (x.out_bytes & pcm_align_mask) {
      dbglog(DBG_ERROR, "** Bad number of output bytes."
	     "%d is not a multiple of %d\n",
	     x.out_bytes, pcm_align_mask+1);
      x.out_bytes &= ~pcm_align_mask;
    }
    pcm_count   = x.out_bytes >> (1+pcm_stereo);
	  
  } else {
    /* Pull in some more data (and check for EOF) */
    if (bs_fill() < 0 || bs_count < frame_bytes) {
      dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Decode complete\n");
      return INP_DECODE_END;
    }
  }
  return 0;
}



/* Open an MPEG stream and prepare for decode */
static int xing_init(const char *fn, decoder_info_t * decoder_info) {
  uint32	fd;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__"('%s')\r\n", fn);

  /* Open the file */
  mp3_fd = fd = fs_open(fn, O_RDONLY);
  if (fd == 0) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__ 
	   " : Can't open input file %s\r\n", fn);
    dbglog(DBG_DEBUG, "** " __FUNCTION__ 
	   " : getwd() returns '%s'\r\n", fs_getwd());
    return -1;
  }
  decoder_info->bytes = fs_total(fd);
  if (decoder_info->bytes<0) {
    decoder_info->bytes = 0;
  }
	
  /* Allocate buffers */
  bs_ptr = bs_buffer; bs_count = 0;
  pcm_ptr = pcm_buffer; pcm_count = 0;
	
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : pcm[%p %d] bs[%p %d]\n",
    pcm_buffer, pcm_buffer_size,
    bs_buffer, bs_buffer_size);
	
  /* Fill bitstream buffer */
  frame_bytes = 1; /* pipo frame bytes must be > 0 */
  if (bs_fill() < 0) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Can't read file header\r\n");
    goto errorout;
  }
  frame_bytes = 0;

  /* Are we looking at a RIFF file? (stupid Windows encoders) */
  if (bs_ptr[0] == 'R' && bs_ptr[1] == 'I' && bs_ptr[2] == 'F' && bs_ptr[3] == 'F') {
    /* Found a RIFF header, scan through it until we find the data section */
    dbglog(DBG_DEBUG, "** " __FUNCTION__ "Skipping stupid RIFF header\r\n");
    while (bs_ptr[0] != 'd' || bs_ptr[1] != 'a' || bs_ptr[2] != 't'	|| bs_ptr[3] != 'a') {
      bs_ptr++;
      if (bs_ptr >= (bs_buffer + BS_SIZE)) {
	dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Indeterminately long RIFF header\r\n");
	goto errorout;
      }
    }

    /* Skip 'data' and length */
    bs_ptr += 8;
    bs_count -= (bs_ptr - bs_buffer);
    dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Final index is %d\r\n", (bs_ptr - bs_buffer));
  }

  if (((uint8)bs_ptr[0] != 0xff) && (!((uint8)bs_ptr[1] & 0xe0))) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Definitely not an MPEG file\r\n");
    goto errorout;
  }

  /* Initialize MPEG engines */
  mpeg_init(&mpeg);
  mpeg_eq_init(&mpeg);

  /* Parse MPEG header */
  {
    int forward;
    frame_bytes = head_info3(bs_ptr, bs_count, &head, &bitrate,&forward);
    dbglog(DBG_DEBUG, "** " __FUNCTION__ 
	   " : head_info [frame_bytes=%d, forward=%d]\r\n",
	   frame_bytes, forward);
    bs_ptr += forward;
    bs_count -= forward;
  }
  if (frame_bytes == 0) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__
	   " : Bad or unsupported MPEG file\r\n");
    goto errorout;
  }
  decoder_info->bps = bitrate;

  /* Print out some info about it */
  {
    static char *layers[] = { "invalid", "layer 3", "layer 2", "layer 1" };
    static char *modes[] = { " stereo", " joint-stereo", " dual", " mono" };
    static char desc[32];
    decoder_info->desc = desc;
    strcpy(desc, layers[head.option]);
    strcat(desc, modes[head.mode]);
  }

  /* Initialize audio decoder */
  /* $$$ Last parameters looks like cut frequency :
     Dim said to me 40000 is a good value */ 
  if (!audio_decode_init(&mpeg, &head, frame_bytes,
			 REDUCT_NORMAL, 0, CONV_NORMAL, 40000)) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__ 
	   " : Failed to initialize decoder\r\n");
    goto errorout;
  }
  audio_decode_info(&mpeg, &decinfo);
	
  /* Copy decoder PCM info */ 
  decoder_info->frq    = decinfo.samprate;
  decoder_info->bits   = decinfo.bits;
  decoder_info->stereo = decinfo.channels-1;

  if (decoder_info->bps > 0) {
    unsigned long long ms;
    ms = decoder_info->bytes;
    ms *= 8 * 1000;
    ms /= decoder_info->bps;
    decoder_info->time = ms;
  }

  pcm_ptr    = pcm_buffer;
  pcm_count  = 0;
  pcm_stereo = decoder_info->stereo;

  dbglog(DBG_DEBUG, ">> Desc            = 	%s\n", decoder_info->desc);
  dbglog(DBG_DEBUG, ">> Bits            = 	%u\n", 8<<decoder_info->bits);
  dbglog(DBG_DEBUG, ">> Channels        = 	%u\n", 1+decoder_info->stereo);
  dbglog(DBG_DEBUG, ">> Sampling        = 	%uhz\n",decoder_info->frq);
  dbglog(DBG_DEBUG, ">> Bitrate         = 	%ubps\n", decoder_info->bps);
  dbglog(DBG_DEBUG, ">> Bytes           = 	%u\n", decoder_info->bytes);
  dbglog(DBG_DEBUG, ">> Time (ms)       = 	%u\n", decoder_info->time);

  if (decinfo.channels < 1 || decinfo.channels > 2 || decinfo.bits != 16) {
    dbglog(DBG_ERROR, ">> Unsupported audio outout format\n");
    goto errorout;
  }

  dbglog(DBG_DEBUG, "<< " __FUNCTION__"('%s') := 0\r\n", fn);
  return 0;

 errorout:
  dbglog(DBG_DEBUG, "<< " __FUNCTION__"('%s') := -1\r\n", fn);
  if (fd) fs_close(fd);
  frame_bytes = 0;
  mp3_fd = 0;
  return -1;
}

static void xing_shutdown() {
  dbglog(DBG_DEBUG, "** " __FUNCTION__"\r\n");
  if (mp3_fd) {
    fs_close(mp3_fd);
    mp3_fd = 0;
  }
}


/************************************************************************/

/*$$$static*/int sndmp3_init(void)
{
  dbglog(DBG_DEBUG, "** " __FUNCTION__"\n");
  return 0;
}

/* Start playback (implies song load) */
/*static */int sndmp3_start(const char *fn, decoder_info_t *info)
{
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " ('%s')\n", fn);
  
  /* Initialize MP3 engine */
  if (xing_init(fn, info) < 0)
    return -1;
  return 0;
}

/* Stop playback (implies song unload) */
static int sndmp3_stop(void)
{
  dbglog(DBG_DEBUG, "** " __FUNCTION__"\n");
  xing_shutdown();
  return 0;
}

/* Shutdown the player */
static int sndmp3_shutdown(void)
{
  dbglog(DBG_DEBUG, "** " __FUNCTION__"\n");
  return 0;
}

static int sndmp3_decoder()
{
  int n = 0;
	
  if (!mp3_fd) {
    return INP_DECODE_ERROR;
  }
  
  /* No more pcm : decode next mp3 frame
   * No more frame : it is the end
   */
  if (!pcm_count) {
    int status = decode_frame();
    /* End or Error */
    if (status & INP_DECODE_END) {
      return status;
    }
  }

  if (pcm_count > 0) {
    /* If we have some pcm , send them to fifo */
    if (pcm_stereo) {
      n = fifo_write((int *)pcm_ptr, pcm_count);
    } else {
      n = fifo_write_mono(pcm_ptr, pcm_count);
    }
    
    if (n > 0) {
      pcm_ptr   += (n<<pcm_stereo);
      pcm_count -= n;
    } else if (n < 0) {
      return INP_DECODE_ERROR;
    }
  }
  return -(n > 0) & INP_DECODE_CONT;
}

static int sndmp3_info(playa_info_t *info, const char *fname)
{
  return id3_info(info, fname);
}

static driver_option_t * sndmp3_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

/*static*/ inp_driver_t xing_driver =
{

  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    INP_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "Xing-mp3-decoder",   /* Driver name */
    "Benjamin Gerard\0"   /* Driver authors */
    "Dan Potter\0",
    "Xing Technology "
    "MPEG I layer "
    "I,II,III decoder",  /**< Description */
    0,                   /**< DLL handler */
    sndmp3_init,         /**< Driver init */
    sndmp3_shutdown,     /**< Driver shutdown */
    sndmp3_options,      /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /* User Id */
  ".mp3\0.mp2\0",         /* EXtension list */

  sndmp3_start,
  sndmp3_stop,
  sndmp3_decoder,
  sndmp3_info,
};

EXPORT_DRIVER(xing_driver)

