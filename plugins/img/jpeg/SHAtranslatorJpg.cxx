/**
 * @ingroup   dcplaya_plugin
 * @file      SHAtranslatorJpg.cxx
 * @brief     Jpeg (JPG) translator class implementation
 * @date      2001/07/17
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorJpg.cxx,v 1.2 2002-12-15 12:44:15 zigziggy Exp $
 */

#include "SHAtranslatorJpg.h"
#include "SHAtranslator/SHAtranslatorBlitter.h"

#include <stdio.h>  // For jpeglib !

extern "C" {
#include "setjmp.h"
#include "jpeg/jpeglib.h"
}

SHAtranslatorJpg::SHAtranslatorJpg()
{
}

const char **SHAtranslatorJpg::Extension(void) const
{                          //01234 56789A BCDEF 01234 56789 A 
  static const char ext[] = ".jpg\0.jpeg\0\0";
  static const char * exts[] = { ext, ext+5, 0 };
  return exts;
}

/** Jpeg data destination manager */
struct SHAjpgDestinationMgr : public jpeg_destination_mgr 
{
  JOCTET buffer[512]; ///< Buffer to fill
  SHAstream * out;    ///< Output stream

  SHAjpgDestinationMgr(SHAstream * outStream);
  void Reset(void);
};

/** Jpeg data source manager */
struct SHAjpgSourceMgr : public jpeg_source_mgr
{
  JOCTET buffer[512]; ///< Buffer to fill
  SHAstream * in;     ///< Input stream

  SHAjpgSourceMgr(SHAstream * inStream);
  void Reset(void);
};

struct SHAjpgErrorMgr : public jpeg_error_mgr
{
  SHAjpgErrorMgr();
  jmp_buf jmpBuf;
};

