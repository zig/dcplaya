/**
 * @file    ogg_friver.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   dcplaya ogg input driver.
 *
 *  Based on sndoggvorbis.c from KallistiOS Ogg/Vorbis Decoder Library
 *
 *  (c)2001 Thorsten Titze
 *
 *  An Ogg/Vorbis player library using sndstream and functions provided by
 *  sndvorbisfile.
 *
 * $Id: ogg_driver.c,v 1.3 2002-09-23 03:25:01 benjihan Exp $
 */

#include <kos.h>

//#include <vorbis/codec.h>

//#include "ogg/config_types.h"
#include "ogg/os_types.h"
#include "sndvorbisfile.h"
#include "inp_driver.h"
#include "pcm_buffer.h"
#include "fifo.h"
#include "sysdebug.h"

/* PCM buffer: for storing data going out to the SPU */
static short * pcm_ptr;
static int pcm_count;
static int pcm_stereo;

//static int32 last_read=0; /* number of bytes the sndstream driver grabbed at last callback */

static int tempcounter =0;

/* liboggvorbis Related Variables */
VorbisFile_headers_t	v_headers;

/* Since we're only providing this library for single-file playback it should
 * make no difference if we define an info-storage here where the user can
 * get essential information about his file.
 *
 * the main() may access this directly but the user *should* use the getter
 * functions since the background code for these routines may change drastically
 * in the future !!!
 */
VorbisFile_info_t	sndoggvorbis_info;

#define BITRATE_MANUAL	-1

/* Library status related variables */
static volatile int sndoggvorbis_bitrateint;	/* bitrateinterval in calls */

/* getter and setter functions for information access
 */

void sndoggvorbis_setbitrateinterval(int interval)
{
  sndoggvorbis_bitrateint=interval;

  /* Just in case we're already counting above interval
   * reset the counter
   */
  tempcounter = 0;
}

/* sndoggvorbis_getbitrate()
 *
 * returns the actual bitrate of the playing stream.
 *
 * NOTE:
 * The value returned is only actualized every once in a while !
 */
long sndoggvorbis_getbitrate()
{
  return(sndoggvorbis_info.actualbitrate);
  // return(VorbisFile_getBitrateInstant());
}

long sndoggvorbis_getposition()
{
  return(sndoggvorbis_info.actualposition);
}

/* The following getters only return the contents of specific comment
 * fields. It is thinkable that these return something like "NOT SET"
 * in case the specified field has not been set !
 */
char *sndoggvorbis_getartist()
{
  return(sndoggvorbis_info.artist);
}
char *sndoggvorbis_gettitle()
{
  return(sndoggvorbis_info.title);
}
char *sndoggvorbis_getgenre()
{
  return(sndoggvorbis_info.genre);
}

char *sndoggvorbis_getcommentbyname(char *commentfield)
{
  return(VorbisFile_getCommentByName(commentfield));
}


void sndoggvorbis_clearcomments()
{
  sndoggvorbis_info.artist=VorbisFile_NULL;
  sndoggvorbis_info.title=VorbisFile_NULL;
  sndoggvorbis_info.album=VorbisFile_NULL;
  sndoggvorbis_info.tracknumber=VorbisFile_NULL;
  sndoggvorbis_info.organization=VorbisFile_NULL;
  sndoggvorbis_info.description=VorbisFile_NULL;
  sndoggvorbis_info.genre=VorbisFile_NULL;
  sndoggvorbis_info.date=VorbisFile_NULL;
  sndoggvorbis_info.location=VorbisFile_NULL;
  sndoggvorbis_info.copyright=VorbisFile_NULL;
  sndoggvorbis_info.isrc=VorbisFile_NULL;
  sndoggvorbis_info.filename=VorbisFile_NULL;

  sndoggvorbis_info.nominalbitrate=0;
  sndoggvorbis_info.actualbitrate=0;

  sndoggvorbis_info.actualposition=0;
}

/* main functions controlling the thread
 */


/* *callback(...)
 *
 * this procedure is called by the streaming server when it needs more PCM data
 * for internal buffering
 */
static int decode_frame(void)
{
  int pcm_decoded;
  pcm_ptr = pcm_buffer;

  if (VorbisFile_isEOS()==1) {
    SDDEBUG("%s() : Decode complete\n", __FUNCTION__);
    return INP_DECODE_END;
  }

  pcm_decoded = VorbisFile_decodePCM(v_headers, pcm_ptr, 4096);
  if (pcm_decoded < 0) {
    SDERROR("%s() : Decode failed\n", __FUNCTION__);
    return INP_DECODE_ERROR;
  }
  pcm_count = pcm_decoded;
  return 0;
}


static int sndogg_init(any_driver_t * d)
{
  SDDEBUG("%s([%s]) := [0]\n", __FUNCTION__, d->name);
  return 0;
}


