/**
 * @file     allocator.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/18
 * @brief    fast allocator for fixed size small buffer.
 * 
 * $Id: allocator.c,v 1.6 2003-01-24 04:28:13 ben Exp $
 */

#include <stdlib.h>
#include <stdio.h>

#include "allocator.h"
#include "sysdebug.h"

#ifdef ALLOCATOR_SEM
# define ALLOCATOR_LOCK(A)   sem_wait((A)->sem)
# define ALLOCATOR_UNLOCK(A) sem_signal((A)->sem)
#elif defined ALLOCATOR_MUTEX
# define ALLOCATOR_LOCK(A)   mutex_lock(&(A)->mutex)
# define ALLOCATOR_UNLOCK(A) mutex_unlock(&(A)->mutex)
#else
# define ALLOCATOR_LOCK(A)   spinlock_lock(&(A)->mutex)
# define ALLOCATOR_UNLOCK(A) spinlock_unlock(&(A)->mutex)
#endif

allocator_t * allocator_create(int nmemb, int size, const char * name)
{
  allocator_t *a, *al;
  allocator_elt_t *p , *e;
  int i;
  int elt_size = size+sizeof(allocator_elt_t);
  int data_size = nmemb * elt_size;
  int align;

  name = name ? name : "noname";
  SDDEBUG("Creating an allocator [%s %dx%d]\n",name, nmemb,size);

  al = malloc(15 + sizeof(*a) - sizeof(a->buffer) + data_size);
  if (!al) {
    return 0;
  }
  align = (-(int)al->buffer) & 15;
  a = (allocator_t *)( (char *)al + align );
  a->realaddress = al;
  a->used = 0;
  a->free = (allocator_elt_t *) a->buffer;
  a->name = name;
#ifdef ALLOCATOR_SEM
  a->sem = sem_create(0);
#elif defined ALLOCATOR_MUTEX
  mutex_init(&a->mutex, 0);
#else
  spinlock_init(&a->mutex);sd
#endif
			     a->elt_size = size;
  a->elements = nmemb;
  a->bufend = a->buffer + data_size;
  for (p=0, e=a->free, i=0; i<nmemb; ++i) {
    if (p) {
      p->next = e;
    }
#if DEBUG_LEVEL > 1
    SDDEBUG(" #%3d -> %p\n",i,e);
#endif
    e->prev = p;
    e->next = 0;
    p = e;
    e = (allocator_elt_t *) ((char *)e+elt_size);
  }
  SDDEBUG(" -> [@:%p real:%p buffer:%p]\n",a, a->realaddress, a->buffer);
#ifdef ALLOCATOR_SEM
  sem_signal(a->sem);
#endif

  return a;
}

void allocator_destroy(allocator_t * a)
{
  if (a && a->realaddress) {
    SDDEBUG("Destroying an allocator [%s %dx%d]\n",a->elements, a->elt_size,
	    a->name);
    free(a->realaddress);
  }
#ifdef ALLOCATOR_SEM
  sem_destroy(a->sem);
  a->sem = 0;
#endif  
}

static void link_used(allocator_t * a, allocator_elt_t *e)
{
  allocator_elt_t *n;
  e->prev = 0;
  a->free = e->next;
  n = e->next = a->used;
  if (n) {
    n->prev = e;
  }
  a->used = e;
}

void * allocator_alloc_inside(allocator_t * a)
{
  allocator_elt_t *e;
  ALLOCATOR_LOCK(a);
  e = a->free;

  if (e) {
    link_used(a,e);
  }
  ALLOCATOR_UNLOCK(a);
  return e+1;
}

void * allocator_alloc(allocator_t * a, unsigned int size)
{
  allocator_elt_t *e;

  if (size > a->elt_size) {
    void * p;
    p = malloc(size);
    return p;
  }

  ALLOCATOR_LOCK(a);
  if (e = a->free, !e) {
    void * p;
    ALLOCATOR_UNLOCK(a);
    p = malloc(size);
    return p;
  }
  link_used(a,e);
  ALLOCATOR_UNLOCK(a);
  return e+1;
}

static int allocator_count(allocator_t * a, const allocator_elt_t * e)
{
  int i;

  for (i=0 ; e; e=e->next, ++i)
    ;
  ALLOCATOR_UNLOCK(a);
  return i;
}

int allocator_count_used(allocator_t * a)
{
  ALLOCATOR_LOCK(a);
  return allocator_count(a, a->used);
}

int allocator_count_free(allocator_t * a)
{
  ALLOCATOR_LOCK(a);
  return allocator_count(a, a->free);
}

int allocator_is_inside(const allocator_t * a, const void * data)
{
  const allocator_elt_t *me = (const allocator_elt_t *)data - 1;
  return (const char*)me >= a->buffer && (const char*)me < a->bufend;
}

