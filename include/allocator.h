/**
 * @file     allocator.h
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/18
 * @brief    fast allocator for fixed size small buffer.
 * 
 * $Id: allocator.h,v 1.5 2003-01-24 04:28:13 ben Exp $
 */

#ifndef _ALLOCATOR_H_
#define _ALLOCATOR_H_

#define ALLOCATOR_MUTEX

#ifdef ALLOCATOR_SEM
# include <kos/sem.h>
#elif defined ALLOCATOR_MUTEX
# include "mutex.h"
#else
# include <arch/spinlock.h>
#endif

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
  const char *name;       /**< allocator name for debug purpose. */
#ifdef ALLOCATOR_SEM
  semaphore_t *sem;       /**< Access semaphore               */
#elif defined ALLOCATOR_MUTEX
  mutex_t mutex;          /**< recursive access mutex.        */ 
#else
  spinlock_t mutex;       /**< Mutex for thread safety.       */
#endif
  unsigned int elt_size;  /**< Size of an element.            */
  unsigned int elements;  /**< Number of elements.            */
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
allocator_t * allocator_create(int nmemb, int size, const char *name);

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
 * @return  Pointer to allocated memory.
 * @retval  0  Error
 *
 * @warning  The returned pointer must NOT be free() directly.
 *           Use allocator_free() instead.
 *
 * @see allocator_create()
 * @see allocator_free()
 * @see allocator_alloc_inside();
 */
void * allocator_alloc(allocator_t * a, unsigned int size);

/** Alloc an element inside the allcoator heap. 
 *
 *    The allocator_alloc_inside() function tries to alloc an element inside
 *    the allcoator heap.
 *
 * @return Pointer to allocated memory.
 * @retval 0  Error
 *
 * @see allocator_free()
 * @see allocator_alloc();
 */
void * allocator_alloc_inside(allocator_t * a);

/** Check for a memory location inside allocator heap.
 *
 *     The allocator_is_inside() function only checks if the data location is
 *     in the range of the allocator heap.
 *
 *     It does not check if the data location is a real allocated blocks.
 *
 *  @retval  1  data is in allocator heap.
 *  @retval  0  data is not in the allocator heap.
 */
int allocator_is_inside(const allocator_t * a, const void * data);

/** Get index of a memory location inside allocator heap.
 *
 *  @retval  -1   Memory is not in allocator heap.
 *  @retval  >=0  Index of this block in allocator heap.
 */
int allocator_index(const allocator_t * a, const void * data);

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

/** Count number of allocated (used) block in the allocator internal heap.
 *
 * @see allocator_count_free()
 */
int allocator_count_used(allocator_t * a);

/** Count number of free block in the allocator internal heap.
 *
 * @see allocator_count_used()
 */
int allocator_count_free(allocator_t * a);

/** Find a matching element in allocator used element list.
 *
 *     The allocator_match() calls the cmp() function successively for each
 *     element in used with data as first parameter and element's data as
 *     second parameter. If cmp() function returns 0 the elements data is 
 *     returned. If no matching element is found it returns 0.
 *
 * @return  Data of matching elements.
 * @retval  0  No matching element.
 */
void * allocator_match(allocator_t * a, const void * data,
					   int (*cmp)(const void *, const void *));

/** Dump allocator list content.
 */
void allocator_dump(allocator_t * a);

/** Lock the allocator. 
 *
 *    The allocator_lock() function locks the allocator list mutex.
 *
 *    This function does not need to be call before allocator_alloc() and 
 *    allocator_free() which are thread-safe. 
 *
 *    You may want call it to perform some statistics on the list or to stop
 *    any other thread that use this allcocator or whatever you want. It is
 *    given for allocator management purpose.
 *
 * @see allocator_unlock()
 */
void allocator_lock(allocator_t * a);

/** Unlock the allocator. 
 *
 * @see allocator_lock()
 */
void allocator_unlock(allocator_t * a);

#endif /* #define _ALLOCATOR_H_ */
