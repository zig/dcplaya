/**
 *  @file   iarray.h
 *  @author benjamin gerard <ben@sashipa.com>
 *  @date   2002/10/22
 *  @brief  Resizable array of indirect elements of any size.
 *
 *  $Id: iarray.h,v 1.2 2002-10-23 02:09:04 benjihan Exp $
 */

#ifndef _IARRAY_H_
#define _IARRAY_H_

#include "mutex.h"

typedef void * (*iarray_alloc_f)(unsigned int size, void *cookie);
typedef void (*iarray_free_f)(void * addr, void *cookie);
typedef int (*iarray_sort_f)(const void * a, const void * b);

typedef struct {
  int size;
  void *addr;
} iarray_elt_t;

typedef struct {
  int n;
  int max;
  iarray_elt_t * elt;
  iarray_alloc_f alloc;
  iarray_free_f free;
  void * cookie;
  mutex_t mutex;
  int elt_default_size;
} iarray_t;

int iarray_create(iarray_t *a,
				  iarray_alloc_f alloc, iarray_free_f free, void *cookie);
void iarray_destroy(iarray_t *a);

void * iarray_addrof(iarray_t *a, int idx);
int iarray_get(iarray_t *a, int idx, void * elt, int eltsize);
iarray_elt_t * iarray_dup(iarray_t *a, int idx, void * elt, int eltsize);
int iarray_insert(iarray_t *a, int idx, void *elt, unsigned int eltsize);
int iarray_remove(iarray_t *a, int idx);

void iarray_lock(iarray_t *a);
void iarray_unlock(iarray_t *a);

void iarray_shuffle(iarray_t *a, int idx, int n);
void iarray_sort(iarray_t *a, iarray_sort_f cmp);
void iarray_sort_part(iarray_t *a, int idx, int n, iarray_sort_f cmp);

#endif /* #define _IARRAY_H_ */