/* Start playback (implies song load) */
static int sndogg_start(const char *fn, decoder_info_t *decoder_info)
{
  int err = -1;

  SDDEBUG("%s([%s])\n", fn);
  SDINDENT;

  sndoggvorbis_clearcomments();
  sndoggvorbis_setbitrateinterval(1000);
  if (VorbisFile_openFile(fn, &v_headers)) {
    SDERROR("Error could not open file\n");
    goto error;
  }

  /*
    v_headers->channels = vi.channels;
    v_headers->frequency = vi.rate;
    v_headers->bitrate = vi.bitrate_nominal;
    v_headers->vendor = vc.vendor;
  */

  decoder_info->bits      = 16;
  decoder_info->stereo    = v_headers.channels-1;
  decoder_info->frq       = v_headers.frequency; //$$$ decinfo.samprate;
  decoder_info->bps       = v_headers.bitrate*1000;
  decoder_info->bytes     = v_headers.bytes;
  decoder_info->time      = 0;

  if (decoder_info->bps > 0) {
    unsigned long long ms;
    ms = decoder_info->bytes;
    ms *= 8 * 1000;
    ms /= decoder_info->bps;
    decoder_info->time = ms;
  }

  {
    static char version[128], *s;
    strcpy(version, "ogg");
    s = VorbisFile_getCommentByName("version");
    if (s && strcmp(s,"N/A")) {
      strcat(version, " ");
      strcat(version, s);
    }
    strcat(version, decoder_info->stereo ? " stereo" : " mono");
    decoder_info->desc    = version;
  }
  pcm_ptr = pcm_buffer;
  pcm_count = 0;
  pcm_stereo = decoder_info->stereo;
  err = 0;

 error:
  SDUNINDENT;
  SDDEBUG("%s() := [%d]\n", __FUNCTION__, err);
  return err;
}

/* sndoggvorbis_stop()
 *
 * function to stop the current playback and set the thread back to
 * STATUS_READY mode.
 */
static int sndogg_stop(void)
{
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  VorbisFile_closeFile();
  SDDEBUG("Vorbis file closed\n");
  sndoggvorbis_clearcomments();
  SDDEBUG("Comments cleared\n");

  SDUNINDENT;
  SDDEBUG("<< %s()\n", __FUNCTION__);
  return 0;
}

/* Shutdown the player */
static int sndogg_shutdown(any_driver_t * d)
{
  SDDEBUG("%s(%s) := [0]\n", __FUNCTION__, d->name);
  return 0;
}

static int sndogg_decoder(decoder_info_t * info)
{
  int n = 0;

  /* Preliminary Bitrate Code
   * For our tests the bitrate is being set in the struct once in a
   * while so that the user can just read out this value from the struct
   * and use it for whatever purpose
   */
  if (tempcounter == sndoggvorbis_bitrateint) {
    long test;
    if ((test = VorbisFile_getBitrateInstant()) != -1) {
      sndoggvorbis_info.actualbitrate = test;
    }
    tempcounter = 0;
  }
  tempcounter++;

  /* No more pcm : decode next mp3 frame
   * No more frame : it is the end
   */
  if (!pcm_count) {
    int status;

    status = decode_frame();
    /* End or Error */
    if (status & INP_DECODE_END) {
      return status;
    }
  }

  if (pcm_count > 0) {
    /* If we have some pcm , send them to fifo */
    if (pcm_stereo) {
      n = fifo_write((int *) pcm_ptr, pcm_count);
    } else {
      n = fifo_write_mono(pcm_ptr, pcm_count);
    }

    if (n > 0) {
      pcm_ptr += (n << pcm_stereo);
      pcm_count -= n;
    } else if (n < 0) {
      return INP_DECODE_ERROR;
    }
  }
  return -(n>0) & INP_DECODE_CONT;
}

static char *StrDup(const char *s)
{
  if (!s) return 0;
  return strdup(s);
}

static int sndogg_info(playa_info_t *info, const char *fname)
{
  info->artist   = StrDup(VorbisFile_getCommentByName("artist"));
  info->title    = StrDup(VorbisFile_getCommentByName("title"));
  info->album    = StrDup(VorbisFile_getCommentByName("album"));
  info->track    = StrDup(VorbisFile_getCommentByName("tracknumber"));
  info->comments = StrDup(VorbisFile_getCommentByName("description"));
  info->genre    = StrDup(VorbisFile_getCommentByName("genre"));
  info->year     = StrDup(VorbisFile_getCommentByName("date"));

  return 0;
}

static driver_option_t * sndogg_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

static inp_driver_t ogg_driver = {
  /* Any driver */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h)  */
    INP_DRIVER,           /* Driver type */
    0x100,                /* Version */
    "ogg-vorbis-decoder", /* Driver name */
    "Benjamin Gerard\0"   /* Authors */
    "Dan Potter\0"
    "and vorbis team\0",
    "ogg vorbis audio "   /* Description */
    "decoder",
    0,                    /* Dll */
    sndogg_init,          /* Init */
    sndogg_shutdown,      /* Shutdown */
    sndogg_options        /* Options */
  },

  /* Input driver specific */
  0,                      /* User Id */
  ".ogg\0",               /* Extension list */

  sndogg_start,
  sndogg_stop,
  sndogg_decoder,
  sndogg_info,
};

EXPORT_DRIVER(ogg_driver)
