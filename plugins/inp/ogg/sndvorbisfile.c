/* KallistiOS Ogg/Vorbis Decoder Library
 *
 * ilboggvorbis.c
 * (c)2001 Thorsten Titze
 *
 * Basic Ogg/Vorbis stream information and decoding routines used by
 * libsndoggvorbis. May also be used directly for playback without
 * threading.
 */

static char id[] =
    "sndoggvorbis $Id: sndvorbisfile.c,v 1.1 2002-08-26 14:14:35 ben Exp $";
#include <kos.h>
#include <stdio.h>
#include <vorbis/codec.h>
#include "sndvorbisfile.h"

VorbisFile_handle_t fd;

// Ogg/Vorbis relevant variables
ogg_sync_state oy;
ogg_page og;
ogg_packet op;
ogg_stream_state os;
vorbis_info vi;
vorbis_comment vc;
vorbis_dsp_state vd;
vorbis_block vb;

int VorbisFile_convbufferlength = 4096;
int VorbisFile_EOS;

/* Variables for calculation of actual bitrate */
double VorbisFile_sampletrack;
double VorbisFile_bittrack;

char *VorbisFile_streambuffer;
int bytes;
uint32 fw;

/* Defines for various VorbisFile status */
#define VorbisFile_EOSreached 1
#define VorbisFile_notplaying -1
#define VorbisFile_playing  1

VorbisFile_handle_t fd;

/* VorbisFile_isEOS()
 *
 * detects whether or not we already reached the end of our Ogg/Vorbis stream
 *
 * Returns:
 * 1  = Reached End Of Stream
 * 0  = Well.. I guess it's clear.. We did not yet reach the End Of Stream
 */
int VorbisFile_isEOS()
{
  if (VorbisFile_EOS == 1)
    return (1);
  else
    return (0);
}

/* VorbisFile_getBitrateNominal()
 *
 * returns the nominal bitrate of the already loaded bitstream.
 *
 * Returns:
 * LONG = Nominal bitrate of the loaded Strea
 * -1   = Clear also: No stream loaded yet.
 */
long VorbisFile_getBitrateNominal()
{
  if (fd == 0)
    return (-1);
  else
    return (vi.bitrate_nominal);
}

/* VorbisFile_getBitrateInstant()
 *
 * returns the actual bitrate of the file since the last call
 *
 * Returns:
 * LONG = Bitrate
 * -1   = No stream loaded.
 */
long VorbisFile_getBitrateInstant()
{
  long ret;

  if (fd == 0)
    return (-1);
  if (VorbisFile_sampletrack == 0)
    return (-1);                /* prevent division by zero */

  /* Calcualte actual bitrate */
  ret = VorbisFile_bittrack / VorbisFile_sampletrack * vi.rate + .5;

  /* reset tracking variables for next call */
  VorbisFile_bittrack = 0.0f;
  VorbisFile_sampletrack = 0.0f;

  return (ret);
}

/* VorbisFile_getCommentByName(...)
 *
 * returns the comment value of a specified commentfield
 *
 * Returns:
 * Success = comment content
 * NULL = field not found
 */
char *VorbisFile_getCommentByName(char *commentfield)
{
  int i;
  int commentlength = strlen(commentfield);

  if (fd == 0)
    return (VorbisFile_NULL);

  //return(vorbis_comment_query(&vc, commentfield,1));
  for (i = 0; i < vc.comments; i++) {
    if (!(strncmp(commentfield, vc.user_comments[i], commentlength))) {
      /* Return adress of comment content but leave out the
       * commentname= part !
       */
      return (vc.user_comments[i] + commentlength + 1);
    }
  }
  return (VorbisFile_NULL);
}

/* VorbisFile_getCommentByID(...)
 *
 * returns the comment identified by id
 *
 * Returns:
 * Success = comment content
 * NULL = field not found
 */
char *VorbisFile_getCommentByID(long commentid)
{
  if (fd == 0)
    return (NULL);

  /* Check if we have at least that much comments in our file ?! */
  if (vc.comments > 0) {
    return vc.user_comments[commentid];
  }
  return (VorbisFile_NULL);
}

/* VorbisFile_closeFile()
 *
 * Closes the Filehandle used for accessing the OggVorbis-Stream
 */
void VorbisFile_closeFile()
{
  if (fd != 0) {
    fs_close(fd);
    fd = 0;
  }
}

