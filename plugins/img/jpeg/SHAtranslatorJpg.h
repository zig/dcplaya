/**
 * @ingroup   dcplaya_img_plugin_devel
 * @file      SHAtranslatorJpg.h
 * @brief     Jpeg (JPG) translator class definition
 * @date      2001/07/17
 * @author    Benjamin Gerard <ben@sashipa.com>
 *
 * $Id: SHAtranslatorJpg.h,v 1.2 2003-03-19 05:16:16 ben Exp $
 *
 * How to decompress JPEG via jpeglib.
 *
 * JPEG decompression operations:
 *	- Allocate and initialize a JPEG decompression object
 *	- Specify the source of the compressed data (eg, a file)
 *	- Call jpeg_read_header() to obtain image info
 *	- Set parameters for decompression
 *	- jpeg_start_decompress(...);
 *	- while (scan lines remain to be read)
 *    - jpeg_read_scanlines(...);
 *	- jpeg_finish_decompress(...);
 *	- Release the JPEG decompression object
 */

#ifndef _SHATRANSLATORJPG_H_
#define _SHATRANSLATORJPG_H_

#include "SHAtranslator/SHAtranslatorImage.h"

/** @defgroup  dcplaya_jpg_img_plugin_devel  JPEG image plugin.
 *  @ingroup   dcplaya_img_plugin_devel
 *  @author    benjamin gerard <ben@sashipa.com>
 *  @brief     Jpeg (.jpg) image plugin
 */


/** Jpeg (.JPG) translator class definition.
 *
 * @ingroup   dcplaya_jpg_img_plugin_devel
 * @author    Benjamin Gerard <ben@sashipa.com>
 */
class SHAtranslatorJpg : public SHAtranslatorImage
{

public:
  SHAtranslatorJpg();

  virtual const char **Extension(void) const;

  virtual int Test(SHAstream * inStream);

  virtual int Info(SHAstream * inStream,
                   SHAtranslatorResult * result);

  virtual int Load(SHAstream * outStream,
                   SHAstream * inStream,
                   SHAtranslatorResult * result);

  virtual int Save(SHAstream * outStream,
                   SHAstream * inStream,
                   SHAtranslatorResult * result);

private:
  int SetOutputFormat(SHAtranslatorResult * result, void * jpgInfo) const;

};

#endif //#define _SHATRANSLATORJPG_H_
