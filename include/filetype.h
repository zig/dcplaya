/** 
 * dreammp3 - File type
 *
 * (c) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: filetype.h,v 1.1.1.1 2002-08-26 14:15:00 ben Exp $
 */

#ifndef _FILETYPE_H_
#define _FILETYPE_H_

#define FILETYPE_ERROR    0
#define FILETYPE_ROOT     1
#define FILETYPE_SELF     2
#define FILETYPE_PARENT   3
#define FILETYPE_DIR      16
#define FILETYPE_LINK     64

#define FILETYPE_FILE     (256*1)
#define FILETYPE_UNKNOWN  FILETYPE_FILE

#define FILETYPE_EXE      (256*2)
#define FILETYPE_ELF      (FILETYPE_EXE+1)
#define FILETYPE_LEF      (FILETYPE_EXE+2)

#define FILETYPE_PLAYLIST (256*3)
#define FILETYPE_M3U      (FILETYPE_PLAYLIST+0)
#define FILETYPE_PLS      (FILETYPE_PLAYLIST+1)

#define FILETYPE_PLAYABLE (256*4)
#define FILETYPE_MP3      (FILETYPE_PLAYABLE+0) 

/** Get file type from filename extension and size.
 *
 * @param  fname  Complete path, base-name or extension.
 * @param  size   Size of file in bytes. Must be set to -1 for directories.
 *
 * @return FILETYPE
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

/** Get extension from file path.
 *
 * @param  fname  Complete path, base-name of directory.
 *
 * @return Pointer to extension in fname.
 * @retval [0]=='.'  Extension found.
 + @retval [0]==0    Extension not found.
 */
const char *filetype_ext(const char *fname);

#endif
