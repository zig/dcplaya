/**
 * @file      SHAwrapper.cxx
 * @brief     SHAtranslator "C" wrapper
 * @date      2002/09/27
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAwrapper.cxx,v 1.6 2003-01-31 14:48:30 ben Exp $
 */

#include "SHAwrapper/SHAwrapper.h"
#include "SHAtk/SHAstreamMem.h"
#include "SHAtk/SHAstreamFile.h"
#include "SHAtranslator/SHAtranslator.h"
#include "SHAblitter/SHAblitter.h"

#include "sysdebug.h"


static int TotalBytes(SHAtranslatorResult & result)
{
  SHAwrapperImage_t *img = 0;
  return 0
    + sizeof(SHAwrapperImage_t) - sizeof(img->data) + 
    + result.data.image.width * result.data.image.height * 4;
}


static SHAwrapperImage_t * SHAwrapperLoad(SHAtranslator * t,
					  SHAstream * in, SHAstreamPos pos,
					  int info_only)
{
  int size;
  char *data;
  SHAtranslatorResult result;
  SHAstreamMem out;
  const char * ext = t->Extension()[0];

  if (in->SeekTo(pos) == -1) {
	SDERROR("Translator [%s] : Can't seek to position [%d].\n", ext, pos);
    return 0;
  }

  /* Get image info */
  if (t->Info(in, &result) < 0) {
    SDERROR("Translator [%s] : [%s]\n", ext, result.ErrorStr());
    return 0;
  }

  if (info_only) {
    SHAwrapperImage_t *info  = new SHAwrapperImage_t;
    if (!info) {
      SDERROR("[Translator] [%s] : info malloc error.\n", ext);
    } else {
      info->type = (SHAwrapperImageFormat_e)result.data.image.type;
      info->width = result.data.image.width;
      info->height = result.data.image.height;
      info->lutSize = result.data.image.lutSize;
      info->ext = ext;
    }
    return info;
  }

//    SDDEBUG("type    : %x\n", result.data.image.type);
//    SDDEBUG("width   : %d\n", result.data.image.width);
//    SDDEBUG("height  : %d\n", result.data.image.height);
//    SDDEBUG("lutSize : %d\n", result.data.image.lutSize);
    
  /* Seek back to start of input stream */
  if (in->SeekTo(pos) == -1) {
    SDERROR("Translator [%s] : Can't seek to position [%d].\n", ext, pos);
    return 0;
  }
  size = TotalBytes(result);
  data = new char[size];
  if (!data) {
    SDERROR("Translator [%s] : malloc (%d bytes) error.\n", ext, size);
    return 0;
  }

  /* Open memory stream in write mode */
  if (out.Open(data, size, 2)) {
    SDERROR("Translator [%s] : Open output memory [%p,%d] error.\n",
			ext,data,size);
    delete [] data;
    return 0;
  }

  /* Try to load image */
  if (t->Load(&out, in, &result) < 0) {
    SDERROR("Translator [%s] : [%s]\n", ext, result.ErrorStr());
    delete [] data;
    data = 0;
  }
  
  return (SHAwrapperImage_t *) data;
}

typedef struct translator_list_s 
{
  struct translator_list_s * next;
  SHAtranslator * translator;
} translator_list_t;

static translator_list_t * tlist = 0;

int SHAwrapperAddTranslator(void * t)
{
  translator_list_t * l;
  if (!t) {
	return -1;
  }
  
  l = new translator_list_t;
  if (!l) {
	return -1;
  }
  l->next = tlist;
  l->translator = (SHAtranslator *)t;
  SDDEBUG("[%s] : [%s]\n", __FUNCTION__, l->translator->Extension()[0]);
  tlist = l;
  return 0;
}

int SHAwrapperDelTranslator(void * t)
{
  translator_list_t * l, * p;

  for (p=0, l=tlist; l && l->translator != t; p=l, l=l->next)
	;
  if (!l) {
	return -1;
  }
  if (p) {
	p->next = l->next;
  } else {
	tlist = l->next;
  }

  SDDEBUG("[%s] : [%s]\n", __FUNCTION__, l->translator->Extension()[0]);

  delete l;
  return 0;
}
  
/* Test all translators one by one...
 */
static SHAwrapperImage_t * SHAwrapperLoad(SHAstream * in, int info_only)
{
  translator_list_t * l;
  SHAtranslator *t;
  SHAstreamPos pos;

  /* Save input stream starting position. */
  pos = in->Tell();
  if (pos == -1) {
 	SDERROR("[%s] : Can't get file pointer position.\n", __FUNCTION__);
    return 0;
  }

  SHAwrapperImage_t * img = 0;
  for (l=tlist; l; l=l->next) {
    t = l->translator;

    const char **ext;
    ext = t->Extension();
 	SDDEBUG("[%s] : translator [%s]\n", __FUNCTION__, ext[0]);
    img = SHAwrapperLoad(t, in, pos, info_only);
    if (img) {
      break;
    }
  }
  return img;
}

SHAwrapperImage_t * SHAwrapperLoadFile(const char *fname, int info_only)
{
  int err;
  SHAstreamFile in;

  /* Open input stream */
  err = in.Open(fname, "rb");
  if (err) {
    SDERROR("Open file [%s] failed\n",fname);
    return 0;
  }

  return SHAwrapperLoad(&in, info_only);
}


SHAwrapperImage_t * SHAwrapperLoadMemory(const void *buffer,
					 int size, int info_only)
{
  SHAstreamMem in;

  /* Open input stream */
  if (in.Open((char *)buffer, size, 1)) {
    SDERROR("Open memory [%p,%d] failed\n",buffer,size);
    return 0;
  }

  return SHAwrapperLoad(&in, info_only);
}

/** Blit an image block. */
void SHAwrapperBlitz(void *dst, int dw, int dh, int dformat, int dmod,
		     const void *src, int sw, int sh, int sformat, int smod)
{
  SHAblitter blitter;

  blitter.Source((SHApixelFormat_e)sformat, (void *)src, sw, sh, smod);
  blitter.Destination((SHApixelFormat_e)dformat, dst, dw, dh, dmod);
  blitter.Copy();
}
