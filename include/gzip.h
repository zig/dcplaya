/**
 * @name     gzip.h
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @date     2002/09/20
 * @brief    Simple gzipped file access.
 *
 * $Id: gzip.h,v 1.1 2002-09-20 00:22:14 benjihan Exp $
 */

#ifndef _GZIP_H_
#define _GZIP_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** Load an optionnally gzipped file.
 *
 *    The gzip_load() function allocates memory and loads the totality of the
 *    given file. If the file is a gzipped file, it will be inflate.
 *
 * @param  fname  Name of file to load.
 * @param  ulen   Pointer to uncompressed or total size of file.
 *                May be set to 0.
 *
 * @return Pointer to the loaded file buffer.
 * @retval 0 Error
 */
void *gzip_load(const char *fname, int *ulen);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _GZIP_H_ */
