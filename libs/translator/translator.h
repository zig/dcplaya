/**
 *  @ingroup dcplaya_devel
 *  @file    translator.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/09/27
 *  @brief   Image translators.
 *  $Id: translator.h,v 1.5 2003-01-31 14:48:30 ben Exp $
 */

#ifndef _TRANSLATOR_H_
#define _TRANSLATOR_H_

/** Image translator.
 *  @ingroup dcplaya_devel
 */
typedef void * translator_t;

#include "translator/SHAwrapper/SHAwrapper.h"

/** @name Image functions.
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Load an image file.
 *  @param fname        filename of image to load.
 *  @param info_only    get image information only.
 *  @return Image.
 */
SHAwrapperImage_t * LoadImageFile(const char * fname, int info_only);

/** Load an image from memory.
 *  @param bufname      image file buffer.
 *  @param info_only    get image information only.
 *  @return Image.
 */
SHAwrapperImage_t * LoadImageMemory(const void *buffer, int size,
				    int info_only);

/** Add a new image translator. */
int AddTranslator(translator_t translator);

/** Remove an image translator. */
int DelTranslator(translator_t translator);

/** Blit an image block. */
void Blitz(void *dst, int dw, int dh, int dformat, int dmodulo,
		   const void *src, int sw, int sh, int sformat, int smodulo);

#endif /* #ifndef _TRANSLATOR_H_ */
