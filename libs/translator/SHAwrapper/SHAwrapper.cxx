/**
 * @file      SHAwrapper.cxx
 * @brief     SHAtranslator "C" wrapper
 * @date      2002/09/27
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAwrapper.cxx,v 1.4 2002-12-16 23:39:36 ben Exp $
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
					  SHAstream * in, SHAstreamPos pos)
{
  int size;
  char *data;
  SHAtranslatorResult result;
  SHAstreamMem out;

  if (in->SeekTo(pos) == -1) {
    return 0;
  }

  /* Get image info */
  if (t->Info(in, &result) < 0) {
    SDERROR("[%s] : [%s]\n", __FUNCTION__, result.ErrorStr());
    return 0;
  }
//    SDDEBUG("type    : %x\n", result.data.image.type);
//    SDDEBUG("width   : %d\n", result.data.image.width);
//    SDDEBUG("height  : %d\n", result.data.image.height);
//    SDDEBUG("lutSize : %d\n", result.data.image.lutSize);
    
  /* Seek back to start of input stream */
  if (in->SeekTo(pos) == -1) {
    return 0;
  }
  size = TotalBytes(result);
  data = new char[size];
  if (!data) {
    SDERROR("[%s] : malloc error\n", __FUNCTION__);
    return 0;
  }

  /* Open memory stream in write mode */
  if (out.Open(data, size, 2)) {
    delete [] data;
    return 0;
  }

  /* Try to load image */
  if (t->Load(&out, in, &result) < 0) {
    SDERROR("[%s] : [%s]\n", __FUNCTION__, result.ErrorStr());
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
static SHAwrapperImage_t * SHAwrapperLoad(SHAstream * in)
{
  translator_list_t * l;

  // Built-in translator(s)
  // $$$ ben : Do not make them static, it crash !
//   SHAtranslatorTga translatorTga;
//   SHAtranslator * translators [] =
//   {
//     &translatorTga,
//     0
//   };
//   SHAtranslator ** trans = translators , *t;

  SHAtranslator *t;
  SHAstreamPos pos;

  /* Save input stream starting position. */
  pos = in->Tell();
  if (pos == -1) {
    return 0;
  }

  SHAwrapperImage_t * img = 0;
  for (l=tlist; l; l=l->next) {
	t = l->translator;

    const char **ext;
    ext = t->Extension();
 	SDDEBUG("[%s] : translator [%s]\n", __FUNCTION__, ext[0]);
    img = SHAwrapperLoad(t, in, pos);
    if (img) {
      break;
    }
  }
  return img;
}

SHAwrapperImage_t * SHAwrapperLoadFile(const char *fname)
{
  int err;
  SHAstreamFile in;

  /* Open input stream */
  err = in.Open(fname, "rb");
  if (err) {
    return 0;
  }

  return SHAwrapperLoad(&in);
}

SHAwrapperImage_t * SHAwrapperLoadMemory(void *buffer, int size)
{
  SHAstreamMem in;

  /* Open input stream */
  if (in.Open((char *)buffer, size, 1)) {
    return 0;
  }

  return SHAwrapperLoad(&in);
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
