/**
 * @name    m3u.h
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/09/20
 * @brief   m3u playlist loader
 *
 * $Id: m3u.h,v 1.3 2002-09-20 13:42:41 benjihan Exp $
 */
#ifndef _M3U_H_
#define _M3U_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


/** @name M3U file access driver.
 *  @{
 */

/** m3u malloc function. */
typedef void *  (*M3Umalloc_f)(void *cookie, int bytes);

/** m3u free function. */
typedef void    (*M3Ufree_f)(void *cookie, void *data);

/** m3u read function. */
typedef int     (*M3Uread_f)(void *cookie, char *dst, int bytes);

/** m3u file access driver struct. */
typedef struct
{
  void * cookie;       /**< user data.        */
  M3Umalloc_f malloc;  /**< malloc function.  */
  M3Ufree_f   free;    /**< free function.    */
  M3Uread_f   read;    /**< read function.    */
} M3Udriver_t;

/**@}*/

/** m3u entry structure. */
typedef struct {
  char *path; /**< Music file location. */
  char *name; /**< Music name.          */
  int  time;  /**< Music time (sec).    */
} M3Uentry_t;

/** m3u list structure. */
typedef struct
{
  int sz;              /**< Max size of list. */
  int n;               /**< Number of entry.  */
  M3Uentry_t entry[1]; /**< Entry table.      */
} M3Ulist_t;

/** Set mu3 driver.
 *
 *    The M3Udriver() functions must be first call to set the memory and
 *    file access functions of the m3u loader.
 *
 * @return error-code
 * @retval  0   Success
 * @retval  <0  Failure
 */
int M3Udriver(M3Udriver_t * p_driver);

/** Create the m3u list.
 *
 *    The M3Uprocess() functions call the driver read function and creates
 *    an M3Ulist_t from read data.
 *
 * @return Loaded m3u list
 * @retval  0  Failure
 */ 
M3Ulist_t * M3Uprocess(void);

/** Kill an m3u list.
 *
 *    The M3Ukill() fuctions call the driver free function to kill the 
 *    m3u list.
 */
void M3Ukill(M3Ulist_t * l);

DCPLAYA_EXTERN_C_END

#endif /* #define _M3U_H_ */
