/**
 * @ingroup dcplaya_filetype_devel
 * @file    filetype.h
 * @author  benjamin gerard
 * @brief   Deal with filetypes and extensions.
 *
 * $Id: filetype.h,v 1.11 2003-03-26 23:02:47 ben Exp $
 */

#ifndef _FILETYPE_H_
#define _FILETYPE_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_filetype_devel Filetypes
 *  @ingroup dcplaya_devel
 *  @brief   filetype API.
 *
 *    The filetype API provides all functions to create filetype and retrieve
 *    type of a file form its name.
 *
 *    Filetypes are composed of two types:
 *     - @b Major type describes the genre of file.
 *     - @b Minor type describes the type of file in its genre.
 *
 *    Filetype are 16 bit numbers created with the @b minor number and the
 *    @b major number. The formula is major << 12 + minor.
 *
 *    The module is nitialized with some predefined special types:
 *     - filetype_root   : root directory
 *     - filetype_self   : the '.' directory
 *     - filetype_parent : the '..' directory
 *     - filetype_dir    : any non-special directory
 *     - filetype_file   : any non-special regular file
 *
 *  @author  benjamin gerard
 *  @{
 */

/** @name Predefined filetypes
 *  @{
 */

/** filetype for root directory (0x0000). */
extern const int filetype_root;

/** filetype for self directory '.' (0x0001). */
extern const int filetype_self;

/** filetype for parent directory '..' (0x0002). */
extern const int filetype_parent;

/** filetype for any directory '*' (0x0003). */
extern const int filetype_dir;

/** filetype for regular file (0x1000). */
extern const int filetype_file;

/** @} */

/** @name Filetype number access macro.
 *  @{
 */

/** Get major filetype number [0..15] from a filetype. */
#define FILETYPE_MAJOR_NUM(TYPE) (((TYPE)>>12)&15)

/** Get a FILETYPE without flags. */
#define FILETYPE(TYPE)    ((TYPE)&0xFFFF)

/**@}*/

/** Get major filetype from major name.
 *
 *  @param  name  Major name.
 *
 *  @return major filetype
 *  @retval -1 on error.
 */
int filetype_major(const char * name);

/** Get filetype from filetype (minor) name.
 *
 *  @param  name  filetype (minor) name.
 *  @param  type  Start search from this filetype. Minor filetype name is not a
 *                unique identifier. This parameter can be use to find other
 *                filetype with the same name.
 *
 *  @return filetype
 *  @retval -1 on error, filetype not found.
 */
int filetype_minor(const char * name, int type);

/** Get major name from filetype.
 *
 *  @param  type  filetype.
 *
 *  @return major name
 *  @retval  0 on undefined major type.
 */
const char * filetype_major_name(int type);

/** Get minor name from filetype.
 *
 *  @param  type  filetype.
 *
 *  @return minor name
 *  @retval  0 on undefined filetype.
 */
const char * filetype_minor_name(int type);

/** Get major and minor names from filetype.
 *
 *  @param  type   filetype.
 *  @param  major  Pointer to returned major name (or 0).
 *  @param  minor  Pointer to returned minor name (or 0).
 *
 *  @return error-code
 *  @retval  0   on success, defined filetype.
 *  @retval  -1  on error, undefined filetype.
 */
int filetype_names(int type, const char ** major, const char ** minor);

/** Create a new major filetype.
 *
 *    The filetype_major_add() funtion returns the filetype of a given major
 *    type name. If it does not exist, the function tries to create a new one.
 *
 *  @param  name  Major name to add.
 *
 *  @return filetype.
 *  @retval -1, on error (all major types in used).
 */
int filetype_major_add(const char * name);

/** Remove a major filetype.
 *
 * @see filetype_major_add()
 */
void filetype_major_del(int type);

/** Create a new filetype in a given major filetype.
 *  
 *    The filetype_add() function returns the filetype of a given named
 *    (minor) filetype in the given major filetype after remplacing the
 *    extensions list by the given one. If it does not exist, the function
 *    tries to create a new minor filetype in the given major type and returns
 *    it.
 *
 *  @param  major_type  Any filetype.
 *  @param  name        Name of the new filetype. If 0 is given, the first
 *                      extension in the extensions list is used without the
 *                      starting period '.'.
 *  @param  exts        Extensions list associates with the filetype.
 *                      Double '\0' terminated string. e.g. ".mp3\0.mp2\0"
 *
 *  @return filetype.
 *  @retval -1, on error (malloc error or all filetypes in used in for
 *                        this major type)
 */
int filetype_add(int major_type, const char * name, const char *exts);

/** Remove a filetype.
 *
 * @see filetype_add()
 */
void filetype_del(int type);

/** Get file type from filename extension and size.
 *
 * @param  fname  filename.
 * @param  size   Size of file in bytes. Must be set to -1 for directories.
 *
 * @return filetype
 * @retval -1, on error (invalid filename).
 *
 * @warning This function handles ".gz" as the secondary extension.
 */
int filetype_get(const char * fname, int size);

/** Get filetype for major type from 1 to 15 (regular files) from filename.
 *
 * @param  fname  filename
 *
 * @return filetype
 * @retval -1, on error (invalid filename).
 */
int filetype_regular(const char * fname);

/** Get filetype for major type 0 (directory) from a filename.
 *
 * @param  fname  filename
 *
 * @return filetype
 * @retval -1, on error (invalid filename).
 */
int filetype_directory(const char * fname);

/** Get filetype in given major type only.
 *
 * @param  fname       filename
 * @param  major_mask  Bit field of accepted major type. Use FILETYPE_MAJOR_NUM
 *                     macro to get the bit number of a given filetype (either
 *                     minor or major). 1 is used as a special value for
 *                     directory types.
 *
 * @return filetype
 * @retval -1, not found (invalid filename or filetype is not defined in the
 *             given major types).
 */
int filetype_get_filter(const char *fname, int major_mask);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FILETYPE_H_ */