extern "C" {
  /**	Initialize destination.  This is called by jpeg_start_compress()
	 *  before any data is actually written.  It must initialize
	 *  next_output_byte and free_in_buffer.  free_in_buffer must be
	 *  initialized to a positive value.
   */
  static void SHAjpgInitDestination(j_compress_ptr cinfo)
  {
    SHAjpgDestinationMgr * mgr = (SHAjpgDestinationMgr *)cinfo->dest;
    mgr->Reset();
  }

	/** This is called whenever the buffer has filled (free_in_buffer
	 *  reaches zero).  In typical applications, it should write out the
	 *  *entire* buffer (use the saved start address and buffer length;
	 *  ignore the current state of next_output_byte and free_in_buffer).
	 *  Then reset the pointer & count to the start of the buffer, and
	 *  return TRUE indicating that the buffer has been dumped.
	 *  free_in_buffer must be set to a positive value when TRUE is
	 *  returned.  A FALSE return should only be used when I/O suspension is
	 *  desired (this operating mode is discussed in the next section).
   */
  static boolean SHAjpgOutputBuffer(j_compress_ptr cinfo)
  {
    SHAjpgDestinationMgr * mgr = (SHAjpgDestinationMgr *)cinfo->dest;
    int err;

    err = mgr->out->Write(mgr->buffer, sizeof(mgr->buffer));
    if (!err) {
      mgr->Reset();
      return TRUE;
    }
    return FALSE;
  }

	/** Terminate destination --- called by jpeg_finish_compress() after all
	 *  data has been written.  In most applications, this must flush any
	 *  data remaining in the buffer.  Use either next_output_byte or
	 *  free_in_buffer to determine how much data is in the buffer.
   */
  static void SHAjpgTermDestination(j_compress_ptr cinfo)
  {
    SHAjpgDestinationMgr * mgr = (SHAjpgDestinationMgr *)cinfo->dest;
    if (mgr->free_in_buffer > 0) {
      mgr->out->Write(mgr->next_output_byte, mgr->free_in_buffer);
    }
  }

  /** Initialize source.  This is called by jpeg_read_header() before any
	 *  data is actually read.  Unlike init_destination(), it may leave
	 *  bytes_in_buffer set to 0 (in which case a fill_input_buffer() call
	 *  will occur immediately).
   */
  static void SHAjpgInitSource(j_decompress_ptr cinfo)
  {
    SHAjpgSourceMgr * mgr = (SHAjpgSourceMgr *)cinfo->src;
    mgr->Reset();
  }

	/** This is called whenever bytes_in_buffer has reached zero and more
	 *  data is wanted.  In typical applications, it should read fresh data
	 *  into the buffer (ignoring the current state of next_input_byte and
	 *  bytes_in_buffer), reset the pointer & count to the start of the
	 *  buffer, and return TRUE indicating that the buffer has been reloaded.
	 *  It is not necessary to fill the buffer entirely, only to obtain at
	 *  least one more byte.  bytes_in_buffer MUST be set to a positive value
	 *  if TRUE is returned.  A FALSE return should only be used when I/O
	 *  suspension is desired (this mode is discussed in the next section).
   */
  static boolean SHAjpgFillInputBuffer(j_decompress_ptr cinfo)
  {
    SHAjpgSourceMgr * mgr = (SHAjpgSourceMgr *)cinfo->src;
    int err;

    mgr->next_input_byte = mgr->buffer; 
    err = mgr->in->Read(mgr->buffer, sizeof(mgr->buffer));
    if (err >= 0) {
      mgr->bytes_in_buffer = sizeof(mgr->buffer) - err;
      return TRUE;
    } 
    return FALSE;
  }

	/** Skip num_bytes worth of data.  The buffer pointer and count should
	 *  be advanced over num_bytes input bytes, refilling the buffer as
	 *  needed.  This is used to skip over a potentially large amount of
	 *  uninteresting data (such as an APPn marker).  In some applications
	 *  it may be possible to optimize away the reading of the skipped data,
	 *  but it's not clear that being smart is worth much trouble; large
	 *  skips are uncommon.  bytes_in_buffer may be zero on return.
	 *  A zero or negative skip count should be treated as a no-op.
   */
  static void SHAjpgSkipInputData(j_decompress_ptr cinfo, long num_bytes)
  {
    SHAjpgSourceMgr * mgr = (SHAjpgSourceMgr *)cinfo->src;
    if (num_bytes > 0) {
      if (num_bytes < (long)mgr->bytes_in_buffer) {
        mgr->bytes_in_buffer -= num_bytes;
        mgr->next_input_byte += num_bytes;
      } else {
        mgr->in->SeekFrom(num_bytes - mgr->bytes_in_buffer);
        mgr->Reset();
      }
    }
  }

	/** This routine is called only when the decompressor has failed to find
	 *  a restart (RSTn) marker where one is expected.  Its mission is to
	 *  find a suitable point for resuming decompression.  For most
	 *  applications, we recommend that you just use the default resync
	 *  procedure, jpeg_resync_to_restart().  However, if you are able to back
	 *  up in the input data stream, or if you have a-priori knowledge about
	 *  the likely location of restart markers, you may be able to do better.
	 *  Read the read_restart_marker() and jpeg_resync_to_restart() routines
	 *  in jdmarker.c if you think you'd like to implement your own resync
	 *  procedure.
   */
  //static boolean SHAjpgResyncToRestart(j_decompress_ptr cinfo, int desired)
  //{
  //  return jpeg_resync_to_restart(cinfo, desired);
  //}

	/** Terminate source --- called by jpeg_finish_decompress() after all
	 *  data has been read.  Often a no-op.
   */
  static void SHAjpgTermSource(j_decompress_ptr cinfo)
  {
  }

  static void SHAjpgErrorExit(j_common_ptr cinfo)
  {
    SHAjpgErrorMgr *mgr = (SHAjpgErrorMgr *)cinfo->err;
    longjmp(mgr->jmpBuf, -1);
  }

  static void SHAjpgOutputMessage(j_common_ptr cinfo)
  {
  }
}

SHAjpgDestinationMgr::SHAjpgDestinationMgr(SHAstream * outStream)
{
  out = outStream;
  next_output_byte = buffer;
  free_in_buffer = 0;
  // Set destination manager Callback
  init_destination    = SHAjpgInitDestination;
  empty_output_buffer = SHAjpgOutputBuffer;
  term_destination    = SHAjpgTermDestination;
}

void SHAjpgDestinationMgr::Reset(void)
{
  next_output_byte = buffer;
  free_in_buffer = sizeof(buffer);
}

