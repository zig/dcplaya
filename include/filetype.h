/**
 * @file    filetype.h
 * @author  ben(jamin) gerard <ben@sashipa.com> 
 * @brief   Deal with file types and extensions.
 *
 * $Id: filetype.h,v 1.6 2002-10-25 01:03:54 benjihan Exp $
 */

#ifndef _FILETYPE_H_
#define _FILETYPE_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @name File type definitions.
 *  @{
 */

#define FILETYPE_ERROR    0   /**< Reserved for error-code returns   */
#define FILETYPE_ROOT     1   /**< File is root directory            */
#define FILETYPE_SELF     2   /**< File is current directory (.)     */
#define FILETYPE_PARENT   3   /**< File os parent directory (..)     */
#define FILETYPE_DIR      16  /**< File is a directory               */
#define FILETYPE_LINK     64  /**< File is a symbolic links. Unused! */

#define FILETYPE_FILE     (256*1)   /**< First available regular filetype */
#define FILETYPE_UNKNOWN  FILETYPE_FILE /**< Unknown filetype. */

#define FILETYPE_EXE      (256*2)          /**< Executable file */
#define FILETYPE_ELF      (FILETYPE_EXE+1) /**< elf file */
#define FILETYPE_LEF      (FILETYPE_EXE+2) /**< lef file */

/** First available playlist filetype. */
#define FILETYPE_PLAYLIST (256*3)
#define FILETYPE_M3U      (FILETYPE_PLAYLIST+0) /**< mu3 playlist */
#define FILETYPE_PLS      (FILETYPE_PLAYLIST+1) /**< pls playlist */

/** First available playable (music) filetype. */
#define FILETYPE_PLAYABLE (256*4)

/** Gzipped file flag. */
#define FILETYPE_GZ       (1<<31)

/** Get a FILETYPE without flags. */
#define FILETYPE(TYPE)    ((TYPE)&~FILETYPE_GZ)

/*@}*/

/** Get file type from filename extension and size.
 *
 * @param  fname  Complete path, base-name or extension.
 * @param  size   Size of file in bytes. Must be set to -1 for directories.
 *
 * @return FILETYPE
 *
 * @warning This function handles .gz extension as is. It does not suit some
 *          others functions which need .gz to be ignored as an extension if
 *          there is a secondary extension in filename. e.g. .sid.gz
 */
int filetype_get(const char * fname, int size);

/** Get file type for regular file from filename extension.
 *
 * @param  fname  Complete path, base-name or extension.
 *
 * @return FILETYPE
 */
int filetype_regular(const char * fname);

/** Get file type for directory from filename.
 *
 * @param  fname  Complete path, base-name of directory.
 *
 * @return FILETYPE
 */
int filetype_dir(const char * fname);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FILETYPE_H_ */
