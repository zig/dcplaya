/**
 * @file      SHAwrapper.h
 * @brief     SHAtranslator "C" wrapper
 * @date      2002/09/27
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAwrapper.h,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */
#ifndef _SHAWRAPPER_H_
#define _SHAWRAPPER_H_

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include "SHAwrapper/SHAwrapperImage.h"

/** Load an image from a file. */
SHAwrapperImage_t * SHAwrapperLoadFile(const char *fname);

/** Load an image from memory */
SHAwrapperImage_t * SHAwrapperLoadMemory(void *buffer, int size);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //#define _SHAWRAPPER_H_
