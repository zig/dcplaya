
#include <stdlib.h>

#include "sha123/api.h"
#include "sha123/tables.h"
#include "sha123/sha123.h"
#include "sha123/alloc.h"
#include "sha123/debug.h"
#include "sha123/decode.h"
#include "sha123/dct.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int sync_header(sha123_t * sha123);

static int skip_frame_bytes(sha123_t * sha123);
static int read_III_sideinfo(sha123_t * sha123);
static int read_frame_bytes(sha123_t * sha123);

/* static int read_more_bytes(sha123_t * sha123); */
static int reach_eof(sha123_t * sha123);

/** Initialize sha123 library. */
int sha123_init(void)
{
  const int out_scale = 32000;

  sha123_debug("sha123_init()\n");

  if (sha123_make_decode_tables(out_scale)) {
    goto error;
  }

  if (sha123_dct_init(DCT_MODE_FASTEST, out_scale)) {
    goto error;
  }

  return 0;

 error:
  sha123_shutdown();
  return -1;
}

/** Shutdown sha123 library. */
void sha123_shutdown(void)
{
  sha123_debug("sha123_shutdown()\n");
  sha123_dct_shutdown();
  sha123_debug("  -> shutdowned\n");
}

const sha123_info_t * sha123_info(sha123_t * sha123)
{
  return (sha123 && sha123->xinfo.layer) ? &sha123->xinfo : 0;
}

/** Reset current layer type. Must be call when layer type changes. */
static int reset_layer(sha123_t * sha123)
{
  int err = -1;

  sha123_header_dump(sha123->frame.header);
  sha123_header_info_dump(&sha123->frame.info);

  switch ( sha123->frame.info.layer ) {
  case 1:
    sha123_set_error(sha123, "Layer I, not supported");
    break;
  case 2:
    sha123_set_error(sha123, "Layer II, not supported");
    break;
  case 3:
    sha123_init_layer3();
    err = 0;
    break;
  default:
    sha123_debug("!!! INTERNAL : invalid layer (%d)\n",
		 sha123->frame.info.layer);
    sha123_set_error(sha123, "INTERNAL: Invalid layer type");
  }

  return err;
}


/** Start mpeg decoder. */
sha123_t * sha123_start(sha123_param_t * param)
{
  sha123_t * sha123 = 0;
  int size;

  sha123_debug("sha123_start [%p]\n", param);
  if (!param) {
    goto error;
  }

  sha123_debug("sha123_start [%s] [%s(%d)]\n",
	       istream_filename(param->istream),
	       param->loop ? "loop" : "infinite",
	       param->loop);

  /* Open stream */
  if (istream_open(param->istream) == -1) {
    goto error;
  }

  /* Compute sha123 struct size, with supplemental info. */
  size = sizeof(*sha123);

  /* Alloc and clean sha123 struct. */
  sha123 = sha123_alloc(size);
  if (!sha123) {
    goto error;
  }
  memset(sha123, 0, size);

  /* Setup sha123 struct */ 
  sha123->istream = param->istream;
  sha123->loop.count = param->loop;
  sha123->limits.frq = -1;
  sha123->frame.synth_mn = sha123_synth_1to1_mono;
  sha123->frame.synth_st = sha123_synth_1to1;

  /* Read fisrt header. */
  if (sync_header(sha123)) {
    goto error;
  }
  sha123_debug("-> Started\n");

  /* Set loop position */
  size = istream_tell(sha123->istream);
  if (size != -1) {
    sha123->loop.pos = size /*- sha123->bsi.count*/ - 4;
    sha123_debug("Set loop position %u\n", sha123->loop.pos);
  }

  return sha123;

 error:
  sha123_debug("-> Start failed : [%s]\n", sha123_get_errstr(sha123));
  sha123_stop(sha123);
  return 0;
}

