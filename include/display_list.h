/**
 * @file      display_list.h
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id: display_list.h,v 1.1 2002-09-25 21:38:33 vincentp Exp $
 */


#ifndef _DISPLAY_LIST_H_
#define _DISPLAY_LIST_H_

#include <arch/spinlock.h>
#include <sys/queue.h>

struct dl_command;

typedef void (*dl_command_func_t)(void *);

/** display list command generic structure */
typedef struct dl_command {
  struct dl_command * next;             /** next command in list */

  dl_command_func_t render_opaque;      /** function to call */
  dl_command_func_t render_transparent; /** function to call */
} dl_command_t;

/** display list structure */
typedef struct dl_list {
  LIST_ENTRY(dl_list) g_list;  /**< linked list entry */

  dl_command_t * command_list; /**< first element of the command list */

  char * heap;                 /**< command heap */
  int    heap_size;            /**< size of the heap */
  int    heap_pos;             /**< position in heap */

  spinlock_t mutex;            /**< mutex */

  int    active;               /**< active state */

} dl_list_t;

typedef LIST_HEAD(dl_lists, dl_list) dl_lists_t;

/* FUNCTION API */

/** Open the display list library */
int dl_open();

/** Close the display list library */
int dl_shutdown();


/** create a new display list 
  *
  * @arg heapsize size of the command heap (0 to get a default size of 1Kb)
  * @arg active initial active state
  *
  */
dl_list_t * dl_new_list(int heapsize, int active);


/** Destroy the given display list */
void dl_destroy_list(dl_list_t * dl);


/** change active state of a display list, return old state */
int dl_set_active(dl_list_t *, int active);

/** query active state of given display list */
int dl_get_active(dl_list_t *);

/** allocate requested number of bytes in the display list heap */
void * dl_alloc(dl_list_t * dl, size_t size);

/** clear the display list (i.e. reset the command list) */
void dl_clear(dl_list_t * dl);

/** opaque render all active display list */
void dl_render_opaque();

/** transparent render all active display list */
void dl_render_transparent();

/** insert the given command into the display list, filling the two render functions with given parameters */
void dl_insert(dl_list_t * dl, void * com, dl_command_func_t opaque_func, dl_command_func_t transparent_func);


#endif /* #ifdef _DISPLAY_LIST_H_ */

