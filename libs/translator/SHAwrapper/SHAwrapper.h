/**
 * @ingroup   dcplaya_shawrapper_devel
 * @file      SHAwrapper.h
 * @brief     SHAtranslator "C" wrapper
 * @date      2002/09/27
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAwrapper.h,v 1.6 2003-03-22 00:35:27 ben Exp $
 */
#ifndef _SHAWRAPPER_H_
#define _SHAWRAPPER_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "translator/SHAwrapper/SHAwrapperImage.h"

/** @defgroup  dcplaya_shawrapper_devel  Image translator wrapper.
 *  @ingroup   dcplaya_translator_devel
 *  @brief     Image translator wrapper.
 *
 *  @author    benjamin gerard <ben@sashipa.com>
 */

/** Load an image from a file.
 *  @ingroup dcplaya_shawrapper_devel
 */
SHAwrapperImage_t * SHAwrapperLoadFile(const char *fname, int info_only);
  
/** Load an image from memory. */
SHAwrapperImage_t * SHAwrapperLoadMemory(const void *buffer, int size,
					 int info_only);
  
/** Add a new image translator.
 *  @ingroup dcplaya_shawrapper_devel
 */

int SHAwrapperAddTranslator(void * translator);

/** Remove an image translator.
 *  @ingroup dcplaya_shawrapper_devel
 */

int SHAwrapperDelTranslator(void * translator);

/** Blit an image block.
 *  @ingroup dcplaya_shawrapper_devel
 */

void SHAwrapperBlitz(void *dst, int dw, int dh, int dformat, int dmod,
		     const void *src, int sw, int sh, int sformat, int smod);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //#define _SHAWRAPPER_H_
