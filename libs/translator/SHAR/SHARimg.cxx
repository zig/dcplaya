/**
 * @ingroup SHARdesc
 * @file    SHARimg.cxx
 * @brief   Image descriptor implementation
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/07/11
 * @version $Id: SHARimg.cxx,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */

#include "SHAR/SHARimg.h"
#include "SHAsys/SHAsysPeek.h"

SHARimgDesc & SHARimgDesc::operator = (const SHARimgFileDesc & imgFileDesc)
{
  type        = imgFileDesc.Type();
  bitPerPixel = imgFileDesc.BitPerPixel();
  lutSize     = imgFileDesc.LutSize();
  lutType     = imgFileDesc.LutType();
  width       = imgFileDesc.Width();
  height      = imgFileDesc.Height();
  return *this;
}


void SHARimgFileDesc::Init(void)
{
  for(unsigned int i=0; i<sizeof(reserved); ++i) {
    reserved[i] = 0;
  }
}

SHARimgFileDesc::SHARimgFileDesc()
{
  Init();
}

SHARimgFileDesc::SHARimgFileDesc(SHARimgDesc & imgDesc)
{
  Init();
  Type(imgDesc.type);
  BitPerPixel(imgDesc.bitPerPixel);
  LutSize(imgDesc.lutSize);
  LutType(imgDesc.lutType);
  Width(imgDesc.width);
  Height(imgDesc.height);
}

void SHARimgFileDesc::Type(Type_e newType) {
  *type = newType;
}

void SHARimgFileDesc::Type(unsigned int newType) {
  *type = newType;
}

void SHARimgFileDesc::BitPerPixel(unsigned int newBitPerPixel)
{
  bitPerPixel[0] = newBitPerPixel;
}

void SHARimgFileDesc::LutSize(unsigned int newLutSize)
{
  SHApokeU16(lutSize, newLutSize);
}

void SHARimgFileDesc::LutType(Type_e newType)
{
  lutType[0] = newType;
}

void SHARimgFileDesc::LutType(unsigned int newType)
{
  lutType[0] = newType;
}

void SHARimgFileDesc::Width(unsigned int newWidth)
{
  SHApokeU32(width, newWidth);
}

void SHARimgFileDesc::Height(unsigned int newHeight)
{
  SHApokeU32(height, newHeight);
}

SHARimgFileDesc::Type_e SHARimgFileDesc::Type(void) const
{
  return (Type_e)(type[0]&255);
}

unsigned int SHARimgFileDesc::BitPerPixel(void) const
{
  return bitPerPixel[0] & 255;
}

unsigned int SHARimgFileDesc::LutSize(void) const
{
  return SHApeekU16(lutSize);
}

SHARimgFileDesc::Type_e SHARimgFileDesc::LutType(void) const
{
  return (Type_e)(lutType[0]&255);
}

unsigned int SHARimgFileDesc::Width(void) const
{
  return SHApeekU32(width);
}

unsigned int SHARimgFileDesc::Height(void) const
{
  return SHApeekU32(height);
}

const char * SHARimgDesc::ToString(unsigned int type)
{
  return SHARimgFileDesc::ToString(type);
}

const char * SHARimgFileDesc::ToString(unsigned int type)
{
  switch(type) 
  {
  case SHAPF_IND1:      return "IND1";
  case SHAPF_IND2:      return "IND2";
  case SHAPF_IND4:      return "IND4";
  case SHAPF_IND8:      return "IND8";
  case SHAPF_RGB233:    return "RGB233";
  case SHAPF_ARGB2222:  return "ARGB2222";
  case SHAPF_GREY8:     return "GREY8";
  case SHAPF_RGB565:    return "RGB565";
  case SHAPF_ARGB1555:  return "ARGB1555";
  case SHAPF_ARGB4444:  return "ARGB4444";
  case SHAPF_AIND88:    return "AIND88";
  case SHAPF_ARGB32:    return "ARGB32";
  case SHAPF_UNKNOWN:   return "UNKNOWN";
  }
  return "ERROR";
}
