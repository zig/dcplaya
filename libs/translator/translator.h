/**
 *  @file    translator.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/09/27
 *  @brief   Image translators.
 *  $Id: translator.h,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */
#ifndef _translator_h_
#define _translator_h_

#include "SHAwrapper/SHAwrapper.h"

SHAwrapperImage_t * LoadImageFile(const char * fname);
SHAwrapperImage_t * LoadImageMemory(void *buffer, int size);

#endif /* #ifndef _translator_h_ */