/** Stop sha123 decoder. */
void sha123_stop(sha123_t * sha123)
{
  sha123_debug("sha123_stop [%s]\n",
	       sha123 ? istream_filename(sha123->istream) : 0);

  if (sha123) {
    istream_close(sha123->istream);
    sha123_free(sha123);
  }
  sha123_debug("-> Stopped\n");
}

/** Decode audio data. */
int sha123_decode(sha123_t * sha123, void * buffer, int n)
{
  int code;

  if (!sha123) {
    return -1;
  }
  if (!sha123->istream) {
    sha123_set_error(sha123,"no input istream");
    return -1;
  }

  if (n) {
    do {
      code = sync_header(sha123);
      if (code == 1) {
	code = reach_eof(sha123);
      }
    } while (code == 1);
  }

  if (!sha123_header_check(sha123->frame.header)) {
/*     sha123_header_dump(sha123->frame.header); */
/*     sha123_header_info_dump(&sha123->frame.info); */


/*     code = skip_frame_bytes(sha123); */
    code = read_III_sideinfo(sha123);
    if (!code) {
      /* Copy previous frame data */
      const int max  = sizeof(sha123->frame.buffer.prev_data);
      const int size = sha123->frame.buffer.size;
      int bytes, missing;

      bytes = size;
      missing = max - bytes;
      if (missing <= 0) {
	bytes = max;
      } else {
	memcpy(sha123->frame.buffer.prev_data,
	       sha123->frame.buffer.prev_data+max-missing,
	       missing);
      }
      memcpy(sha123->frame.buffer.data - bytes,
	     sha123->frame.buffer.data + size - bytes,
	     bytes);

      code = read_frame_bytes(sha123);
      if (!code) {
	//static unsigned char buffer[64000];
	sha123->pcm_sample = buffer;
	sha123->pcm_point = 0;
	if (sha123_do_layer3(sha123) == -1) {
	  sha123_debug("ERROR\n");
	  //	  return -1;
	}
	sha123_debug("sha123->pcm_point:%d\n",sha123->pcm_point);

	/* VP : !!!!!! */
/* 	fwrite(buffer, 1, sha123->pcm_point, */
/* 	       stdout); */
      }
    }
  }

  return code;
}

const char * sha123_set_error(sha123_t *sha123, const char * errstr)
{
  if (sha123) {
    sha123->errstr = errstr;
  }
  return errstr;
}

const char * sha123_get_errstr(sha123_t * sha123)
{
  return sha123 ? sha123->errstr : "<null> pointer";
}

/*
 * @retval -1 error
 * @retval  0 valid header found
 * @retval  1 eof
 */
static int sync_header(sha123_t * sha123)
{
  const unsigned int id3  = ('I' << 24) | ('D' << 16) | ('3' << 8);
  const unsigned int riff = ('R' << 24) | ('I' << 16) | ('F' << 8) | 'F';

  unsigned int hd;
  sha123_header_t header;
  int code;
  int resync = 0;
  unsigned char b[4];

  /* Copy previous frame info. */
  sha123->prev_info = sha123->frame.info;

  code = istream_read(sha123->istream, b, 3);
  if (code != 3) {
    return code == -1 ? -1 : 1;
  }

  hd = (b[0] << 16) | (b[1] <<  8) | b[2];
  do {
    code = istream_read(sha123->istream, b, 1);
    if (code != 1) {
      return code == -1 ? -1 : 1;
    }

    hd = ((hd << 8) | *b) & 0xFFFFFFFF;

    if (hd == riff) {
      sha123_debug("Found RIFF header\n");
    } else if ((hd & 0xFFFFFF00) == id3) {
      sha123_debug("Found ID3 header\n");
    }
    sha123_header_set(&header,hd);
    code = sha123_header_check(header);
    if (!code) {
      code = sha123_decode_header(&sha123->frame.info, header);
    }
    resync += !!code;
  } while (code);

  /* At this point, we get a new valid frame header. */

  if (resync) {
    sha123_debug("Resync after %u bytes\n", resync);
  }

  /* Save frame header. */
  sha123->frame.header = header;

  /* Copy frame info to external struct. */
  sha123->xinfo.layer = sha123->frame.info.layer;
  sha123->xinfo.channels = 1 << sha123->frame.info.log2chan;
  sha123->xinfo.sampling_rate = sha123->frame.info.sampling_rate;

  /* Check format change. */
  if (sha123->frame.info.layer != sha123->prev_info.layer) {
      sha123_debug("On the fly: layer change %d->%d\n",
		   sha123->prev_info.layer, sha123->frame.info.layer);
      if (reset_layer(sha123)) {
	return -1;
      }
  } else if (sha123->frame.info.sampling_rate
	     != sha123->prev_info.sampling_rate
	     || sha123->frame.info.log2chan
	     != sha123->prev_info.log2chan) {
    sha123_debug("On the fly: output format changes %dx%d->%dx%d\n",
		 1 << sha123->prev_info.log2chan,
		 sha123->prev_info.sampling_rate,
		 1 << sha123->frame.info.log2chan,
		 sha123->frame.info.sampling_rate);
    /* $$$ TODO : probably change sblimit and other stuff. */
  }
  return 0;
}

