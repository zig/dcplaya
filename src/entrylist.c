/* 2002/02/10 */

#include <kos.h>
#include <stdio.h>

#include "entrylist.h"

/*
void entrylist_lock(entrylist_t *list)
{
  if (list->sem) {
    sem_wait(list->sem);
  }
}

void entrylist_unlock(entrylist_t *list)
{
  list->sync = 1;
  if (list->sem) {
    sem_signal(list->sem);
  }
}
*/

void entrylist_sync(entrylist_t *list, int sync)
{
  if (sync) {
    sem_wait(list->sem);
    list->sync = 0;
  } else {
    list->sync = 1;
    sem_signal(list->sem);
  }
}

int entrylist_create(entrylist_t *list, int max, int elsz)
{
  int alignsz = (elsz+7)&-8;
  
  list->e = 0;
  list->nb = 0;
  list->size = 0;
  list->elsz = 0;
  
  list->sem = sem_create(0);
  if (!list->sem) {
    return -1;
  }
  
  list->e = malloc(max * alignsz);
  if (!list->e) {
    sem_destroy(list->sem);
    list->sem = 0;
    return -1;
  }
  list->size = max;
  list->elsz = alignsz;
  list->sync = 1;
  sem_signal(list->sem);
  
  return 0;
}

int entrylist_kill(entrylist_t *list)
{
  if (list->sem && list->sync) {sem_wait(list->sem); list->sync=1;}
  
  list->size = 0;
  list->nb   = 0;
  list->elsz = 0;
  if (list->e) {
    free(list->e);
    list->e = 0;
  }
  if (list->sem) {
    if (list->sync) sem_signal(list->sem);
    sem_destroy(list->sem);
    list->sem = 0;
  }
  list->sync = 0;
  return 0;
}

int entrylist_clean(entrylist_t *list)
{
  if (list->sync) {sem_wait(list->sem); list->sync=1;}
  list->nb = 0;
  if (list->sync) sem_signal(list->sem);
  return 0;
}

int entrylist_add(entrylist_t *list, const void *e, int idx)
{
  int nb;
  uint8 * src;
  
  if (list->sync) {sem_wait(list->sem); list->sync=1;}
  if (list->nb == list->size) {
    if (list->sync) sem_signal(list->sem);
    return -1;
  }
  
  if ((unsigned int)idx > (unsigned int)list->nb) {
    idx = list->nb;
  }
  
  src = list->e + idx * list->elsz;
  if (nb = list->nb - idx, nb) {
    memmove(src + list->elsz, src, nb * list->elsz);
  }
  memcpy (src, e, list->elsz);
  ++list->nb;
  if (list->sync) sem_signal(list->sem);
  return idx;
}

int entrylist_del(entrylist_t *list, int idx)
{
  int err = -1;
  
  if (list->sync) {sem_wait(list->sem); list->sync=1;}
  if ((unsigned int)idx < (unsigned int)list->nb) {
    int copy = list->nb - idx - 1;
    uint8 * dest = (uint8 *)list->e + idx*list->elsz;
    memmove(dest, dest+list->elsz, list->elsz * copy);
    --list->nb;
    err = 0;
  }
  if (list->sync) sem_signal(list->sem);
  return err;
}

void * entrylist_addrof(entrylist_t *list, int idx)
{
  if ((unsigned int) idx >= (unsigned int)list->nb) {
    return 0;
  } else {
    return (void *)(list->e + list->elsz * idx);
  }
}

int entrylist_get(entrylist_t *list, void * e, int idx)
{
  int err = -1;
  if (idx >= 0) {
    if (list->sync) {sem_wait(list->sem); list->sync=1;}
  
    if (idx < list->nb) {
      err = 0;
      memcpy(e, list->e + list->elsz * idx, list->elsz);
    }
  
    if (list->sync) sem_signal(list->sem);
  }
  return err;
}

int entrylist_find(entrylist_t *list, const void *what, int (*cmp)(const void *a, const void *b))
{
  int i, n;
  char *e;
  
  if (list->sync) {sem_wait(list->sem); list->sync=1;}
  e = (char *)list->e;
  n = list->nb;
  for (i=0; i<n && cmp(what, e); ++i, e += list->elsz);
  if (list->sync) sem_signal(list->sem);
  return (i==n) ? -1 : i;
}

static int find_max(const char *e, int elsz, int n, int (*cmp)(const void *a, const void *b))
{
  int i,k=0;
  const char *ek = e;
  
  for (i=1, e+=elsz; i<n; ++i, e+=elsz) {
    if (cmp(e, ek) > 0) {
      k = i;
      ek = e;
    }
  }
  return k; 
}

static void swapmem(void *a, void * b, int sz)
{
  int *ai = (int *)a;
  int *bi = (int *)b;
  sz >>= 2;
  
  while (sz--) {
    int tmp = *ai;
    *ai++ = *bi;
    *bi++ = tmp;
  }
}

int entrylist_sort(entrylist_t *list, int (*cmp)(const void *a, const void *b))
{
  /*
  int n;
  char *e;
  if (list->sync) {sem_wait(list->sem); list->sync=1;}

  e = list->e;
  n = list->nb;
  
  for (; n>0; --n) {
    int k = find_max(e, list->elsz, n, cmp);
    
    if (k != n-1) {
      swapmem(e + k * list->elsz, e + (n-1) * list->elsz, list->elsz);
    }
  }
  if (list->sync) sem_signal(list->sem);
  return 0;
  */
  return  entrylist_sort_part(list,0,list->nb,cmp);
}

int entrylist_sort_part(entrylist_t *list,
			int idx, int n,
			int (*cmp)(const void *a, const void *b))
{
  char *e;
  if (list->sync) {sem_wait(list->sem); list->sync=1;}

  if (idx < 0) {
    idx = 0;
  }  else if (idx > list->nb) {
    idx = list->nb;
  }
  e = list->e + idx * list->elsz;

  if (n+idx > list->nb) {
    n = list->nb-idx;
  }
  
  for (; n>0; --n) {
    int k = find_max(e, list->elsz, n, cmp);
    
    if (k != n-1) {
      swapmem(e + k * list->elsz, e + (n-1) * list->elsz, list->elsz);
    }
  }
  if (list->sync) sem_signal(list->sem);
  return 0;
}

int entrylist_shuffle(entrylist_t *list, int idx)
{
  int i,n;
  char *e;
  if (list->sync) {sem_wait(list->sem); list->sync=1;}

  e = list->e;
  n = list->nb;

  if (idx < 0) {
    idx = 0;
  }
  
  for (i=idx; i<n; ++i) {
    int m = (rand()&0xFFFF) % (list->nb-idx) + idx;
    
    if (i != m) {
      swapmem(e + m * list->elsz, e + i * list->elsz, list->elsz);
    }
  }
  if (list->sync) sem_signal(list->sem);
  return 0;
}


int entrylist_isfull(entrylist_t *list)
{
  int full;
  
  if (list->sync) {sem_wait(list->sem); list->sync=1;}
  full = list->nb == list->size;
  if (list->sync) sem_signal(list->sem);
  return full;
}