int allocator_is_mine(const allocator_t * a, const void * data)
{
  const unsigned int esize = a->elt_size + sizeof(allocator_elt_t);
  const char * me = (const char *)((const allocator_elt_t *)data - 1);
  if (me >= a->buffer && me <= a->bufend-esize) {
    int idx = (me - a->buffer) % esize;
    if (idx) {
      SDCRITICAL("allocator [%p,%s] : %p unaligned address [%d]\n",
		 a, a->name, data, idx);
      return 0;
    }
  } else {
    SDCRITICAL("allocator [%p,%s] : %p invalid range address\n",
	       a, a->name, data);
    return 0;
  }
  return 1;
}


int allocator_index(const allocator_t * a, const void * data)
{
  const char * me = (const char *)((const allocator_elt_t *)data - 1);
  int idx;

  idx = (me - a->buffer) / (a->elt_size + sizeof(allocator_elt_t));
  if ((unsigned int)idx >= a->elements) {
    idx = -1;
  }
  return idx;
}

static int allocator_search(allocator_t * a, const allocator_elt_t * e,
			    const void * data)
{
  allocator_elt_t *me = (allocator_elt_t *)data - 1;
  for ( ; e && e != me; e=e->next)
    ;
  ALLOCATOR_UNLOCK(a);
  return e == me;
}

static int allocator_is_used(allocator_t * a, const void * data)
{
  if (!allocator_is_inside(a,data)) {
    return 0;
  }
  ALLOCATOR_LOCK(a);
  return allocator_search(a, a->used, data);
}

static int allocator_is_free(allocator_t * a, const void * data)
{
  if (!allocator_is_inside(a,data)) {
    return 0;
  }
  ALLOCATOR_LOCK(a);
  return allocator_search(a, a->free, data);
}

void allocator_free(allocator_t * a, void * data)
{
  allocator_elt_t * e , * n, * p;

#ifdef DEBUG
  if (!allocator_is_inside(a,data)) {
    /* 	SDDEBUG("allocator real free(%p)\n", data); */
  } else {
    int err = 0;
    if (!allocator_is_mine(a,data)) {
      err = 1;
    } else if (!allocator_is_used(a,data)) {
      SDCRITICAL("allocator [%p,%s] : try to free something not used (%p)\n",
		 a, a->name, data);
      err = 1;
    }
    if (allocator_is_free(a,data)) {
      SDCRITICAL("allocator [%p,%s]  : try to free something free (%p)\n",
		 a, a->name, data);
      err = 1;
    }
    if (err) {
      return;
    }
  }
#endif
  if (!allocator_is_inside(a,data)) {
    free(data);
    return;
  }

  ALLOCATOR_LOCK(a);

  e = (allocator_elt_t *)data - 1;
  n = e->next;
  p = e->prev;

  if (!p) {
    a->used = n;
  } else {
    p->next = n;
  }
  if (n) {
    n->prev = p;
  }

  e->next = a->free;
  a->free = e;
  ALLOCATOR_UNLOCK(a);
}

void * allocator_match(allocator_t * a, const void * data,
		       int (*cmp)(const void *, const void *))
{
  allocator_elt_t * e;
  ALLOCATOR_LOCK(a);
  for (e=a->used; e && cmp(data,e+1); e=e->next)
    ; 
  ALLOCATOR_UNLOCK(a);
  return e ? e+1 : 0;
}

void allocator_dump(allocator_t * a)
{
  int i,j;
  allocator_elt_t *e;

  ALLOCATOR_LOCK(a);
  printf("allocator [%p,%s], %d elements of %d bytes\n",
	 a, a->name, a->elements, a->elt_size);

  printf("\nFree elements:\n");
  for (i=0, e=a->free; e; e=e->next, ++i) {
    printf("#%03d @:%p, data:%p\n",
	   ((char *)e-a->buffer) / (a->elt_size+sizeof(*e)),e,e+1);
  }
  printf("%d free elements\n", i);
	
  printf("\nUsed elements:\n");
  for (j=0, e=a->used; e; e=e->next, ++j) {
    printf("#%03d @:%p, data:%p\n",
	   ((char *)e-a->buffer) / (a->elt_size+sizeof(*e)),e,e+1);
  }
  printf("%d used elements\n", j);

  if (j+i != a->elements) {
    printf("Allocator [%p,%s] inconsistent  : %d differs from %d\n",
	   a,a->name,i+j,a->elements);
  }
  ALLOCATOR_UNLOCK(a);
}

void allocator_lock(allocator_t * a)
{
  ALLOCATOR_LOCK(a);
}

void allocator_unlock(allocator_t * a)
{
  ALLOCATOR_UNLOCK(a);
}

