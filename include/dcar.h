/**
 * @ingroup   dcplaya_dcar_devel
 * @file      dcar.h
 * @author    benjamin gerard <ben@sashipa.com>
 * @date      2002/09/21
 * @brief     dcplaya archive.
 *
 * $Id: dcar.h,v 1.5 2003-03-22 00:35:26 ben Exp $
 *
 */

#ifndef _DCAR_H_
#define _DCAR_H_

#include "extern_def.h"
#include <kos/fs.h> /* for dirent_t */

DCPLAYA_EXTERN_C_START

/** @defgroup  dcplaya_dcar_devel  dcar archiver
 *  @ingroup   dcplaya_devel
 *  @brief     dcar archiver
 *
 *    dcar is a tar-like file archiver. Like tar the archive could be
 *    compressed with gzip. The whole archive file is compressed.
 *
 *    dcar is used for dcplaya vmu files. 
 *
 *    @b limitations: filename are limited to 32 characters.
 *
 *  @warning   Architecture dependent code.
 *  @author    benjamin gerard <ben@sashipa.com>
 */

/** dcplaya filter function return codes.
 *  @ingroup dcplaya_dcar_devel
 */
typedef enum {
  DCAR_FILTER_ACCEPT = 0, /**< Entry is accepted.                       */
  DCAR_FILTER_REJECT,     /**< Entry is rejected.                       */
  DCAR_FILTER_BREAK,      /**< Entry is rejected, stop archiving.       */
  DCAR_FILTER_ERROR = -1  /**< Entry is rejected, archive is discarded. */
} dcar_filter_e;

/** dcplaya archive filter function.
 *  @ingroup dcplaya_dcar_devel
 */
typedef dcar_filter_e (*dcar_filter_f)(const dirent_t *de, int level);

/** dcplaya archive tree entry.
 *  @ingroup dcplaya_dcar_devel
 */
typedef struct {
  char name[32];    /**< entry name, not neccessary 0 terminated. */
  struct {
    unsigned int size : 30;
    unsigned int dir  : 1;
    unsigned int end  : 1;
  } attr;
} dcar_tree_entry_t;

/** dcplaya archive tree.
 *  @ingroup dcplaya_dcar_devel
 */
typedef struct {
  union {
    char magic[4];        /**< Magic "DCAR".              */
    char * data;          /**< Pointer to the file data.  */
  };
  int n;                  /**< Number of dcar entry.      */
  dcar_tree_entry_t e[1]; /**< Table of all dcar entries. */
} dcar_tree_t;

/** dcplaya archive option.
 *  @ingroup dcplaya_dcar_devel
 */
typedef struct {
  
  /** dcplaya archive user option. */
  struct {
    int verbose;            /**< Display archive entry while processing.    */
    dcar_filter_f filter;   /**< Filter function to use. 0 for default.     */
    int compress;           /**< Compress level [0..9].                     */
    int skip;               /**< Number of byte to skip at start of file.   */
  } in;

  /** dcplaya archive returned info. */
  struct {
    int level;              /**< Returned subdirectory level.               */
    int entries;            /**< Returned number of entries.                */
    int bytes;              /**< Returned number of bytes for entries data. */
    int cbytes;             /**< Returned size of compressed archives.      */
    int ubytes;             /**< Returned size of uncompressed archives.    */
  } out;

  /** dcplaya archive internal used to avoid stack overflow. */
  struct {
    int level;              /**< Internal: current recursion level.         */
    char * path;            /**< Internal: current path.                    */
    char * tmp;             /**< Internal: temporary data buffer for I/O.   */
    int max;                /**< Internal: size of tmp buffer.              */
  } internal;

  const char * errstr;      /**< Error string. */
  
} dcar_option_t;

/** @name dcar functions.
 *  @ingroup dcplaya_dcar_devel
 *  @{
 */

/** Default dcplaya filter function.
 *
 *    The default_filter() function accepts all files but special directory
 *    entries such as root '/', parent '..' or current '.'. This function
 *    calls the filetype_get() function to determine file type.
 *    May be call by user provided filter function.
 *
 *  @param  de     Directory entry to filter. Must be non null.
 *  @param  level  Current recursion level (ignored here).
 *
 *  @return filter function return code.
 *  @retval DCAR_FILTER_ACCEPT entry accepted (for all valid entries) 
 *  @retval DCAR_FILTER_REJECT entry rejected (root/parent/current directory)
 *  @retval DCAR_FILTER_ERROR  entry error (null pointer).
 */
dcar_filter_e dcar_default_filter(const dirent_t *de, int level);

/** Set default options.
 *
 *    The dcar_default_option() function setup the given dcar_option_t
 *    with dcar default parameters.
 *
 *  @param  opt  Pointer to option structure to setup.
 */
void dcar_default_option(dcar_option_t * opt);

/** Simulate archive creation.
 *
 *    The dcar_simulate() function simulates an archive creation. It does not
 *    calculate the compressed nor uncompressed archive size. 
 *
 *  @param  path  Directory to archive 
 *  @param  opt   Pointer to dcar option (0 for default)
 *
 *  @return number of file archived.
 *  @retval  >=0  Success.
 *  @retval   <0  Failure.
 */
int dcar_simulate(const char *path, dcar_option_t * opt);

/** Create an archive.
 *
 *    The dcar_create() function creates a new archive from a given path.
 *
 *  @param  name  Name of archive file to create.
 *  @param  path  Directory to archive
 *  @param  opt   Pointer to dcar option (0 for default)
 *
 *  @return number of file archived.
 *  @retval  >=0  Success.
 *  @retval   <0  Failure.
 */
int dcar_archive(const char *name, const char *path, dcar_option_t * opt);

/** Extract an archive.
 *
 *    The dcar_extract() function extracts an archive file to a given path.
 *
 *  @param  name  Name of archive file to extract.
 *  @param  path  Extract location. 
 *  @param  opt   Pointer to dcar option (0 for default)
 *
 *  @return number of file extracted
 *  @retval  >=0  Success.
 *  @retval   <0  Failure.
 **/
int dcar_extract(const char *name, const char *path, dcar_option_t *opt);

/*@}*/

DCPLAYA_EXTERN_C_END

#endif /* #define _DCAR_H_ */
