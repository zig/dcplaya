/**
 * @ingroup   SHAtranslatorImage
 * @file      SHAtranslatorImage.cxx
 * @brief     Image translator base class implementation
 * @date      2001/07/19
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorImage.cxx,v 1.2 2002-10-05 09:43:58 benjihan Exp $
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

  v.type    = result->data.image.type;
  v.width   = result->data.image.width;
  v.height  = result->data.image.height;
  v.lutSize = result->data.image.lutSize;

  err = out->Write(&v, sizeof(v));
  if (err) {
    err = result->Error("Image : Error writing header");
  }
  return err;
}
