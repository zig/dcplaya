/**
 * @file      SHAwrapper.cxx
 * @brief     SHAtranslator "C" wrapper
 * @date      2002/09/27
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAwrapper.cxx,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */

#include "SHAwrapper/SHAwrapper.h"
#include "SHAtk/SHAstreamMem.h"
#include "SHAtk/SHAstreamFile.h"
#include "SHAtranslator/SHAtranslator.h"
#include "SHAtranslator/SHAtranslatorTga.h"

#include "sysdebug.h"


static int TotalBytes(SHAtranslatorResult & result)
{
  return 0
    + sizeof(SHAwrapperImage_t)
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
//   SDDEBUG("type    : %x\n", result.data.image.type);
//   SDDEBUG("width   : %d\n", result.data.image.width);
//   SDDEBUG("height  : %d\n", result.data.image.height);
//   SDDEBUG("lutSize : %d\n", result.data.image.lutSize);
    
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


/* Test all translators one by one...
 */
static SHAwrapperImage_t * SHAwrapperLoad(SHAstream * in)
{
  // Built-in translator(s)
  // $$$ ben : Do not make them static, it crash !
  SHAtranslatorTga translatorTga;
  SHAtranslator * translators [] =
  {
    &translatorTga,
    0
  };
  SHAtranslator ** trans = translators , *t;
  SHAstreamPos pos;

  /* Save input stream starting position. */
  pos = in->Tell();
  if (pos == -1) {
    return 0;
  }

  SHAwrapperImage_t * img = 0;
  while (t = *trans++, t) {
    const char **ext;
    ext = t->Extension();
//     SDDEBUG("[%s] : translator [%s]\n", __FUNCTION__, ext[0]);
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
