/**
 * @ingroup  dcplaya_translator
 * @file     translator.c
 * @author   benjamin gerard <ben@sashipa.com>
 * @brief    Image translator C interface.
 *
 * $Id: translator.c,v 1.4 2002-12-16 23:39:36 ben Exp $
 */

#include "translator/translator.h"

/** Load an image file. */
SHAwrapperImage_t * LoadImageFile(const char * fname)
{
  return SHAwrapperLoadFile(fname);
}

/** Load an image from memory. */
SHAwrapperImage_t * LoadImageMemory(void *buffer, int size)
{
  return SHAwrapperLoadMemory(buffer, size);
}

/** Add a new image translator. */
int AddTranslator(translator_t translator)
{
  return SHAwrapperAddTranslator(translator);
}

/** Remove an image translator. */
int DelTranslator(translator_t translator)
{
  return SHAwrapperDelTranslator(translator);
}

/** Blit an image block. */
void Blitz(void *dst, int dw, int dh, int dformat, int dmodulo,
		   const void *src, int sw, int sh, int sformat, int smodulo)
{
  SHAwrapperBlitz(dst, dw, dh, dformat, dmodulo,
				  src, sw, sh, sformat, smodulo);
}
