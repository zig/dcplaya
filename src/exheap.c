/**
 * @file     exheap.c
 * @author   Vincent Penne <ziggy@sashipa.com>
 * @date     2003/01/19
 * @brief    External heap management.
 * 
 * $Id: exheap.c,v 1.5 2003-01-20 14:57:24 zigziggy Exp $
 */


/* IDEA :

   

*/


#include "exheap.h"
#include "sysdebug.h"
#include <malloc.h>


/* #define EH_DEBUG */


#define MARK_USED(b) ( (b)->g_freelist.cqe_next = NULL )
#define IS_USED(b) ( (b)->g_freelist.cqe_next == NULL )
#define IS_FREE(b) ( (b)->g_freelist.cqe_next != NULL )


#define MIN_FREEBLOCK_SZ sizeof(eh_block_t)


eh_block_t * default_block_alloc(eh_heap_t * heap)
{
  if (heap->last_free) {
    eh_block_t * tmp = heap->last_free;
    heap->last_free = NULL;
    return tmp;
  } else
    return (eh_block_t *) malloc(sizeof(eh_block_t));
}

void default_block_free(eh_heap_t * heap, eh_block_t * block)
{
  if (heap->last_free)
    free(heap->last_free);
  heap->last_free = block;
}

size_t default_sbrk(eh_heap_t * heap, size_t size)
{
  return size; /* just say YES to all request :) */
}


eh_heap_t * eh_create_heap()
{
  eh_heap_t * heap;

  heap = (eh_heap_t *) calloc(sizeof(eh_heap_t), 1);

  if (heap == NULL)
    goto error;

  heap->freeblock_alloc = default_block_alloc;
  heap->freeblock_free = default_block_free;
  heap->usedblock_alloc = default_block_alloc;
  heap->usedblock_free = default_block_free;
  heap->sbrk = default_sbrk;

  CIRCLEQ_INIT(&heap->list);
  CIRCLEQ_INIT(&heap->freelist);

  return heap;

 error:
  if (heap != NULL)
    free(heap);

  return 0;
}

void eh_destroy_heap(eh_heap_t * heap)
{
  /* TODO !! */
}



static void dump_freeblock(eh_heap_t * heap)
{
#ifdef EH_DEBUG
  eh_block_t * b;
  size_t sz, total = 0;

  printf("Dumping free heap areas :\n");

  /* browse all free blocks */
  CIRCLEQ_FOREACH(b, &heap->freelist, g_freelist) {
    eh_block_t * next;
    next = CIRCLEQ_NEXT(b, g_list);
    if (next != (void *) &heap->list) {
      sz = next->offset - b->offset;
    } else {
      sz = heap->total_sz - b->offset;
    }

    printf("block %gKb (%x -- %x)\n", 
	   sz/1024.0f, b->offset, b->offset + sz - 1);
    total += sz;
  }

  printf("total %gKb\n", total/1024.0f);
#endif
}


static void free_usedblock(eh_heap_t * heap, eh_block_t * b)
{
  SDDEBUG("free_usedblock(%x)\n", b);
  CIRCLEQ_REMOVE(&heap->list, b, g_list);
  heap->usedblock_free(heap, b);
}

static void free_freeblock(eh_heap_t * heap, eh_block_t * b)
{
  SDDEBUG("free_freeblock(%x)\n", b);
  CIRCLEQ_REMOVE(&heap->freelist, b, g_freelist);
  CIRCLEQ_REMOVE(&heap->list, b, g_list);
  heap->freeblock_free(heap, b);
}

static eh_block_t * new_freeblock(eh_heap_t * heap, size_t offset)
{
  eh_block_t * b;

  heap->current_offset = offset;
  b = heap->freeblock_alloc(heap);
  if (b)
    b->offset = offset;

  return b;
}

static eh_block_t * new_usedblock(eh_heap_t * heap, size_t offset)
{
  eh_block_t * b;

  heap->current_offset = offset;
  b = heap->usedblock_alloc(heap);
  MARK_USED(b);
  if (b)
    b->offset = offset;

  return b;
}

static eh_block_t * do_alloc(eh_heap_t * heap, eh_block_t * b, size_t bsz, size_t size)
{
  eh_block_t * a;

  SDDEBUG("do_alloc(%x, %d, %d)\n", b, bsz, size);

  if (bsz - size < MIN_FREEBLOCK_SZ) {
    /* make sure we don't leave too small free blocks */
    bsz = size;
  }

  if (heap->usedblock_alloc == heap->freeblock_alloc && bsz == size) {
    /* used block totally replace free block */
    CIRCLEQ_REMOVE(&heap->freelist, b, g_freelist);
    MARK_USED(b);
    dump_freeblock(heap);
    return b;
  }

  a = new_usedblock(heap, size < heap->small_threshold? b->offset + bsz - size : b->offset);

  if (a == NULL)
    return NULL;

  if (bsz == size) {
    /* used block totally replace free block */
    CIRCLEQ_INSERT_BEFORE(&heap->list, b, a, g_list);

    free_freeblock(heap, b);
  } else
    if (size < heap->small_threshold) {
      CIRCLEQ_INSERT_AFTER(&heap->list, b, a, g_list);
    } else {
      eh_block_t * nb;
      eh_block_t * prev;
      size_t offset = b->offset;

      CIRCLEQ_INSERT_BEFORE(&heap->list, b, a, g_list);
      prev = CIRCLEQ_PREV(b, g_freelist);
      free_freeblock(heap, b);

      nb = new_freeblock(heap, offset + size);
      if (nb == NULL) {
	/* PROBLEM HERE !! in this case, what's happening is exactly the same
	   as if we allocated the entire free block ... */
      } else {

	if (prev == (void *)&heap->freelist)
	  CIRCLEQ_INSERT_HEAD(&heap->freelist, nb, g_freelist);
	else
	  CIRCLEQ_INSERT_AFTER(&heap->freelist, prev, nb, g_freelist);
	CIRCLEQ_INSERT_AFTER(&heap->list, a, nb, g_list);

      }

    }

  dump_freeblock(heap);
  return a;
}


