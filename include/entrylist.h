
/* 2002/02/10 */

#ifndef _ENTRYLIST_H_
#define _ENTRYLIST_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include <kos/thread.h>
#include <kos/sem.h>

typedef struct {
  int size;           /**< max number of entry */
  int nb;             /**< current number of entry */
  int elsz;           /**< 8 bytes aligned list element size */
  int sync;           /**< mutual exclusion enable */
  char *e;            /**< entry array */
  semaphore_t *sem;   /**< Access semaphore */
} entrylist_t;

int entrylist_create(entrylist_t *list, int max, int elsz);
int entrylist_kill(entrylist_t *list);
int entrylist_clean(entrylist_t *list);
int entrylist_add(entrylist_t *list, const void *e, int idx);
int entrylist_del(entrylist_t *list, int idx);
int entrylist_sort(entrylist_t *list,
		   int (*cmp)(const void *a, const void *b));
int entrylist_sort_part(entrylist_t *list,
			int idx, int n,
			int (*cmp)(const void *a, const void *b));

int entrylist_find(entrylist_t *list, const void *what, int (*cmp)(const void *a, const void *b));
int entrylist_shuffle(entrylist_t *list, int idx);

int entrylist_get(entrylist_t *list, void * e, int idx);

void * entrylist_addrof(entrylist_t *list, int idx);

int entrylist_isfull(entrylist_t *list);

/*
void entrylist_lock(entrylist_t *list);
void entrylist_unlock(entrylist_t *list);
*/
void entrylist_sync(entrylist_t *list, int sync);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _ENTRYLIST_H_ */
