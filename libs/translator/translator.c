/**
 * @ingroup  dcplaya_translator
 * @file     translator.c
 * @author   benjamin gerard <ben@sashipa.com>
 * @brief    Image translator C interface.
 *
 * $Id: translator.c,v 1.2 2002-11-25 20:31:39 ben Exp $
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
