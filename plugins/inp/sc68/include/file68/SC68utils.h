/**
 * @ingroup   file68_devel
 * @file      SC68utils.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/03/08
 * @brief     Miscellaneous useful functions
 * @version   $Id: SC68utils.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 *
 * String, memory, file and others useful functions.
 *
 */

#ifndef _SC68UTILS_H_
#define _SC68UTILS_H_

#include <stdio.h>
#include "file_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @name  Endianess functions.
 *  @{
 */

/** Test for host machine processor little endianess */
int SC68utils_is_little_endian(void);

/** Test for host machine processor big endianess */
int SC68utils_is_big_endian(void);

/*@}*/


/** @name String functions.
 *  @{
 */

/** Compare two string (case insensitive).
 *
 *    The SC68utils_strcmp() function compares the two strings a and b,
 *    ignoring the case of the characters. It returns an integer less than,
 *    equal to, or greater than zero if a is found, respectively, to be less
 *    than, to match, or be greater than b.
 *
 *  @param  a  First string to compare
 *  @param  b  String to compare with
 *
 *  @return  Integer result of the two string compare. The difference
 *           between last tested characters of a and b.
 *  @retval  0   a and b are equal
 *  @retval  <0  a is less than b
 *  @retval  >0  a is greater than b
 */
int SC68utils_strcmp(char *a, char *b);

/** Concatenate two strings.
 *
 *    The SC68utils_strcat() function appends the b string to the a string
 *    overwriting the 0 character at the end of dest, and then adds a
 *    terminating 0 character. The strings may not overlap. Destination
 *    string has a maximum size of l characters. On overflow, the trailing 0
 *    is omitted.
 *
 *  @param  a  Destination string
 *  @param  b  String to append.
 *  @param  l  Destination maximum size (including trailing 0)
 *
 *  @return  Destination string
 *  @retval  a
 */
char *SC68utils_strcat(char *a, char *b, int l);

/** Make a track and time infornmation string.
 *
 *    The SC68utils_make_track_time_info() function formats a string with track time info.
 *    The general format is "TK MN:SS" where:
 *     - TK is track_num or "--" if track_num < 0 or track_num > 99
 *     - MN:SS is time minutes and seconds or "--:--" if seconds < 0
 *
 *  @param track_num  Track number from 00 to 99, other values disable.
 *  @param seconds    time to display in seconds [00..5999], other values
 *                    disable.
 *
 * @return  Pointer to result formatted string in a static buffer.
 *
 * @warning  The function returns a static buffer. Do try to modify it.
 * @warning  Not thread safe.
 *
 * @see SC68utils_make_big_playing_time()
 */
char *SC68utils_make_track_time_info(int track_num, int seconds);

/** Convert time in second to string.
 *
 *    The SC68utils_make_big_playing_time() function converts a time in
 *    seconds to days, hours, minutes and second string. Day and hour unit
 *    are removed if they are null (not signifiant). The output string looks
 *    like : [D days, ][H h, ] MN' SS"
 *
 *  @param  time  time in second to convert to string
 *
 *  @return  pointer to result time string in a static buffer.
 *
 * @warning  The function returns a static buffer. Do try to modify it.
 * @warning  Not thread safe.
 *
 * @see SC68utils_make_track_time_info()
 */
char *SC68utils_make_big_playing_time(int time);

/*@}*/

/** @name  Memory allocation functions.
 *  @{
 *     This functions are just a layer over Clib allocation functions. The
 *     only difference is that these functions handle SC68 error messages.
 *
 */

/** Alloc a memory buffer.
 *
 *    The SC68_malloc() allocates size bytes and returns a pointer to the
 *    allocated memory. The memory is not cleared. This function call
 *    standard C-lib malloc() function.
 *
 *  @param  nb  Number of bytes to allocate.
 *
 *  @return  Pointer to allocated memeory block.
 *  @retval  0  Memory allocation failure.
 *
 *  @see SC68_free()
 */
void *SC68_malloc(int nb);

/** Free a previously allocated memory buffer.
 *
 *    The SC68_free() function free a previously allocated memory block
 *    pointed by b. This memory block could have been allocated Clib
 *    standard malloc() family functions or SC68_malloc() function, or any
 *    other functions using Clib allocation.
 *
 *  @see SC68_malloc()
 */
int SC68_free(void *b);

/*@}*/


/** @name  File operation functions
 *  @{
 *     This functions are just a layer over Clib file functions. The only
 *     difference is that these functions handle SC68 error messages.
 *
 */

/** Open a file stream in given mode.
 *
 *    The SC68_fopen() function opens the file whose name is the string
 *    pointed to by name and associates a stream with it. The mode parameter
 *    is the same than in the Clib open() function.
 *
 *  @param  name  Pathname of the file to open
 *  @param  mode  Open mode {r|w|a}[+][b|t]
 *
 *  @return  pointer to Clib FILE structure
 *  @retval  0  Open failure
 *
 *  @see SC68_fclose()
 */
FILE *SC68_fopen(char *name, char *mode);

/** Close a file stream.
 *
 *     The SC68_fclose() functions close a file stream. All buffered data
 *     are flushed first.
 *
 *  @param  f  Pointer to FILE to close
 *
 *  @see SC68_fopen()
 */
int SC68_fclose(FILE *f);

/** Read a file stream into memory buffer.
 *
 *    The SC68_fread() function read nb bytes from an opened file stream f
 *    storing them at the location pointed by b.
 *
 *  @param  b   destination buffer
 *  @param  nb  number of byte to read
 *  @param  f   pointer to source FILE
 *
 *  @return error-code
 *  @retval  0  Success
 *  @retval <0  Failure
 *
 * @see SC68_fwrite()
 */
int SC68_fread(void *b, int nb, FILE *f);

/** Write a memory buffer to file stream.
 *
 *    The SC68_fwrite() function write nb bytes pointed by b to an opened
 *    file stream f.
 *
 *  @param  b   source buffer
 *  @param  nb  number of byte to write
 *  @param  f   pointer to destination FILE
 *
 *  @return error-code
 *  @retval  0  Success
 *  @retval <0  Failure
 *
 * @see SC68_fread()
 */
int SC68_fwrite(void *b, int nb, FILE *f);

/** Move file position indicator.
 *
 *    The SC68_fseek() function move the file stream f position indicator by
 *    adding offset to the current position indicator. * file stream f.
 *
 *  @param  f   pointer to FILE stream
 *  @param  offset  Number of bytes to move position indicator from.
 *
 *  @return error-code
 *  @retval  0  Success
 *  @retval <0  Failure
 *
 */
int SC68_fseek(FILE *f, int offset);

/** Get a file stream length.
 *
 *     The SC68_flen() function returns a opened file stream f length by
 *     using fseek() and ftell() functions.
 *
 *  @param  f  pointer to involved stream
 *
 *  @return  stream length in byte
 *  @retval  <0  Failure
 */
int SC68_flen(FILE *f);

/** Read a binary number from file stream.
 *
 *    The SC68_numberfread() function read a binary number of nbbytes size.
 *    Byte order depends on the bigendian boolean. The result is stored in
 *    the integer value pointed by pres if not NULL. For this reason nbbytes
 *    could not be greater than sizeof(int).
 *
 *  @param  pres        pointer to destination integer
 *  @param  nbbytes     number of byte of the number [1..sizeof(int)]
 *  @param  bigendian   number endian encoding [0:little other:big]
 *  @param  f           source file stream
 *
 *  @see SC68_numberfwrite()
 */
int SC68_numberfread(int *pres, int nbbytes, int bigendian, FILE *f);

/** Write a binary number to file stream.
 *
 *    The SC68_numberfwrite() function write val integer value as a binary
 *    number of nbbytes size into a file stream.  Byte order depends on the
 *    bigendian boolean.
 *
 *  @param  vals        value of integer number to write
 *  @param  nbbytes     number of byte to write [1..sizeof(int)-1]
 *  @param  bigendian   number endian encoding [0:little other:big]
 *  @param  f           destination file stream
 *
 *  @see SC68_numberfread()
 */
int SC68_numberfwrite(int val, int nbbytes, int bigendian, FILE *f) ;

/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68UTILS_H_ */
