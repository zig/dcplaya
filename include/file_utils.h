/**
 * @file    file_utils.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   File manipulation utilities.
 *
 * $Id: file_utils.h,v 1.1 2002-09-30 20:03:02 benjihan Exp $
 */

#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** file_utils returns code. */
enum fu_error_code_e {
  FU_INVALID_PARM  = -2, /**< Invalid parameters.                    */
  FU_OPEN_ERROR    = -3, /**< Open error.                            */
  FU_DIR_EXISTS    = -4, /**< Destination is an existing  directory. */
  FU_FILE_EXISTS   = -5, /**< Destination is an existing file.       */
  FU_READ_ERROR    = -6, /**< Read error.                            */
  FU_WRITE_ERROR   = -7, /**< Write error.                           */
  FU_CREATE_ERROR  = -8, /**< File creation error.                   */
  FU_UNLINK_ERROR  = -9, /**< Removing file error.                   */

  FU_OK            = 0,  /**< Success.                               */
  FU_ERROR         = -1  /**< General error.                         */
};

/** Translate error code to error message (string).
 *
 *  int  err  fu_error_code_e error code to translate to string.
 *
 *  return  String corresponding to given error-code.
 */
const char * fu_strerr(int err);

/** Test if a file could be opened in a given mode.
 *
 *     The fu_open_test() function tries to open a file in a given mode,
 *     close it and returns success or failure.
 *
 * @param  fname  Name of file to open.
 * @param  mode   Open mode to test as in kos/fs.h
 *
 * @retval  0  File could not be opened in this mode.
 * @retval  1  File could ne opened in this mode.
 */
int fu_open_test(const char *fname, int mode);

/** Check if a file exist and is a regular file.
 *
 *    The fu_is_regular() function runs the fu_open_test() function with
 *    read-only mode to determine if it is a regular file.
 *
 * @param  fname  Name of file to check.
 *
 * @retval  0  File does not exist or is not a regular file.
 * @retval  1  File exists and is a regular file.
 */
int fu_is_regular(const char *fname);

/** Check if a file exist and is a directory.
 *
 *    The fu_is_dir() function runs the fu_open_test() function with
 *    read-only-dir mode to determine if it is a directory.
 *
 * @param  fname  Name of directory to check.
 *
 * @retval  0  File does not exist or is not a directory.
 * @retval  1  File exists and is a directory.
 */
int fu_is_dir(const char *fname);

/** Check if a file exist.
 *
 *    The fu_exist() function runs both fu_is_regular() and fu_is_dir()
 *    functions to determine if a file exist.
 *
 * @param  fname  Name of file or directory to check.
 *
 * @retval  0  File does not exist.
 * @retval  1  File exists.
 */
int fu_exist(const char *fname);

/** Remove a regular or empty diretory.
 *
 * @param  fname  Name of file or directory to remove.
 *
 * @return fu_error_code_e.
 */
int fu_remove(const char *fname);

/** Alias for fu_remove(). */
int fu_unlink(const char *fname);

/** Copy regular file to another regular file.
 *
 * @param  dstname  Target (destination) filename.
 * @param  srcname  Source (origine) filename.
 * @param  force    0:Do not overwrite existing file.
 *
 * @return fu_error_code_e or number of byte copied.
 * @retval <0   fu_error_code_e.
 * @retval >=0  number of byte copied.
 * 
 * @see fu_move()
 */
int fu_copy(const char * dstname, const char * srcname, int force);

/** Move regular file to another regular file.
 *
 * @param  dstname  Target (destination) filename.
 * @param  srcname  Source (origine) filename.
 * @param  force    0:Do not overwrite existing file.
 *
 * @return fu_error_code_e or number of byte moved.
 * @retval <0   fu_error_code_e.
 * @retval >=0  number of byte moved.
 *
 * @see fu_copy()
 * @see fu_remove()
 */
int fu_move(const char * dstname, const char * srcname, int force);

/** Create a new directory.
 *
 * @param  dirname  Path of directory to create.
 *
 * @return fu_error_code_e
 */
int fu_create_dir(const char *dirname);

DCPLAYA_EXTERN_C_END

#endif /* #define _FILE_UTILS_H_ */
