/**
 * @ingroup   SHAtranslatorImage
 * @file      SHAtranslatorTga.h
 * @brief     Targa (TGA) translator class definition
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorTga.h,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

#ifndef _SHATRANSLATORTGA_H_
#define _SHATRANSLATORTGA_H_

#include "SHAtranslator/SHAtranslatorImage.h"

/** True-Vision Targa (.TGA) translator class definition.
 *
 * @ingroup   SHAtranslatorImage
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAtranslatorTga : public SHAtranslatorImage
{

public:
  SHAtranslatorTga();

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
  /// TGA file header descriptor.

  struct TGAfileHeader
  {
    char    idfield_size[1];  ///< id field lenght in byte
    char    colormap_type[1]; ///< 0:no color table, 1:color table, >=128:user defined
    char    type[1];          ///< pixel encoding format

    char    colormap_org[2];  ///< first color used in colormap
    char    colormap_n[2];    ///< number of colormap entries
    char    colormap_bit[1];  ///< number of bit per colormap entries [15,16,24,32]

    char    xorg[2];          ///< horizontal coordinate of lower left corner of image
    char    yorg[2];          ///< vertical coordinate of lower left corner of image
    char    w[2];             ///< width of image in pixel
    char    h[2];             ///< height of image in pixel
    char    bpp[1];           ///< bit per pixel
    char    descriptor[1];    ///< image descriptor bits [76:clear|5:bottom/top|4:left/right|3210:alpha bits]
  };

  struct TGAheader
  {
    unsigned int idfield_size;  ///< id field lenght in byte
    unsigned int colormap_type; ///< 0:no color table, 1:color table, >=128:user defined
    unsigned int type;          ///< pixel encoding format
    unsigned int colormap_org;  ///< first color used in colormap
    unsigned int colormap_n;    ///< number of colormap entries
    unsigned int colormap_bit;  ///< number of bit per colormap entries [15,16,24,32]
    unsigned int xorg;          ///< horizontal coordinate of lower left corner of image
    unsigned int yorg;          ///< vertical coordinate of lower left corner of image
    unsigned int w;             ///< width of image in pixel
    unsigned int h;             ///< height of image in pixel
    unsigned int bpp;           ///< bit per pixel
    unsigned int descriptor;    ///< image descriptor bits [76:clear|4:bottom/top|5:left/right|3210:alpha bits]

    TGAheader();
    void operator = (const TGAfileHeader & fHd);

  };

  static int RGBtype(int bpp, int alphaBit);

  int ReadHeader(SHAtranslatorResult *result, TGAheader * hd, SHAstream * inStream) const;
  int CheckHeader(SHAtranslatorResult *result, TGAheader * hd) const;
  int ReadCheckHeader(SHAtranslatorResult *result, TGAheader * hd, SHAstream * inStream) const;
  int ConvertType(SHAtranslatorResult *result, TGAheader * hd) const;
  int GetConvertor(SHAtranslatorResult * result, TGAheader * hd, void ** rgbConvertor, void ** pixelConvertor) const;


  int ReadLine(SHAtranslatorResult * result, unsigned char *dest, SHAstream * in, int width, int bytePerPix) const;
  int ReadRleLine(SHAtranslatorResult * result, unsigned char *dest, SHAstream * in, int width, int bytePerPix) const;
  int RleLineOffset(SHAtranslatorResult * result, SHAstreamPos * linePos, SHAstream * in, int width, int height, int bytePerPix) const;
  void HorizontalFlip(unsigned char *pix, int w, int bytePerPix) const;

  enum Format_e {
    NODATA   = 0,
    PAL      = 1,
    RGB      = 2,
    GREY     = 3,
    RLEPAL   = 9,
    RLERGB   = 10,
    RLEGREY  = 11
  };

};

#endif //#define _SHATRANSLATORTGA_H_