SHAjpgSourceMgr::SHAjpgSourceMgr(SHAstream * inStream)
{
  in = inStream;
  next_input_byte = buffer;
  bytes_in_buffer = 0;
  // Set source manager Callback
  init_source         = SHAjpgInitSource;
  fill_input_buffer   = SHAjpgFillInputBuffer;
  skip_input_data     = SHAjpgSkipInputData;
  resync_to_restart   = jpeg_resync_to_restart;
  term_source         = SHAjpgTermSource;
}

void SHAjpgSourceMgr::Reset(void)
{
  next_input_byte = buffer;
  bytes_in_buffer = 0;
}

SHAjpgErrorMgr::SHAjpgErrorMgr()
{
  jpeg_std_error(this);
  error_exit      = SHAjpgErrorExit;
  //emit_message    = SHAjpgEmitMessage;
  output_message  = SHAjpgOutputMessage;
  //format_message  = SHAjpgFormatMessage;
  //reset_error_mgr = SHAjpgResetErrorMgr;
}

int SHAtranslatorJpg::Test(SHAstream * inStream)
{
  return Info(inStream, 0);
}

int SHAtranslatorJpg::Info(SHAstream * inStream, SHAtranslatorResult * result)
{
  volatile int init = 0; 
  int err;
  SHAjpgSourceMgr srcMgr(inStream);
  SHAtranslatorResult defaultResult;
  struct jpeg_decompress_struct cinfo;
  SHAjpgErrorMgr jerr;

  // Prepare result
  if (!result) {
    result = &defaultResult;
  }
  result->Clean();

  /* Initialize the JPEG decompression object with default error handling. */
  cinfo.err = &jerr;

  /* set error manager return point */
  err = setjmp(jerr.jmpBuf);
  if(err) {
    // Jpeglib critical error !!
    err = result->Error("JPG: jpeglib internal unrecoverable error.");
    goto error;
  }

  // Allocate and initialize a JPEG decompression object
  jpeg_create_decompress(&cinfo);
  init = 1;

  // Specify the source of the compressed data (eg, a file)
  cinfo.src = &srcMgr;

  // Call jpeg_read_header() to obtain image info.
  /* If you pass require_image = TRUE (normal case), you need not check for
   * a TABLES_ONLY return code; an abbreviated file will cause an error exit.
   * JPEG_SUSPENDED is only possible if you use a data source module that can
   * give a suspension return (the stdio source module doesn't).
   */
  err = jpeg_read_header(&cinfo, TRUE /*require_image*/);
  if (err != JPEG_HEADER_OK) {
    err = result->Error("JPG: Read jpeg header failure.");
    goto error;
  }

  /* Calculate output image dimensions */
  jpeg_calc_output_dimensions(&cinfo);
  result->data.image.lutSize     = 0;
  result->data.image.lutType     = 0;
  result->data.image.width  = cinfo.image_width;
  result->data.image.height = cinfo.image_height;
  SetOutputFormat(result, &cinfo);

error:
  if (init) {
    // Release the JPEG decompression object
    jpeg_destroy_decompress(&cinfo);
  }
  return result->Error();
}

