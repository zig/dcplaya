/**
 * @ingroup  dcplaya_devel
 * @name     gzip.h
 * @author   benjamin gerard
 * @date     2002/09/20
 * @brief    Simple gzipped file access.
 *
 * $Id: gzip.h,v 1.4 2003-03-23 23:54:54 ben Exp $
 */

#ifndef _GZIP_H_
#define _GZIP_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @name gzip file functions.
 *  @ingroup  dcplaya_devel
 *  @{
 */

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

/** Save binary buffer as a gzip file.
 *
 *    The gzip_save() function simply creates a gzip file from a given 
 *    memory buffer.
 *
 * @param  fname  Name of file to save.
 * @param  buffer Pointer to data to compress and save.
 * @param  len    Number of uncompressed bytes in buffer.
 *
 * @return Size of compressed file.
 * @retval -1  Failure.
 * @retval >=0 Success.
 */
int gzip_save(const char *fname, const void * buffer, int len);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _GZIP_H_ */
