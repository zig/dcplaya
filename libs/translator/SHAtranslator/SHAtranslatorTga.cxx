/**
 * @ingroup   SHAtranslatorImage
 * @file      SHAtranslatorTga.cxx
 * @brief     Targa (TGA) translator class implementation
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorTga.cxx,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

#include <string.h>
#include "SHAtranslator/SHAtranslatorTga.h"
#include "SHAtranslator/SHAtranslatorBlitter.h"
#include "SHAsys/SHAsysPeek.h"

SHAtranslatorTga::SHAtranslatorTga()
{
}


// RGB Pixel trans
typedef void (*TGArgbConvertor_f)(void *dst, const void *src, int n);

// Any pixel translator
typedef void (*TGApixelConvertor_f)(void *dst, const void *src,
				    const void *lut, int n, int bpp,
				    TGArgbConvertor_f convert);

static void TGAindirect8(void *dst, const void * src, const void *lut,
			 int n, int bpp, TGArgbConvertor_f convert)
{
  for (int i=0; i<n; ++i) {
    unsigned int index;
    index = *(unsigned char *)src;
    convert(dst, (char *)lut + (index * bpp), 1);
    dst = (char*)dst + 4;
    src = (char *)src + 1;
  }
}

static void TGAindirect16(void *dst, const void * src, const void *lut, int n, int bpp, TGArgbConvertor_f convert)
{
  for (int i=0; i<n; ++i) {
    unsigned int index;
    index = SHApeekU16(src);
    convert(dst, (char *)lut + (index * bpp), 1);
    dst = (char*)dst + 4;
    src = (char *)src + 2;
  }
}

static void TGAdirect(void *dst, const void * src, const void *lut,
		      int n, int bpp, TGArgbConvertor_f convert)
{
  convert(dst, src, n);
}

static void TGAconvertor_ARGB1555_ARGB32(void *dst, const void *src, int n,
					 const void *lut)
{
  ARGB1555toARGB32(dst,src,n);
}

static void TGAconvertor_ARGB4444_ARGB32(void *dst, const void *src, int n,
					 const void *lut)
{
  ARGB4444toARGB32(dst,src,n);
}

static void TGAconvertor_RGB565_ARGB32(void *dst, const void *src, int n,
				       const void *lut)
{
  RGB565toARGB32(dst,src,n);
}

const char **SHAtranslatorTga::Extension(void) const
{                          //01234 56789A BCDEF 01234 56789 A
  static const char ext[] = ".tga\0.tpic\0.vda\0.icb\0.vst\0\0";
  static const char * exts[] = { ext, ext+5, ext+0xB, ext+0x10, ext+0x15, 0 };
  return exts;
}

///$$$ Place this in SHAstring
inline static int SHAmemcmp(const void *a, const void *b, unsigned int n)
{
  return memcmp(a,b,n);
}

int SHAtranslatorTga::ReadHeader(SHAtranslatorResult *result,
				 TGAheader * hd, SHAstream * inStream) const
{
  TGAfileHeader fhd;
  if (sizeof(fhd) != 18) {
    return result->Error("TGA: fatal, compiled tga file header size not 18");
  }
  int err = inStream->Read(&fhd, sizeof(fhd));
  if (err) {
    err = result->Error("TGA: header read failure", -1);
  } else {
    *hd = fhd;
  }
  return err;
}

int SHAtranslatorTga::RGBtype(int bpp, int alphaBit)
{
  int type = SHAPF_UNKNOWN;

  if (bpp == 15) {
    type = SHAPF_ARGB1555;
  } else if (bpp == 16) {
    if (alphaBit == 0) {
      type = SHAPF_RGB565;
    } else if (alphaBit == 1) {
      type = SHAPF_ARGB1555;
    } else if (alphaBit == 4) {
      type = SHAPF_ARGB4444;
    }
  } else if (bpp == 24 || bpp == 32) {
    type = SHAPF_ARGB32;
  }
  return type;
}

int SHAtranslatorTga::ConvertType(SHAtranslatorResult *result,
				  TGAheader * hd) const
{
  Format_e tgatype = (Format_e )hd->type;
  unsigned int bpp = hd->bpp;
  unsigned int alphaBit = hd->descriptor & 15;

  result->data.image.bitPerPixel = 0;
  result->data.image.type    = SHAPF_UNKNOWN;
  result->data.image.lutType = SHAPF_UNKNOWN;
  result->data.image.lutSize = 0;

  switch(tgatype)
  {
  case NODATA:
    break;

  case PAL:
  case RLEPAL:
    // In pallet mode, alpha bit is probably for palette since it is meaningless for index...
    result->data.image.lutSize      = hd->colormap_n;
    result->data.image.lutType      = RGBtype(hd->colormap_bit, alphaBit);
    result->data.image.bitPerPixel  = bpp;
    result->data.image.type         = SHAPF_IND8;
    break;

  case RLERGB:
  case RGB:
    result->data.image.bitPerPixel = (bpp + 7) & -8;
    result->data.image.type = RGBtype(bpp, alphaBit);
    if (result->data.image.bitPerPixel == 24) {
      result->data.image.bitPerPixel = 32;
    }
    break;

  case RLEGREY:
  case GREY:
    if (bpp == 8) {
      result->data.image.bitPerPixel = bpp;
      result->data.image.type = SHAPF_GREY8;
    }
    break;
  }

  if (result->data.image.type == SHAPF_UNKNOWN) {
    return result->Error("TGA: Don't understand this TGA format");
  }
  return 0;
}


int SHAtranslatorTga::CheckHeader(SHAtranslatorResult *result,
				  TGAheader * hd) const
{
  int err;

  err = ConvertType(result, hd);
  if (!err) {
    result->data.image.width       = hd->w;
    result->data.image.height      = hd->h;
    if (hd->colormap_type & ~1) {
      err = result->Error("TGA: bad colormap type (not 0 or 1)");
    }
  }
  return err;
}

int SHAtranslatorTga::ReadCheckHeader(SHAtranslatorResult *result,
				      TGAheader * hd,
				      SHAstream * inStream) const
{
  int err;

  err = ReadHeader(result, hd, inStream);
  if (!err) {
    err = CheckHeader(result, hd);
  }
  return err;
}

int SHAtranslatorTga::Test(SHAstream * in)
{
  SHAtranslatorResult defaultResult;  //dummy result...
  TGAheader hd;
  return ReadCheckHeader(&defaultResult, &hd, in);
}

int SHAtranslatorTga::Info(SHAstream * in, SHAtranslatorResult *result)
{
  SHAtranslatorResult defaultResult;
  TGAheader hd;
  int err;

  // Prepare result
  if (!result) {
    result = &defaultResult;
  }
  result->Clean();

  err = ReadCheckHeader(result, &hd, in);
  if (err < 0) {
    return err;
  }
  return 0;
}

void SHAtranslatorTga::HorizontalFlip(unsigned char *pix, int w,
				      int bytePerPix) const
{
  unsigned char data[16];
  unsigned char * pixEnd = pix + (w-1) * bytePerPix;
  while (pix < pixEnd) {
    for (int i=0; i<bytePerPix; ++i) {
      data[i]   = pix[i];
      pix[i]    = pixEnd[i];
      pixEnd[i] = data[i];
    }
    pix    += bytePerPix;
    pixEnd -= bytePerPix;
  }
}


int SHAtranslatorTga::ReadLine(SHAtranslatorResult * result,
			       unsigned char *dest, SHAstream * in,
			       int width, int bytePerPix) const
{
  int err;
  err = in->Read(dest, width * bytePerPix);
  if (err) {
    err = result->Error("TGA: Read line data failure.");
  }
  return err;
}

int SHAtranslatorTga::RleLineOffset(SHAtranslatorResult * result,
				    SHAstreamPos * linePos, SHAstream * in,
				    int width, int height,
				    int bytePerPix) const
{
  int err = 0;
  unsigned char data;

  for (int y=0; !err && y<height; ++y) {
    int cnt = 0;

    // Get current pos as line position
    linePos[y] = in->Tell();
    if ((int)linePos[y] < 0) {
      err = result->Error("TGA: Get RLE line position failure.");
      break;
    }

    while (cnt < width) {
      // Get RLE control byte
      err = in->Read(&data, 1);
      if (err) {
        err = result->Error("TGA: Get RLE line position, read control byte failure.");
        break;
      }
      int len = (data & 0x7f) + 1;
      cnt += len;
      if (cnt > width) {
        err = result->Error("TGA: Get RLE line position, line too long.");
        break;
      }

      SHAstreamPos seekOffset = bytePerPix * ((data & 0x80) ? 1 : len);
      err = in->SeekFrom(seekOffset);
      if (err) {
        err = result->Error("TGA: Get RLE line position, skip RLE data failure.");
        break;
      }
    }
  }

  return err;
}


int SHAtranslatorTga::ReadRleLine(SHAtranslatorResult * result,
				  unsigned char *dest,  SHAstream * in,
				  int width, int bytePerPix) const
{
  int err = 0;
  unsigned char data[4];
  int cnt = 0;

  while (cnt < width) {
    err = in->Read(data, 1);
    if (err < 0) {
      break;
    }
    unsigned int len = (data[0] & 0x7f) + 1;
    cnt += len;
    if (cnt > width) {
      break;
    }

    if (data[0] & 0x80) {
      // Run length packet : read next pix and duplic it
      err = in->Read(data, bytePerPix);
      if (err < 0) {
        break;
      }
      while (len--) {
        for (int i=0; i<bytePerPix; ++i) {
          *dest++ = data[i];
        }
      }
    } else {
      // Raw packet : direct read pix
      err = in->Read(dest, bytePerPix * len);
      dest += bytePerPix * len;
      if (err) {
        break;
      }
    }
  }

  if (err) {
    err = result->Error("TGA: RLE read failure");
  } else if (cnt != width) {
    err = result->Error("TGA: RLE depacked line size don't match");
  }
  return err;
}

int SHAtranslatorTga::Load(SHAstream * out,
                           SHAstream * in,
                           SHAtranslatorResult * result)
{
  unsigned char *pix = 0;
  unsigned char *lut = 0;
  SHAstreamPos  *linePos = 0;
  SHAtranslatorResult defaultResult;
  TGAheader hd;
  int err;

  int y;
  int lutSize, lutBpp, lutBytes;
  int rle, vFlip, hFlip;
  int pixBpp, pixBytes, lineSize;
  SHAstreamPos pixPos;

  // Prepare result
  if (!result) {
    result = &defaultResult;
  }
  result->Clean();

  // Read and check tga header
  err = ReadCheckHeader(result, &hd, in);
  if (err < 0) {
    goto error;
  }

  // Skip id field
  if (hd.idfield_size) {
    if (in->SeekFrom(hd.idfield_size)) {
      err = result->Error("TGA: Skip id field failure");
      goto error;
    }
  }

  // See if TGA format is convertible,
  TGArgbConvertor_f rgbConvertor;
  TGApixelConvertor_f pixelConvertor;
  err = GetConvertor(result, &hd, (void**)&rgbConvertor,
		     (void**)&pixelConvertor);
  if (err) {
    goto error;
  }

  // Load LUT if any ...
  lutSize = 0;
  lutBpp  = 0;
  lutBytes = 0;
  if (hd.colormap_type) {
    lutBpp  = (hd.colormap_bit + 7) & -8;
    lutBytes = lutBpp >> 3;
    lutSize = hd.colormap_n * lutBytes;
    lut = new unsigned char [lutSize];
    if (!lut) {
      err = result->Error("TGA: LUT buffer allocation failure");
      goto error;
    } else {
      err = in->Read(lut,lutSize);
      if (err) {
        err = result->Error("TGA: LUT read failure");
        goto error;
      }
    }
  }

  // Set some processing flags...
  rle = hd.type >= 9;
  vFlip = ~hd.descriptor & (1<<5);
  hFlip = hd.descriptor & (1<<4);
  pixPos = 0;

  // Get pixel address in stream for vertical flip (no RLE)
  if (vFlip && !rle) {
    pixPos = in->Tell();
    if ((int)pixPos < 0) {
      err = result->Error("TGA: Vertical flip, image position failure.");
      goto error;
    }
  }

  // Image pixel info
  pixBpp   = (hd.bpp + 7) & -8;
  pixBytes = pixBpp >> 3;
  lineSize = hd.w * pixBytes;

  // Get line offset in RLE format for vertical flip
  if (rle && vFlip) {
    err = 0;
    linePos = new SHAstreamPos[hd.h];
    if (!linePos) {
      err = result->Error("TGA: RLE line start buffer allocation failure");
    } else {
      err = RleLineOffset(result, linePos, in, hd.w, hd.h, pixBytes);
    }
    if (err) {
      goto error;
    }
  }

  // Alloc line buffer
  pix = new unsigned char [lineSize];
  if (!pix) {
    err = result->Error("TGA: line buffer allocation failure");
    goto error;
  }

  // Temporary buffer for image convertion
  char convBuffer[32*4];

  // Output is always ARGB32
  result->data.image.type        = SHAPF_ARGB32;
  result->data.image.bitPerPixel = 32;
  result->data.image.lutSize     = 0;
  result->data.image.lutType     = 0;

  // Write SHAR header
  err = WriteHeader(result, out);
  if (err) {
    goto error;
  }

  for (y=0; y<(int)hd.h; ++y) {
    // Seek to start of line
    if (vFlip) {
      int y2 = hd.h-y-1;
      SHAstreamPos seekPos;
      seekPos = linePos ? linePos[y2] : (pixPos + lineSize * y2);
      if (in->SeekTo(seekPos)) {
        err = result->Error("TGA: Seek to start of line failure.");
        break;
      }
    }

    // read line
    if (rle) {
      err = ReadRleLine(result, pix, in, hd.w, pixBytes);
    } else {
      err = ReadLine(result, pix, in, hd.w, pixBytes);
    }
    if (err) {
      break;
    }

    // re-order horizontal
    if (hFlip) {
      HorizontalFlip(pix, hd.w, pixBytes);
    }

    // convert and write
    int rem = hd.w;
    unsigned char *pix_ptr = pix;
    while (rem) {
      const int max = sizeof(convBuffer) >> 2;
      int n = rem > max ? max : rem;
      rem -= n;
      pixelConvertor(convBuffer, pix_ptr, lut, n, lutBytes, rgbConvertor);
      pix_ptr += n * pixBytes;
      err = out->Write(convBuffer, n * 4);
      if (err) {
        err = result->Error("TGA: Error writting pixel");
        goto error;
      }
    }
  }

error:
  if (lut) {
    delete [] lut;
  }
  if (pix) {
    delete [] pix;
  }
  if (linePos) {
    delete [] linePos;
  }
  return err;
}

int SHAtranslatorTga::Save(SHAstream * out,
                           SHAstream * in,
                           SHAtranslatorResult * result)
{
  SHAtranslatorResult defaultResult;
  if (!result) {
    result = &defaultResult;
  }
  result->Clean();
  return result->Error("TGA: save not implemented");
}

SHAtranslatorTga::TGAheader::TGAheader()
{
  for (unsigned int i=0; i<sizeof(*this)/sizeof(int); ++i) {
    ((unsigned int *)this)[i] = 0;
  }
}

void SHAtranslatorTga::TGAheader::operator = (const TGAfileHeader & fHd)
{
  idfield_size  = *fHd.idfield_size & 255;
  colormap_type = *fHd.colormap_type & 255;
  type          = *fHd.type & 255;
  colormap_org  = SHApeekU16(fHd.colormap_org);
  colormap_n    = SHApeekU16(fHd.colormap_n);
  colormap_bit  = *fHd.colormap_bit & 255;
  xorg          = SHApeekU16(fHd.xorg);
  yorg          = SHApeekU16(fHd.yorg);
  w             = SHApeekU16(fHd.w);
  h             = SHApeekU16(fHd.h);
  bpp           = *fHd.bpp & 255;
  descriptor    = *fHd.descriptor & 255;
}

int SHAtranslatorTga::GetConvertor(SHAtranslatorResult * result,
				   TGAheader * hd, void ** rgbConvertor,
				   void ** pixelConvertor) const
{
  Format_e tgatype = (Format_e )hd->type;
  unsigned int bpp = hd->bpp;
  unsigned int alphaBit = hd->descriptor & 15;

  *rgbConvertor = 0;
  *pixelConvertor = 0;

  switch (tgatype) {
    case PAL:
    case RLEPAL:
      bpp = (bpp + 7) & -8;
      if (bpp == 8) {
        *pixelConvertor = (void *)TGAindirect8;
      } else if (bpp == 16) {
        *pixelConvertor = (void *)TGAindirect16;
      }
      bpp = hd->colormap_bit;

    case RLERGB:
    case RGB:
      if (!*pixelConvertor ) {
        *pixelConvertor =(void *) TGAdirect;
      }
      if (bpp == 15) {
        *rgbConvertor = (void *)ARGB1555toARGB32;
      } else if (bpp == 16) {
        if (alphaBit == 0) {
          *rgbConvertor = (void *)RGB565toARGB32;
        } else if (alphaBit == 1) {
          *rgbConvertor = (void *)ARGB1555toARGB32;
        } else if (alphaBit == 4) {
          *rgbConvertor = (void *)ARGB4444toARGB32;
        }
      } else if (bpp == 24) {
        *rgbConvertor = (void *)RGB24toARGB32;
      } else if (bpp == 32) {
        *rgbConvertor = (void *)ARGB32toARGB32;
      }
      break;

    case RLEGREY:
    case GREY:
      *rgbConvertor   = (void *)GREY8toARGB32;
      break;
  }

  if (!*pixelConvertor || !*rgbConvertor) {
    return result->Error("TGA: TGA format has no suitable convertor.");
  }
  return 0;
}

