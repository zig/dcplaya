/**
 * @ingroup  dcplaya_translator
 * @file     translator.c
 * @author   benjamin gerard <ben@sashipa.com>
 * @brief    Image translator C interface.
 *
 * $Id: translator.c,v 1.3 2002-12-15 16:15:03 ben Exp $
 */

#include "translator/translator.h"

SHAwrapperImage_t * LoadImageFile(const char * fname)
{
  return SHAwrapperLoadFile(fname);
}

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
