/**
 * @ingroup  dcplaya_translator
 * @file     translator.c
 * @author   benjamin gerard <ben@sashipa.com>
 * @brief    Image translator C interface.
 *
 * $Id: translator.c,v 1.5 2003-01-31 14:48:30 ben Exp $
 */

#include "translator/translator.h"

/** Load an image file. */
SHAwrapperImage_t * LoadImageFile(const char * fname, int info_only)
{
  return SHAwrapperLoadFile(fname, info_only);
}

/** Load an image from memory. */
SHAwrapperImage_t * LoadImageMemory(const void *buffer, int size,
				    int info_only)
{
  return SHAwrapperLoadMemory(buffer, size, info_only);
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