int SHAtranslatorJpg::Load(SHAstream * outStream,
                           SHAstream * inStream,
                           SHAtranslatorResult * result)
{
  volatile int init = 0; 
  int err;
  SHAjpgSourceMgr srcMgr(inStream);
//   SHARwriter sharwriter(outStream);
  SHAtranslatorResult defaultResult;
  struct jpeg_decompress_struct cinfo;
  SHAjpgErrorMgr jerr;
  JSAMPLE * lineBuffer = 0;
  int lineBufferSize;

  // Prepare result
  if (!result) {
    result = &defaultResult;
  }
  result->Clean();

  /* Initialize the JPEG decompression object with default error handling. */
  cinfo.err = &jerr;
  err = setjmp(jerr.jmpBuf);
  if(err) {
    err = result->Error("JPG: jpeglib internal unrecoverable error.");
    goto error;
  }

  jpeg_create_decompress(&cinfo);
  init = 1;

  // Specify the source of the compressed data (eg, a file)
  cinfo.src = &srcMgr;

  // Call jpeg_read_header() to obtain image info.
  /* If you pass require_image = TRUE (normal case), you need not check for
   * a TABLES_ONLY return code; an abbreviated file will cause an error exit.
   * JPEG_SUSPENDED is only possible if you use a data source module that can
   * give a suspension return (the stdio source module doesn't).
   */
  err = jpeg_read_header(&cinfo, TRUE /*require_image*/);
  if (err != JPEG_HEADER_OK) {
    err = result->Error("JPG: Read jpeg header failure.");
    goto error;
  }

  /* Calculate output image dimensions */
  jpeg_calc_output_dimensions(&cinfo);
  result->data.image.lutSize      = 0;
  result->data.image.lutType      = 0;
  result->data.image.width        = cinfo.image_width;
  result->data.image.height       = cinfo.image_height;

  /* Start jpeg decompress, as soon as possible to prevent from SHAR header
   * writting in bogus case.
   */
  jpeg_start_decompress(&cinfo);
  init = 3;

  err = SetOutputFormat(result, &cinfo);
  if (err) {
    goto error;
  }

  /* Alloc temporary scanline bufffer */
  lineBufferSize = cinfo.output_width * cinfo.output_components;
  lineBuffer = new JSAMPLE [lineBufferSize];
  if (!lineBuffer) {
    err = result->Error("JPG: temporary scanline allocation failed.");
    goto error;
  }
  
  // Convert to image file descriptor

  /* Write output file header */
  err = WriteHeader(result, outStream);
  if (err < 0) {
    goto error;
  }

  /* Process data, and convert to suitable output format */
  if (result->data.image.type == SHAPF_ARGB32) {
    while (cinfo.output_scanline < cinfo.output_height) {
      int num_scanlines = jpeg_read_scanlines(&cinfo, &lineBuffer, 1);
      if (num_scanlines == 1) {
        JSAMPLE dst[64*4];

        // Reserve endianess
        for (int i=0; i<lineBufferSize; i+=3) {
          JSAMPLE tmp = lineBuffer[i+0];
          lineBuffer[i+0] = lineBuffer[i+2];
          lineBuffer[i+2] = tmp;
        }

        // Convert to ARGB32 and write
        int rem = cinfo.output_width;
        JSAMPLE *src = lineBuffer;
        while (rem) {
          int n = sizeof(dst)>>2;
          if (n > rem) {
            n = rem;
          }
          rem -= n;
          RGB24toARGB32(dst, src, n);
          src += n * 3;
          err = outStream->Write(dst, n << 2);
          if (err) {
            err = result->Error("JPG: write image raw failure.");
            goto error;
          }
        }
      }
    }
  } else if (result->data.image.type == SHAPF_GREY8) {
    while (cinfo.output_scanline < cinfo.output_height) {
      int num_scanlines = jpeg_read_scanlines(&cinfo, &lineBuffer, 1);
      if (num_scanlines == 1) {
        err = outStream->Write(lineBuffer, lineBufferSize);
		if (err) {
		  err = result->Error("JPG: write image raw failure.");
		  goto error;
		}
      }
    }
  }

error:
  if (lineBuffer) {
    delete [] lineBuffer;
  }
  if (init&2) {
    jpeg_finish_decompress(&cinfo);
  }
  // Release the JPEG decompression object
  if (init&1) {
    jpeg_destroy_decompress(&cinfo);
  }
  return result->Error();
}

int SHAtranslatorJpg::Save(SHAstream * outStream,
                           SHAstream * inStream,
                           SHAtranslatorResult * result)
{
  SHAtranslatorResult defaultResult;
  if (!result) {
    result = &defaultResult;
  }
  result->Clean();
  return result->Error("JPG: save not implemented");
}

int SHAtranslatorJpg::SetOutputFormat(SHAtranslatorResult * result,
									  void * jpgInfo) const
{
  j_decompress_ptr  cinfo = (j_decompress_ptr)jpgInfo;

  result->data.image.type = SHAPF_UNKNOWN;
  result->data.image.bitPerPixel = 0;

  switch (cinfo->out_color_space) {
  case JCS_RGB:
    if (cinfo->num_components == 3) {
      result->data.image.type        = SHAPF_ARGB32;
      result->data.image.bitPerPixel = 32;
    }
    break;
  case JCS_GRAYSCALE:
    if (cinfo->num_components == 1) {
      result->data.image.type        = SHAPF_GREY8;
      result->data.image.bitPerPixel = 8;
    }
    break;
  }
  if (result->data.image.type == SHAPF_UNKNOWN) {
    return result->Error("JPG: Unsupported jpeg source format.");
  }
  return 0;
}