/*
 * @retval -1 error
 * @retval  2 finish
 * @retval  1 loop
 */
static int reach_eof(sha123_t * sha123)
{
  int code;

  sha123_debug("[%s] : read complete\n", istream_filename(sha123->istream));
  /* End of stream, loopz. */
  if (sha123->loop.count && !--sha123->loop.count) {
    sha123_debug("-> Loop counter reach 0 : finish\n");
    return 2;
  }

  sha123_debug("-> Loop counter [%d]\n", sha123->loop.count);
  /* Rewind to first valid frame. */
  code = istream_tell(sha123->istream);
  sha123_debug("-> current stream pos [%d]\n", code);
  if (code == -1) {
    sha123_set_error(sha123, "Can not get stream position");
  } else {
    code = sha123->loop.pos - code;
    sha123_debug("-> seek offset [%d]\n", code);
    code = istream_seek(sha123->istream, code);
    if (code == -1) {
      sha123_set_error(sha123, "Can not rewind");
    } else {
      code = 1;
    }
  }
  return code;
}

static int read_III_sideinfo(sha123_t * sha123)
{
  const int bytes = sha123->frame.info.ssize;
  int code;

#ifdef SHA123_PARANO
  if (bytes != 9 && bytes != 17 && bytes != 32 &&
      bytes != 11 && bytes != 19 && bytes != 34) {
    sha123_debug("III invalid side info size (%d)\n",bytes);
    sha123_set_error(sha123, "III invalid side info size");
    return -1;
  }
#endif

  sha123_debug("Reading %d III side info bytes\n", bytes);
  code = istream_read(sha123->istream,
		     sha123->frame.sideinfo,
		     bytes);
  return -(code != bytes);
}

static int read_frame_bytes(sha123_t * sha123)
{
  const unsigned int bytes = sha123->frame.info.frame_bytes;
  const unsigned int max_bytes = sizeof(sha123->frame.buffer.data);
  int code;

  sha123_debug("Reading %d frame bytes\n", bytes);

  if (bytes > max_bytes) {
    sha123_debug("frame size %u > %u\n", bytes, max_bytes);
    return -1;
  }
  code = istream_read(sha123->istream,
		      sha123->frame.buffer.data,
		      bytes);
  if (code != bytes) {
    sha123->frame.buffer.size = 0;
    if (code != -1) {
      code = 1;
    }
  } else {
    sha123->frame.buffer.size = code;
    code = 0;
  }
  return code;
}

static int skip_frame_bytes(sha123_t * sha123)
{
  const int bytes = sha123->frame.info.frame_bytes;
  int code;

  sha123_debug("Skipping %d frame bytes\n", bytes);
  code = istream_seek(sha123->istream, bytes);

  return -(code == -1);
}
