/**
 * @file     allocator.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/18
 * @brief    fast allocator for fixed size small buffer.
 * 
 * $Id: allocator.c,v 1.4 2002-10-21 14:57:00 benjihan Exp $
 */

#include <stdlib.h>
#include <stdio.h>

#include "allocator.h"
#include "sysdebug.h"

allocator_t * allocator_create(int nmemb, int size)
{
  allocator_t *a, *al;
  allocator_elt_t *p , *e;
  int i;
  int elt_size = size+sizeof(allocator_elt_t);
  int data_size = nmemb * elt_size;
  int align;

  SDDEBUG("Creating an allocator [%dx%d]\n",nmemb,size);

  al = malloc(15 + sizeof(*a) - sizeof(a->buffer) + data_size);
  if (!al) {
	return 0;
  }
  align = (-(int)al->buffer) & 15;
  a = (allocator_t *)( (char *)al + align );
  a->realaddress = al;
  a->used = 0;
  a->free = (allocator_elt_t *) a->buffer;
  spinlock_init(&a->mutex);
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
  return a;
}

void allocator_destroy(allocator_t * a)
{
  if (a && a->realaddress) {
	SDDEBUG("Destroying an allocator [%dx%d]\n",a->elements, a->elt_size);
	free(a->realaddress);
  }
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
  spinlock_lock(&a->mutex);
  e = a->free;

  if (e) {
	link_used(a,e);
  }
  spinlock_unlock(&a->mutex);
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

  spinlock_lock(&a->mutex);
  if (e = a->free, !e) {
	void * p;
	spinlock_unlock(&a->mutex);
	p = malloc(size);
	return p;
  }
  link_used(a,e);
  spinlock_unlock(&a->mutex);
  return e+1;
}

static int allocator_count(allocator_t * a, const allocator_elt_t * e)
{
  int i;

  for (i=0 ; e; e=e->next, ++i)
	;
  spinlock_unlock(&a->mutex);
  return i;
}

int allocator_count_used(allocator_t * a)
{
  spinlock_lock(&a->mutex);
  return allocator_count(a, a->used);
}

int allocator_count_free(allocator_t * a)
{
  spinlock_lock(&a->mutex);
  return allocator_count(a, a->free);
}

int allocator_is_inside(const allocator_t * a, const void * data)
{
  const allocator_elt_t *me = (const allocator_elt_t *)data - 1;
  return (const char*)me >= a->buffer && (const char*)me < a->bufend;
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
  spinlock_unlock(&a->mutex);
  return e == me;
}

static int allocator_is_used(allocator_t * a, const void * data)
{
  if (!allocator_is_inside(a,data)) {
	return 0;
  }
  spinlock_lock(&a->mutex);
  return allocator_search(a, a->used, data);
}

static int allocator_is_free(allocator_t * a, const void * data)
{
  if (!allocator_is_inside(a,data)) {
	return 0;
  }
  spinlock_lock(&a->mutex);
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
	if ( !allocator_is_used(a,data) ) {
	  SDCRITICAL("allocator try to free something not used (%p) !!!\n", data);
	  err = 1;
	}
	if ( allocator_is_free(a,data)) {
	  SDCRITICAL("allocator try to free something free (%p) !!!\n", data);
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

  spinlock_lock(&a->mutex);

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
  spinlock_unlock(&a->mutex);
}

void * allocator_match(allocator_t * a, const void * data,
					   int (*cmp)(const void *, const void *))
{
  allocator_elt_t * e;
  spinlock_lock(&a->mutex);
  for (e=a->used; e && cmp(data,e+1); e=e->next)
	; 
  spinlock_unlock(&a->mutex);
  return e ? e+1 : 0;
}

void allocator_dump(allocator_t * a)
{
  int i,j;
  allocator_elt_t *e;

  spinlock_lock(&a->mutex);
  printf("allocator %p, %d elements of %d bytes\n",
		 a, a->elements, a->elt_size);

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
	printf("Allocator %p inconsistent  : %d differs from %d\n",
		   a,i+j,a->elements);
  }
  spinlock_unlock(&a->mutex);
}

void allocator_lock(allocator_t * a)
{
  spinlock_lock(&a->mutex);
}

void allocator_unlock(allocator_t * a)
{
  spinlock_unlock(&a->mutex);
}
