/**
 *  @file    translator.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/09/27
 *  @brief   Image translators.
 *  $Id: translator.h,v 1.2 2002-11-25 20:31:39 ben Exp $
 */
#ifndef _translator_h_
#define _translator_h_

#include "translator/SHAwrapper/SHAwrapper.h"

SHAwrapperImage_t * LoadImageFile(const char * fname);
SHAwrapperImage_t * LoadImageMemory(void *buffer, int size);

#endif /* #ifndef _translator_h_ */