eh_block_t * eh_alloc(eh_heap_t * heap, size_t size)
{
  eh_block_t * b;
  size_t sz;

  SDDEBUG("eh_alloc(%d)\n", size);

  /* make sure size not too small */
  if (size < MIN_FREEBLOCK_SZ) /* TODO : remove this limitation, free should take care about that instead */
    size = sizeof(eh_block_t);

  /* browse all free blocks */

#define TRY_FREEBLOCK                                \
      eh_block_t * next;                             \
      next = CIRCLEQ_NEXT(b, g_list);                \
      SDDEBUG("%x %x (%x)\n", b, next, &heap->list); \
      if (next != (void *) &heap->list) {            \
	sz = next->offset - b->offset;               \
      } else {                                       \
	sz = heap->total_sz - b->offset;             \
      }                                              \
      if (sz >= size) {                              \
	/* found a big enough free block ! */        \
	return do_alloc(heap, b, sz, size);          \
      }                                              \

  if (size < heap->small_threshold) {
    CIRCLEQ_FOREACH_REVERSE(b, &heap->freelist, g_freelist) {
      TRY_FREEBLOCK;
    }
  } else {
    CIRCLEQ_FOREACH(b, &heap->freelist, g_freelist) {
      TRY_FREEBLOCK;
    }
  }

  /* no room, need to try sbrk */
  b = CIRCLEQ_LAST(&heap->freelist);
  if (b == (void *)&heap->freelist || CIRCLEQ_NEXT(b, g_list) != (void *)&heap->list) {
    /* no free blocks or last free block is not last in global list,
       need to create a new free block at the end */
    size_t cur_sz = heap->total_sz;
    heap->total_sz = heap->sbrk(heap, cur_sz + size);

    sz = heap->total_sz - cur_sz;
    if (sz < size) {
      heap->total_sz = cur_sz; /* put back old value */
      return 0; /* FAILED : sbrk could not allocate enough */
    }
    
    b = new_freeblock(heap, cur_sz);
    if (!b)
      return 0; /* FAILED */

    /* insert the new free block */
    CIRCLEQ_INSERT_TAIL(&heap->freelist, b, g_freelist);
    CIRCLEQ_INSERT_TAIL(&heap->list, b, g_list);

  } else {
    /* we have a free block at the end of the list, but it is not big enough */
    size_t cur_sz = b->offset;
    heap->total_sz = heap->sbrk(heap, cur_sz + size);

    sz = heap->total_sz - cur_sz;
    if (sz < size)
      return 0; /* FAILED : sbrk could not allocate enough */
  }
  
  return do_alloc(heap, b, sz, size);
}

eh_block_t * eh_realloc(eh_heap_t * heap, eh_block_t * block, size_t newsize)
{
  /* TODO */
  return 0;
}

void eh_free(eh_heap_t * heap, eh_block_t * b)
{
  size_t offset = b->offset;
  eh_block_t * prev = CIRCLEQ_PREV(b, g_list);
  eh_block_t * next = CIRCLEQ_NEXT(b, g_list);

  SDDEBUG("eh_free(%x)\n", b);

  free_usedblock(heap, b);

  if (prev != (void *)&heap->list && IS_FREE(prev)) {
    if (next != (void *)&heap->list && IS_FREE(next)) {
      /* both previous and next blocks are free blocks, merge them */
      free_freeblock(heap, next);

    } else {
      /* nothing to do */
    }
  } else if (next != (void *)&heap->list && IS_FREE(next)) {
    /* just adjust the offset to include the newly freed block */
    next->offset = offset;
  } else {

    /* ok, none of the prev or next blocks were free blocks, need
       to create a new free block ... */
    b = new_freeblock(heap, offset);
      
    if (b == NULL) {
      /* NOW WE HAVE A PROBLEM !! The free block will be lost ... 
	 But the memory might be recovered later if an adjacent used
	 block is freed. */
    } else {
      if (prev == (void *)&heap->list) {
	CIRCLEQ_INSERT_HEAD(&heap->list, b, g_list);
	CIRCLEQ_INSERT_HEAD(&heap->freelist, b, g_freelist);
      } else {
	CIRCLEQ_INSERT_AFTER(&heap->list, prev, b, g_list);

	/* find first previous free block */
	for ( ; ; ) {
	  prev = CIRCLEQ_PREV(prev, g_list);
	  if (prev == (void *)&heap->list) {
	    CIRCLEQ_INSERT_HEAD(&heap->freelist, b, g_freelist);
	    break;
	  } else if (IS_FREE(prev)) {
	    CIRCLEQ_INSERT_AFTER(&heap->freelist, prev, b, g_freelist);
	    break;
	  }
	}
	  
      }

    }
  }
  dump_freeblock(heap);
}

