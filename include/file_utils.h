/**
 * @ingroup dcplaya_fu_devel
 * @file    file_utils.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   File manipulation utilities.
 *
 * $Id: file_utils.h,v 1.9 2003-03-22 00:35:26 ben Exp $
 */

#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_fu_devel File utils 
 *  @ingroup dcplaya_devel
 *  @brief   file utils API.
 *
 *    The file utils API is dcplaya high level API for handling file and
 *    directory operations.
 *
 *  @see      dcplaya_fn_devel
 *  @author  benjamin gerard <ben@sashipa.com>
 */

/** @name Error handling.
 *  @ingroup dcplaya_fu_devel
 *  @{
 */

/** file_utils returns code. */
enum fu_error_code_e {
  FU_INVALID_PARM  = -2,  /**< Invalid parameters.                    */
  FU_OPEN_ERROR    = -3,  /**< Open error.                            */
  FU_DIR_EXISTS    = -4,  /**< Destination is an existing  directory. */
  FU_FILE_EXISTS   = -5,  /**< Destination is an existing file.       */
  FU_READ_ERROR    = -6,  /**< Read error.                            */
  FU_WRITE_ERROR   = -7,  /**< Write error.                           */
  FU_CREATE_ERROR  = -8,  /**< File creation error.                   */
  FU_UNLINK_ERROR  = -9,  /**< Removing file error.                   */
  FU_ALLOC_ERROR   = -10, /**< Memory allocation error.               */

  FU_OK            = 0,   /**< Success.                               */
  FU_ERROR         = -1   /**< General error.                         */
};

/** Translate error code to error message (string).
 *
 *  int  err  fu_error_code_e error code to translate to string.
 *
 *  return  String corresponding to given error-code.
 */
const char * fu_strerr(int err);
/**@}*/

/** Directory entry. */
typedef struct _fu_dirent_s 
{
  char name[256];               /**< Filename. */
  int  size;                    /**< File size (-1 for directory). */
} fu_dirent_t;

/** @name File existence test functions. 
 *  @ingroup dcplaya_fu_devel
 *  @{
 */

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

/**@}*/

/** @name File functions.
 *  @ingroup dcplaya_fu_devel
 *  @{
 */

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

/** Get size of a file.
 *
 * @param  fname  Path of file.
 *
 * @return File size in bytes.
 * @retval <0 fu_error_code_e.
 */
int fu_size(const char * fname);

/**@}*/

/** @name  Directory functions.
 *  @ingroup dcplaya_fu_devel
 *  @{
 */

/** Directory reading filter function.
 *
 *    The fu_filter_f function is call by fu_read_dir() function for each
 *    scanned directory entry. The fu_filter_f function returns 0 if the entry
 *    is accepted.
 */
typedef int (*fu_filter_f)(const fu_dirent_t *);

/** Directory add entry function.
 *
 *    The fu_addentry_f function is call by the fu_read_dir_cb() function
 *    for each scanned directory entry. The fu_addentry_f function returns 1
 *    if the entry is accepted, 0 if the entry is rejected and a
 *    fu_error_code_e (minus value) in error. In that case, fu_read_dir_cb()
 *    will stop scanning the directory and returns with that error.
 */
typedef int (fu_addentry_f)(const fu_dirent_t *, void *cookie);

/** Create a new directory.
 *
 * @param  dirname  Path of directory to create.
 *
 * @return fu_error_code_e
 */
int fu_create_dir(const char *dirname);

/** Read directory content.
 *
 * @param  dirname  Path of directory to scan.
 * @param  res      Pointer to return fu_dirent_t table. Don't forget to free()
 *                  the return pointer when done with it. Returned value is
 *                  null in case of error or if there is no entry.
 * @param  filter   Filter function. 0 for default default filter function,
 *                  which accept all entrires.
 *
 * @return Number of entry in the fu_dirent_t table returned in res.
 * @retval <0  fu_error_code_e
 * @retval >=0 success, number of entry.
 */
int fu_read_dir(const char *dirname, fu_dirent_t **res, fu_filter_f filter);

/** Read directory content, callback version.
 *
 * @param  dirname  Path of directory to scan.
 * @param  addentry Callback function for each directory entry read.
 * @param  cookie   User parameter for the addentry function.
 *
 * @return Number of entry read and accepted by addentry function.
 * @retval <0  fu_error_code_e
 * @retval >=0 success, number of entry.
 */
int fu_read_dir_cb(const char *dirname, fu_addentry_f addentry, void * cookie);


/**@}*/

/** @name Directory sorting.
 *  @ingroup dcplaya_fu_devel
 *  @{
 */

/** Directory sorting function.
 *
 *    The fu_sortdir_f function must return an integer less than,
 *    equal  to,  or  greater than zero if the first argument is
 *    considered to be respectively  less than, equal to, or
 *    greater than the second.
 */
typedef int (*fu_sortdir_f)(const fu_dirent_t *a, const fu_dirent_t *b);

/** Directory comparison function for sorting by name, directory first. */
int fu_sortdir_by_name_dirfirst(const fu_dirent_t *a, const fu_dirent_t *b);

/** Directory comparison function for sorting by name. */
int fu_sortdir_by_name(const fu_dirent_t *a, const fu_dirent_t *b);

/** Directory comparison function for sorting by descending size,
    directory first. */
int fu_sortdir_by_descending_size(const fu_dirent_t *a, const fu_dirent_t *b);

/** Directory comparison function for sorting by ascending size,
    directory first. */
int fu_sortdir_by_ascending_size(const fu_dirent_t *a, const fu_dirent_t *b);

/** Sorts directory entries.
 *
 *     The contents of the array are sorted in ascending order according to
 *     a fu_sortdir_f comparison function pointed to by sortdir.
 *     If two members compare as equal, their order in the sorted array is
 *     undefined.
 *
 * @param  dir      Pointer to the directory entries to sort.  
 * @param  entries  Number of entries in the directory.    
 * @param  sortdir  The comparison function. 0 is the default comparison
 *                  function : fu_sortdir_by_name_dirfirst().
 *
 * @return fu_error_code_e
 */
int fu_sort_dir(fu_dirent_t *dir, int entries, fu_sortdir_f sortdir);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #define _FILE_UTILS_H_ */
