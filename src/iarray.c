/**
 *  @file   iarray.c
 *  @author benjamin gerard <ben@sashipa.com>
 *  @date   2002/10/22
 *  @brief  Resizable array of indirect elements of any size.
 */

#include <stdlib.h>

#include "iarray.h"

void iarray_lock(iarray_t *a)
{
  spinlock_lock(&a->mutex);
}

void iarray_unlock(iarray_t *a)
{
  spinlock_unlock(&a->mutex);
}


static void * iarray_alloc(unsigned int size, void * cookie)
{
  cookie = cookie;
  return malloc(size);
}

static void iarray_free(void * addr, void * cookie)
{
  cookie = cookie;
  if (addr) {
	free(addr);
  }
}

int iarray_create(iarray_t *a,
				  iarray_alloc_f alloc, iarray_free_f free, void *cookie)
{
  if (!a) {
	return -1;
  }
  a->n = 0;
  a->max = 0;
  a->elt = 0;
  a->alloc = alloc ? alloc : iarray_alloc;
  a->free  = free  ? free  : iarray_free;
  a->cookie = cookie;
  spinlock_init(&a->mutex);
  return 0;
}

void iarray_destroy(iarray_t *a)
{
  if (a->elt) {
	int i;
	for (i=0; i<a->n; ++i) {
	  if (a->elt[i].addr) {
		a->free(a->elt[i].addr, a->cookie);
		a->elt[i].addr = 0;
		a->elt[i].size = 0;
	  }
	}
	free(a->elt);
	a->elt = 0;
  }
  a->n = a->max = 0;
}

void * iarray_addrof(iarray_t *a, int idx)
{
  void * addr = 0;
  iarray_lock(a);
  if ((unsigned int)idx < a->n) {
	addr = a->elt[idx].addr;
  }
  iarray_unlock(a);
  return addr;
}

int iarray_get(iarray_t *a, int idx, void * elt, int maxsize)
{
  iarray_lock(a);
  if ((unsigned int)idx < a->n) {
	iarray_elt_t * e = a->elt + idx;
	idx = -2;
	if (e->size <= maxsize) {
	  idx = 0;
	  maxsize = e->size;
	}
	memcpy(elt, a->elt[idx].addr, maxsize);
  } else {
	idx = -1;
  }
  iarray_unlock(a);
  return idx;
}

iarray_elt_t * iarray_dup(iarray_t *a, int idx, void * elt, int eltsize)
{
  iarray_elt_t * addr = 0;
  iarray_lock(a);
  if ((unsigned int)idx < a->n) {
	iarray_elt_t * e = a->elt + idx;
	addr = a->alloc( sizeof(*addr) + e->size, a->cookie);
	if (addr) {
	  addr->size = e->size;
	  addr->addr = addr+1;
	  memcpy(addr+1,  e->addr, e->size);
	}
  }
  iarray_unlock(a);
  return addr;
}

static void resize(iarray_t * a, int newsize)
{
  void * b;

  if (newsize < a->n) {
	int i;
	for (i=newsize; i<a->n; ++i) {
	  if (a->elt[i].addr) {
		a->free(a->elt[i].addr, a->cookie);
	  }
	}
  }

  b = realloc(a->elt, newsize * sizeof(*a->elt));
  if (b) {
	a->max = newsize;
	a->elt = b;
  } else if (newsize < a->n) {
	/* $$$ Safety net. Reducing a buffer should always work ! */
	a->n = newsize;
  }
}

int iarray_insert(iarray_t *a, int idx, void *elt, unsigned int eltsize)
{
  void * addr = 0;
  iarray_elt_t * e, * end;

  iarray_lock(a);
  if (idx < 0) {
	idx = a->n + idx + 1;
  }
  if ((unsigned int)idx > a->n) {
	goto error;
  }
  addr = a->alloc(eltsize, a->cookie);
  if (!addr) {
	goto error;
  }
  if (idx == a->max) {
	resize(a, a->max ? a->max * 2 : 256);
  }
  if ((unsigned int)idx >= a->n) {
	goto error;
  }
  memcpy(addr, elt, eltsize);
  
  for (e = a->elt+idx, end = a->elt+a->n-1; end >= e; --end) {
	end[1] = end[0];
  }
  e->addr = addr;
  e->size = eltsize;
  ++a->n;
  iarray_unlock(a);
  return idx;

 error:
  if (addr) {
	a->free(addr, a->cookie);
  }
  iarray_unlock(a);
  return -1;
}

int iarray_remove(iarray_t *a, int idx)
{
  iarray_lock(a);
  if ((unsigned int)idx < a->n) {
	iarray_elt_t * e = a->elt + idx, *end;
	if (e->addr) {
	  a->free(e->addr, a->cookie);
	}
	for (end = a->elt + a->n-1; e < end; ++e) {
	  e[0] = e[1];
	}
	if (end->addr) {
	  a->free(end->addr, a->cookie);
	}
	end->addr = 0;
	end->size = 0;
	--a->n;
	idx = 0;
  } else {
	idx = -1;
  }
  iarray_unlock(a);
  return idx;
}

static void swap(iarray_elt_t * a, iarray_elt_t * b)
{
  iarray_elt_t tmp = *a;
  *a = *b;
  *b = tmp;
}

extern unsigned int rand();

void iarray_shuffle(iarray_t *a, int idx, int n)
{
  int i;
  iarray_elt_t * e;
  iarray_lock(a);

  e = a->elt;
  if ((unsigned int)idx >= a->n || n <= 0) {
    return;
  }

  if (idx+n > a->n) {
    n = a->n - idx;
  }
  
  for (i=idx; i<n; ++i) {
    int m = (rand()&0x7FFFFFF) % (n-idx) + idx;
	swap(e+m, e+i);
  }

  iarray_unlock(a);
}

static int find_max(const iarray_elt_t * e, int n, iarray_sort_f cmp)
{
  int i, k=0;
  const iarray_elt_t * ek = e;
  
  for (i=1, ++e; i<n; ++i, ++e) {
    if (cmp(e, ek) > 0) {
      k = i;
      ek = e;
    }
  }
  return k; 
}

static void sort_part(iarray_t *a, int idx, int n, iarray_sort_f cmp)
{
  iarray_elt_t * e;

  if (n<=0) {
	return;
  }

  if (idx < 0) {
    idx = 0;
  }  else if (idx > a->n) {
    idx = a->n;
  }
  e = a->elt + idx;

  if (n+idx > a->n) {
    n = a->n-idx;
  }
  
  for (; n>0; --n) {
    int k = find_max(e, n, cmp);
	swap(e+k, e+n-1);
  }
}

void iarray_sort(iarray_t *a, iarray_sort_f cmp)
{
  iarray_lock(a);
  sort_part(a, 0, a->n, cmp);
  iarray_unlock(a);
}

void iarray_sort_part(iarray_t *a, int idx, int n, iarray_sort_f cmp)
{
  iarray_lock(a);
  if (idx < 0) {
    idx = 0;
  }  else if (idx > a->n) {
    idx = a->n;
  }
  if (n+idx > a->n) {
    n = a->n-idx;
  }
  sort_part(a, idx, n, cmp);
  iarray_unlock(a);
}

