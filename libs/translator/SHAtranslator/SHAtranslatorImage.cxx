/**
 * @ingroup   SHAtranslatorImage
 * @file      SHAtranslatorImage.cxx
 * @brief     Image translator base class implementation
 * @date      2001/07/19
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: SHAtranslatorImage.cxx,v 1.4 2002-12-15 16:13:36 ben Exp $
 */


#include "SHAtranslator/SHAtranslatorImage.h"
#include "SHAwrapper/SHAwrapperImage.h"

SHAtranslatorImage::SHAtranslatorImage ()
{
  Init();
}

void SHAtranslatorImage::Init(void)
{
}

int SHAtranslatorImage::WriteHeader(SHAtranslatorResult *result,
				    SHAstream * out)
{
  SHAwrapperImage_t v;
  int err;

  v.type    = (SHAwrapperImageFormat_e)result->data.image.type;
  v.width   = result->data.image.width;
  v.height  = result->data.image.height;
  v.lutSize = result->data.image.lutSize;

  err = out->Write(&v, sizeof(v) - sizeof(v.data));
  if (err) {
    err = result->Error("Image : Error writing header");
  }
  return err;
}
