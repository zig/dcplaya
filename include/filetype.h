/**
 * @file    filetype.h
 * @author  ben(jamin) gerard <ben@sashipa.com> 
 * @brief   Deal with file types and extensions.
 *
 * $Id: filetype.h,v 1.8 2002-12-09 16:26:49 ben Exp $
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

/** First available regular filetype. */
#define FILETYPE_FILE     (256*1)
#define FILETYPE_UNKNOWN  FILETYPE_FILE /**< Unknown filetype. */

/** First available executable filetype (plugins). */
#define FILETYPE_EXE      (256*2)          
#define FILETYPE_ELF      (FILETYPE_EXE+1) /**< elf file */
#define FILETYPE_LEF      (FILETYPE_EXE+2) /**< lef file */
#define FILETYPE_EXE_LAST (256*3-1)        /**< Last executable filetype */

/** First available playlist filetype. */
#define FILETYPE_PLAYLIST (256*3)
#define FILETYPE_M3U      (FILETYPE_PLAYLIST+0) /**< mu3 playlist */
#define FILETYPE_PLS      (FILETYPE_PLAYLIST+1) /**< pls playlist */
#define FILETYPE_PLAYLIST_LAST (256*4-1)        /**< Last playlist filetype */

/** First available image filetype. */
#define FILETYPE_IMAGE    (256*4)
#define FILETYPE_TGA      (FILETYPE_IMAGE+0)    /**< tga image */
#define FILETYPE_JPG      (FILETYPE_IMAGE+1)    /**< jpg image */
#define FILETYPE_GIF      (FILETYPE_IMAGE+2)    /**< gif image */
#define FILETYPE_TIF      (FILETYPE_IMAGE+3)    /**< tif image */
#define FILETYPE_BMP      (FILETYPE_IMAGE+4)    /**< bmp image */
#define FILETYPE_PNM      (FILETYPE_IMAGE+5)    /**< pnm image */
#define FILETYPE_PNG      (FILETYPE_IMAGE+6)    /**< png image */
#define FILETYPE_PPM      (FILETYPE_IMAGE+7)    /**< ppm image */
#define FILETYPE_XPM      (FILETYPE_IMAGE+8)    /**< xpm image */
#define FILETYPE_IMAGE_LAST (256*5-1)           /**< Last image filetype */

/** First available playable (music) filetype. */
#define FILETYPE_PLAYABLE      (256*8)
#define FILETYPE_PLAYABLE_LAST (256*9-1)        /**< Last playable filetype */

/** Gzipped file flag. */
#define FILETYPE_GZ       (1<<16)

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

/** Add a new "playable" type.
 *
 * @return FILETYPE
 */
int filetype_add(const char *exts);

/** Remove a "playable" type.
 * @see filetype_add()
 */
void filetype_del(int type);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FILETYPE_H_ */