/* int VorbisFile_openFile(...)
 *
 * returns:
 *  0 = File open and header read succesful
 * -1 = File not opened successfully or no valid OggVorbis stream
 */
int VorbisFile_openFile(char *filename, VorbisFile_headers_t * v_headers)
{
  int i;
  VorbisFile_EOS = 0;

  dbglog(DBG_DEBUG,
         "** " __FUNCTION__
         ": liboggvorbis: trying to open file \"%s\"\r\n", filename);

  fd = fs_open(filename, O_RDONLY);

  if (fd == 0) {
    dbglog(DBG_DEBUG,
           "** " __FUNCTION__
           ": liboggvorbis: cannot open file \"%s\"\r\n", filename);
    return (-1);
  }
  v_headers->bytes = fs_total(fd);

  ogg_sync_init(&oy);

  VorbisFile_streambuffer = ogg_sync_buffer(&oy, 4096);
  bytes = fs_read(fd, VorbisFile_streambuffer, 4096);

  ogg_sync_wrote(&oy, bytes);

  if (ogg_sync_pageout(&oy, &og) != 1) {
    if (bytes < 4096)
      return (-1);
  }
  dbglog(DBG_DEBUG,
         "** " __FUNCTION__
         ": libogg: input bitstream has been detected to be Ogg compliant\n");

  ogg_stream_init(&os, ogg_page_serialno(&og));
  dbglog(DBG_DEBUG,
         "** " __FUNCTION__ ": libogg: ogg input bitstream initialized\n");

  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  dbglog(DBG_DEBUG,
         "** " __FUNCTION__ ": libvorbis: info and comment initialized\n");

  if (ogg_stream_pagein(&os, &og) < 0) {
    dbglog(DBG_DEBUG,
           "** " __FUNCTION__
           ": libogg: error reading first page of Ogg bitstream\n");
  }

  if (ogg_stream_packetout(&os, &op) != 1) {
    dbglog(DBG_DEBUG,
           "** " __FUNCTION__
           ": libogg: error reading initial header packet\n");
  }

  if (vorbis_synthesis_headerin(&vi, &vc, &op) < 0) {
    dbglog(DBG_DEBUG,
           "** " __FUNCTION__
           ": libvorbis: Ogg bitstream does not contain Vorbis encoded date\n");
  }


  i = 0;
  while (i < 2) {
    while (i < 2) {
      int result = ogg_sync_pageout(&oy, &og);
      if (result == 0)
        break;
      if (result == 1) {
        ogg_stream_pagein(&os, &og);
        while (i < 2) {
          result = ogg_stream_packetout(&os, &op);
          if (result == 0)
            break;
          if (result < 0) {
            dbglog(DBG_DEBUG,
                   "** " __FUNCTION__
                   ": libogg: corrupt secondary header in bitstream\n");
          }
          vorbis_synthesis_headerin(&vi, &vc, &op);
          i++;
        }
      }
    }
    VorbisFile_streambuffer = ogg_sync_buffer(&oy, 4096);
    bytes = fs_read(fd, VorbisFile_streambuffer, 4096);
    if (bytes == 0 && i < 2) {
      dbglog(DBG_DEBUG,
             "** " __FUNCTION__
             ": libogg: EOF occured before all headers have been found\n");
    }
    ogg_sync_wrote(&oy, bytes);
  }

  dbglog(DBG_DEBUG,
         "** " __FUNCTION__
         ": libvorbis: Bitstream is %d channel, %ld Hz, %ld bit/s\r\n",
         vi.channels, vi.rate, vi.bitrate_nominal);
  dbglog(DBG_DEBUG, "** " __FUNCTION__ ": libvorbis: Encoded by: %s\r\n",
         vc.vendor);
  dbglog(DBG_DEBUG,
         "** " __FUNCTION__
         ": libvorbis: Found %d additional comment fields\r\n",
         vc.comments);

  /* Todo:
   * Reading of additional comment fields and putting in v_headers
   */

  v_headers->channels = vi.channels;
  v_headers->frequency = vi.rate;
  v_headers->bitrate = vi.bitrate_nominal;
  v_headers->vendor = vc.vendor;
  v_headers->convsize = VorbisFile_convbufferlength / vi.channels;

  /* Initialize bitrate tracking variables */
  VorbisFile_bittrack = 0;
  VorbisFile_sampletrack = 0;

  // Initialize the decoder
  vorbis_synthesis_init(&vd, &vi);
  vorbis_block_init(&vd, &vb);

  return (0);
}

