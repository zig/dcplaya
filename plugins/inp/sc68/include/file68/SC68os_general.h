/**
 * @ingroup   file68_devel
 * @file      SC68os_general.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/05/11
 * @brief     sc68 general purpose OS-DEPENDANT function definitions.
 @ version    $id$
 *
 *   The implementation of these functions depends on the type of operating
 *   system. Most of them are used for file path operations. sc68 tries to
 *   handle POSIX path.
 *
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _SC68OS_GENERAL_H_
#define _SC68OS_GENERAL_H_

#include "config.h"
#include <stdio.h>
#include "file68/SC68rsc.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Formatted debug output.
 *
 *    The SC68os_pdebug() function formats and outputs a formatted stream to
 *    debug stream. The format parameter is a printf() format string. Debug
 *    stream is defined as follow
 *    - Unix stderr
 *    - Windows debug stream (use Microsoft OutputDebugStream() function)
 *
 */
//#ifndef DREAMCAST68 
void SC68os_pdebug(char *format, ... );
//#else
//# define SC68os_pdebug printf
//#endif

/** @name  SC68 resource file functions.
  * @{
  */

/** Make an OS compatible filename for a resource.
 *
 *    The SC68os_resource_name() function makes a full pathname for a
 *    resource file defined by a simple name fname and a resource type rsc.
 *    In most case, this function adds path to SC68 resource directory and
 *    appends a file extension. But it could differ for some Operating
 *    System.
 *
 *  @param  rsc    resource file type
 *  @param  fname  string to a leaf name (without path nor extension)
 *
 *  @return  pointer to a static string containing full path to resource file
 *  @retval  0  failure
 *
 *  @warning  returned string is a static buffer. Do not modify it.
 *  @warning  not thread safe.
 *
 *  @see SC68os_open_resource()
 */
char *SC68os_resource_name(SC68rsc_t rsc, char *fname);

/** Open a resource file.
 *
 *    The SC68os_open_resource() function opens a resource file for a
 *    resource file defined by a simple name fname and a resource type rsc.
 *
 *  @param  rsc    resource file type
 *  @param  fname  string to a leaf name (without path nor extension)
 *
 *  @return  pointer to a FILE structure
 *  @retval  0  failure
 *
 *  @see SC68os_resource_name()
 */
FILE *SC68os_open_resource(SC68rsc_t rsc, char *fname, char *openmode);

/*@}*/


/** @name  Filename functions.
 *  @{
 */

/** Print POSIX compatible filename to file stream.
 *
 *    The SC68os_fput_fname() function performs a POSIX conversion of a file
 *    pathname fname and prints the result in the destination strwam f.
 *    Conversion is done in temporary buffer and fname remains unchanged.
 *
 *  @param  f       pointer to destination file stream
 *  @param  fname   pointer to file pathname string
 *
 *  @return error-code
 *  @retval  0  success
 *  @retval <0  failure
 *
 * @see SC68os_os2posix_fname()
 */
int SC68os_fput_fname( FILE *f, char *fname );

/** Convert file pathname from OS to POSIX.
 *
 *    The SC68os_os2posix_fname() function perform all necessary conversion
 *    to transform a OS compatible filename to POSIX. The most important
 *    rule consists on using slash '/' character as directory separator.
 *
 *  @param  fname  pointer to string to convert.
 *
 *  @return pointer to converted string (should be fname)
 #  @retval  0  failure
 *
 *  @see SC68os_posix2os_fname()
 */
char *SC68os_os2posix_fname(char *filename);

/** Convert file pathname from POSIX to OS.
 *
 *    The SC68os_posix2os_fname() function perform all necessary conversion
 *    to transform a POSIX compatible filename to OS.
 *
 *  @param  fname  pointer to string to convert.
 *
 *  @return pointer to converted string (should be fname)
 #  @retval  0  failure
 *
 *  @see SC68os_os2posix_fname()
 */
char *SC68os_posix2os_fname(char *filename);

/** Create absolute file pathname.
 *
 *    The SC68os_absolute_fname() function make an absolute file pathname
 *    from filename relative to current directory. This function removes all
 *    . or .. and handles correctly Windows drive and network filename.
 *
 *  @param  filename  string with file pathname relative to current
 *                    directory
 *
 *  @return  pointer to absolute file pathname
 *  @retval 0  failure
 */
char *SC68os_absolute_fname(char *filename);

/* Initialize SC68 resource path.
 *
 *   The SC68os_get_exec_path() function initializes SC68 resource path from
 *   executable (binary) file path. Typically argv[0] must be given for the
 *   path argument. This function is not useful for Unix version since
 *   resources are located by $HOME environment variable.
 *
 *  @return error-code
 *  @retval  0  success
 *  @retval <0  failure
 */
int SC68os_get_exec_path(char *path);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68OS_GENERAL_H_ */
