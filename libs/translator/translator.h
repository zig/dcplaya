/**
 *  @ingroup dcplaya_devel
 *  @file    translator.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/09/27
 *  @brief   Image translators.
 *  $Id: translator.h,v 1.3 2002-12-15 16:15:03 ben Exp $
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

/** Load an image file. */
SHAwrapperImage_t * LoadImageFile(const char * fname);

/** Load an image from memory. */
SHAwrapperImage_t * LoadImageMemory(void *buffer, int size);

/** Add a new image translator. */
int AddTranslator(translator_t translator);

/** Remove an image translator. */
int DelTranslator(translator_t translator);

#endif /* #ifndef _TRANSLATOR_H_ */
