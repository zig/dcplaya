/**
 * @file     exheap.h
 * @author   Vincent Penne <ziggy@sashipa.com>
 * @date     2003/01/19
 * @brief    External heap management.
 * 
 * $Id: exheap.h,v 1.2 2003-01-19 21:39:34 zigziggy Exp $
 */

#ifndef EXHEAP_H
#define EXHEAP_H

#include <stddef.h>
#include <sys/queue.h>

enum {
  EH_BLOCK_FREE,
  EH_BLOCK_USED
};


typedef struct eh_block eh_block_t;

/* currently 24 bytes */
/** Block structure. User should only use the <offset> field which indicate the
    offset of the memory chunk into the external heap */
struct eh_block {

  CIRCLEQ_ENTRY(eh_block) g_list;  /**< global linked list entry */
  CIRCLEQ_ENTRY(eh_block) g_freelist;  /**< linked list of free block entry */

  int flags; /* maybe we won't need this */

  size_t offset; /**< start offset of the heap chunk */

};

typedef struct eh_heap eh_heap_t;

typedef eh_block_t * (*eh_block_alloc_f)(eh_heap_t * heap);
typedef void (*eh_block_free_f)(eh_heap_t * heap, eh_block_t *);

/** Requires <size> total bytes in the heap, return actual new number of
    bytes */
typedef size_t (*eh_sbrk_f)(eh_heap_t * heap, size_t size);

typedef CIRCLEQ_HEAD(eh_block_list, eh_block) eh_block_list_t;


struct eh_heap {

  eh_block_list_t list;
  eh_block_list_t freelist;

  /** Allocation smaller than this threshold are performed at the end of */
  /** the heap, others are performed at the beginning. */
  size_t small_threshold; 

  eh_block_alloc_f freeblock_alloc;
  eh_block_free_f freeblock_free;

  eh_block_alloc_f usedblock_alloc;
  eh_block_free_f usedblock_free;

  eh_sbrk_f sbrk;

  void * userdata;

  /** When (free/used)block_alloc is called, this */
  /** contains the offset that will have the block. */
  /** This can be used by the alloc function to */
  /** actually allocate the block into the external */
  /** memory (if it is accessible as a normal pointer) */
  size_t current_offset; 


  /** Total heap size, may be increased if an sbrk function is provided. */
  size_t total_sz;

  eh_block_t * last_free; /**< used by the default block_free to actually cache freeing, same block may be reused by next alloc */
};



/** Create an external heap with default alloc/free/sbrk set of functions. */
eh_heap_t * eh_create_heap();

/** Destroy an external heap */
void eh_destroy_heap(eh_heap_t * heap);


/** Allocate <size> bytes into the external heap, return a block structure.
    @return NULL upon error, otherwise a pointer on the block structure. */
eh_block_t * eh_alloc(eh_heap_t * heap, size_t size);

/** Reallocate <size> bytes into the external heap. 
    @return NULL upon error, otherwise a pointer on the block structure. */
eh_block_t * eh_realloc(eh_heap_t * heap, eh_block_t * block, size_t newsize);

/** Free a block. */
void eh_free(eh_heap_t * heap, eh_block_t * block);

#endif /* ifndef EXHEAP_H */
