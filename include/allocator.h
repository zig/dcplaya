/**
 * @file     allocator.h
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/18
 * @brief    fast allocator for fixed size small buffer.
 * 
 * $Id: allocator.h,v 1.1 2002-10-18 11:39:21 benjihan Exp $
 */

#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#include <arch/spinlock.h>

/** Allocator elements.
 *
 *    Real object is stored just after it. Notice than an allocator_elt_t
 *    size is 16 bytes so that the real address keep a 16 bytes alignment.
 *
 */
typedef struct _allocator_elt_s {
  struct _allocator_elt_s * next; /**< Pointer to next element in list.      */
  struct _allocator_elt_s * prev; /**< Pointer to previous elements in list. */
  int align[2];                   /**< To keep 16 aligment.                  */
} allocator_elt_t;

/** Allocator type.
 *
 *    This object MUST be created by the allocator_create() function and
 *    destroyed by the allocator_destroy() function.
 *
 *    The allocator object allocates its buffer in its heap block. This 
 *    means there is only one malloc(). For alignment requirement, the 
 *    buffer field is aligned to a 16 bytes boundary. That is why the
 *    value returned by allocator_create() is not a valid heap block for
 *    free(). The valid heap address is stored in the realaddress field.
 *
 * @warning Do NOT free() an allocator_t directly. The pointer is not a
 *          valid heap block.
 */
typedef struct {
  void * realaddress;     /**< Address of big alloc.          */
  allocator_elt_t * used; /**< List of allocated elements.    */
  allocator_elt_t * free; /**< List of free elements.         */
  spinlock_t mutex;       /**< Mutex for thread safety.       */
  int elt_size;           /**< Size of an element.            */
  char * bufend;          /**< Pointer to this end of buffer. */
  char buffer[16];        /**< Start of elements buffer.      */
} allocator_t;

/** Create an allocator.
 *
 * @param  nmemb  Number of elements preallocated.
 * @param  size   Size of pre-allocated elements.
 *
 * @return pointer to allocator.
 * @retval 0  Error
 *
 * @warning  The returned value must not be free() directly.
 *           Use allocator_destroy() instead.
 *
 * @see allocator_destroy()
 */
allocator_t * allocator_create(int nmemb, int size);

/** Destroy an allocator.
 *
 *    The allocator_destroy() function free the allocator buffer. It does not
 *    perform any test on in-used elements.
 *
 * @param  a  Pointer to allocator to destroy.
 *
 * @see allocator_create()
 */
void allocator_destroy(allocator_t * a);

/** Alloc an element via an allocator.
 *
 *    The allocator_alloc() function tries to alloc an element of a given size
 *    in its pre-allocated buffer. This is possible only this the requested
 *    size is less or equal to the allocator element size given at allocator
 *    creation and if there is a free element. If both these conditions are
 *    not completed the allocator_alloc() calls and returns malloc() functions.
 *
 *    The allocator_alloc() function is thread safe.
 *
 * @return  Pointer to allcoated memory.
 * @retval  0  Error
 *
 * @warning  The returned pointer must NOT be free() directly.
 *           Use allocator_free() instead.
 *
 * @see allocator_create()
 * @see allocator_free()
 */
void * allocator_alloc(allocator_t * a, int size);

/** Free memory via an allocator.
 *
 *    The allocator_free() functions tests if the data memory is part of its
 *    own. If it is not, the memory is freed by the free() function.
 *    It is safe to use this function with a valid malloc() pointer even it
 *    has not been allocated throught this allocator (or another).
 *
 *    This function aims to be fast that is why data which is part of the
 *    allocator must be valid ! In DEBUG mode, the function performs more
 *    validity checks such as looking for the block in both used and free
 *    lists.
 *
 *    The allocator_free() function is thread safe.
 */
void allocator_free(allocator_t * a, void * data);

#endif /* #define _ALLOCATOR_H_ */
