/**
 *  @ingroup dcplaya_iarray_container
 *  @file    iarray.h
 *  @author  benjamin gerard
 *  @date    2002/10/22
 *  @brief   Resizable array of indirect elements of any size.
 *
 *  $Id: iarray.h,v 1.6 2003-04-05 16:33:30 ben Exp $
 */

#ifndef _IARRAY_H_
#define _IARRAY_H_

#include "mutex.h"

/** @defgroup dcplaya_container Containers
 *  @ingroup  dcplaya_devel
 *  @brief    Thread safe container
 *  @author   benjamin gerard
 */

/** @defgroup dcplaya_iarray_container Indirect array
 *  @ingroup  dcplaya_container
 *  @date     2002/10/22
 *  @brief    resizable array of indirect elements of any size.
 *  @author   benjamin gerard
 *  @{
 */

/** User provided allocation function. */
typedef void * (*iarray_alloc_f)(unsigned int size, void *cookie);
/** User provided free function. */
typedef void (*iarray_free_f)(void * addr, void *cookie);
/** User provided compare function. */
typedef int (*iarray_cmp_f)(const void * a, const void * b, void * cookie);

/** Array element. */
typedef struct {
  int size;     /**< element size.    */
  void *addr;   /**< element address. */
} iarray_elt_t;

/** Array definition. */
typedef struct {
  int n;                 /**< Number of elenent stored.              */
  int max;               /**< Number of element allocated.           */
  iarray_elt_t * elt;    /**< Elements.                              */
  iarray_alloc_f alloc;  /**< Funtion to allocate a new element.     */
  iarray_free_f free;    /**< Funtion to free a element.             */
  void * cookie;         /**< User data given to alloc() and free(). */
  mutex_t mutex;         /**< Recursive mutex for thread safety.     */
  int elt_default_size;  /**< Default element size.                  */
} iarray_t;

/** Create and initialized. 
 *  @param  a      pointer to an iarray_t.
 *  @param  alloc  alloc function (0 default).
 *  @param  free   free function (0 default).
 *  @param  cookie user value to be giveb to alloc() and free()
 *  @return error-code
 *  retval 0 success
 */
int iarray_create(iarray_t *a,
		  iarray_alloc_f alloc, iarray_free_f free, void *cookie);
/** Destroy.
 *  @param  a  pointer to previously created iarray_t.
 */
void iarray_destroy(iarray_t *a);

/** Remove all elements.
 *  @param  a  iarray to clear
 *  @return error-code
 *  @retval success
 */
int iarray_clear(iarray_t *a);

void * iarray_addrof(iarray_t *a, int idx);
iarray_elt_t * iarray_eltof(iarray_t *a, int idx);

int iarray_get(iarray_t *a, int idx, void * elt, int eltsize);
int iarray_find(iarray_t *a, const void * what, iarray_cmp_f cmp,
		void * cookie);
iarray_elt_t * iarray_dup(iarray_t *a, int idx);
int iarray_set(iarray_t *a, int idx, void *elt, unsigned int eltsize);
int iarray_insert(iarray_t *a, int idx, void *elt, unsigned int eltsize);
int iarray_remove(iarray_t *a, int idx);

/** Lock the array for thread safe modification.
 *  @param  a  iarray to lock
 *  @see mutex_lock();
 */
void iarray_lock(iarray_t *a);

/** Unlock the array for thread safe modification.
 *  @param  a  iarray to unlock
 *  @see mutex_unlock();
 */
void iarray_unlock(iarray_t *a);

/** Test if the array is lockable.
 *  @param  a  iarray to (virtually) lock
 *  @see mutex_trylock();
 */
int iarray_trylock(iarray_t *a);

/** Get current lock count.
 *  @param  a  iarray
 *  @see mutex_lockcount();
 */
int iarray_lockcount(const iarray_t *a);

/** Shuffle a partition of the array.
 *
 *  @param  a    iarray to shuffle
 *  @param  idx  Index of first element to shuffle (must be valid).
 *  @param  n    Number of element (clipped to max).
 * 
 */
void iarray_shuffle(iarray_t *a, int idx, int n);

/** Sort the whole array.
 *  @param  a       iarray to sort
 *  @param  cmp     compare function
 *  @param  cookie  cookie for cmp function
 */
void iarray_sort(iarray_t *a, iarray_cmp_f cmp, void * cookie);

/** Sort a partition of an array.
 *  @param  a    iarray to sort
 *  @param  idx  Index of first (clipped)
 *  @param  n    Number of element (clipped to max).
 *  @param  cmp  compare function.
 *  @param  cookie  cookie for cmp function
 */
void iarray_sort_part(iarray_t *a, int idx, int n, iarray_cmp_f cmp,
		      void * cookie);

/**@}*/

#endif /* #define _IARRAY_H_ */