/* int VorbisFile_decodePCM(...)
 *
 * dependent on the target-buffer size we have to change to pointer arithmetic to
 * reflect other datatypes. this example as it is coded here reflects an int16
 * data-type for PCM buffer storage.
 *
 * returns:
 * >0 = number of bytes decoded
 * -1 = we have reached the end of the Ogg stream
 */
// Subroutine that grabs stream data from a file
//
void iVorbisFile_grabData()
{
  VorbisFile_streambuffer = ogg_sync_buffer(&oy, 4096);
  bytes = fs_read(fd, VorbisFile_streambuffer, 4096);
  ogg_sync_wrote(&oy, bytes);
}


static void conv_mono(ogg_int16_t *d, const float *s, int n)
{
  if (n<=0) {
    return;
  }

  do {
    int v = (int)(*s++ * 32767.0f);

    v += 32768;               /* change sign */
    v &= ~(v>>31);            /* Lower clip  */
    v |= (65535 - v) >> 31;   /* Upper clip  */ 
    *d++ = v ^ 0x8000;
  } while (--n);

}

static void conv_stereo(ogg_int32_t *d, const float *l, const float *r, int n)
{
  if (n<=0) {
    return;
  }

  do {
    int v,w;

    v = (int)(*l++ * 32767.0f);
    v += 32768;               /* change sign */
    v &= ~(v>>31);            /* Lower clip  */
    v |= (65535 - v) >> 31;   /* Upper clip  */ 
    v &= 0xFFFF;

    w = (int)(*r++ * 32767.0f);
    w += 32768;               /* change sign */
    w &= ~(w>>31);            /* Lower clip  */
    w |= (65535 - w) >> 31;   /* Upper clip  */ 

    *d++ = (v | (w<<16)) ^ 0x80008000;
  } while (--n);
}

/* VorbisFile_decodePCM(...)
 *
 * same as VorbisFile_decodePCMint8 but decodes into a 16-bit Integer buffer not in
 * a 8 bit byte buffer
 */
int VorbisFile_decodePCM(VorbisFile_headers_t vhd,
			   ogg_int16_t * target,
			   int req_pcm)
{
  int convsize = vhd.convsize;
  int n = req_pcm;

  if (n <= 0) {
    return n;
  }

  do {
    float **pcm;
    int samples;

    // first we have to try to get some data out of the current stream

    while (n > 0 && (samples = vorbis_synthesis_pcmout(&vd, &pcm)) > 0) {
      // int clipflag=0;

      // the "bout" variable holds the number of PCM samples we got. but this
      // only counts how many stereo-16bit samples we have.
      //
      // so to get the number of 16-bit samples we have to multiply by 2
      // and to get the number of bytes at all we have to multiply by 4
      int bout = (samples < convsize ? samples : convsize);

      // pcm contains both (left and right) decoded pcm values
      switch(vi.channels) {
      case 1:
	conv_mono(target, pcm[0], bout);
	break;
      case 2:
	conv_stereo(target, pcm[0], pcm[1], bout);
	break;
      default:
	dbglog(DBG_DEBUG,"!! " __FUNCTION__
	       " : bad number of channel : %d\n", vi.channels);
	return -1;
      }
      target += bout << (vi.channels-1);
      n -= bout;

      vorbis_synthesis_read(&vd, bout);
    }
    
    if (n > 0) {
      while (ogg_stream_packetout(&os, &op) == 0) {
        while (ogg_sync_pageout(&oy, &og) == 0) {
          iVorbisFile_grabData();
        }
        if (ogg_page_eos(&og)) {
          VorbisFile_EOS = 1;
          dbglog(DBG_DEBUG,
                 "** " __FUNCTION__
                 ": liboggvorbis: reached the end of the stream\r\n");
	  break;
        }
        ogg_stream_pagein(&os, &og);
      }
      if (vorbis_synthesis(&vb, &op) == 0) {
        vorbis_synthesis_blockin(&vd, &vb);
      }
    }
  } while (n > 0);
  if (req_pcm - n < 0) {
    dbglog(DBG_DEBUG, "%d ", req_pcm - n);
  }
  return req_pcm - n;
}

