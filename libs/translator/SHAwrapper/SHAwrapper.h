/**
 * @ingroup   dcplaya_devel
 * @file      SHAwrapper.h
 * @brief     SHAtranslator "C" wrapper
 * @date      2002/09/27
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAwrapper.h,v 1.5 2003-01-31 14:48:30 ben Exp $
 */
#ifndef _SHAWRAPPER_H_
#define _SHAWRAPPER_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "translator/SHAwrapper/SHAwrapperImage.h"

/** Load an image from a file. */
SHAwrapperImage_t * SHAwrapperLoadFile(const char *fname, int info_only);
  
/** Load an image from memory. */
SHAwrapperImage_t * SHAwrapperLoadMemory(const void *buffer, int size,
					 int info_only);
  
/** Add a new image translator. */
int SHAwrapperAddTranslator(void * translator);

/** Remove an image translator. */
int SHAwrapperDelTranslator(void * translator);

/** Blit an image block. */
void SHAwrapperBlitz(void *dst, int dw, int dh, int dformat, int dmod,
		     const void *src, int sw, int sh, int sformat, int smod);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //#define _SHAWRAPPER_H_
