/**
 * @file      display_list.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id: display_list.c,v 1.1 2002-09-25 21:38:34 vincentp Exp $
 */


#include <malloc.h>

#include "display_list.h"

/** this is the list of displayed list */
static dl_lists_t dl_active_lists;

/** this is the list of non displayed list */
static dl_lists_t dl_sleeping_lists;


#define lock(l) spinlock_lock(&(l)->mutex)
#define unlock(l) spinlock_unlock(&(l)->mutex)


static spinlock_t listmutex;

static void locklists(void)
{
  spinlock_lock(&listmutex);
}

void unlocklists(void)
{
  spinlock_unlock(&listmutex);
}


dl_list_t * dl_new_list(int heapsize, int active)
{
  dl_list_t * l;

  if (!heapsize)
    heapsize = 1024;

  l = calloc(1, sizeof(dl_list_t));

  l->active = active;
  l->heap = malloc(heapsize);
  l->heap_size = heapsize;

  locklists();

  if (active)
    LIST_INSERT_HEAD(&dl_active_lists, l, g_list);
  else
    LIST_INSERT_HEAD(&dl_sleeping_lists, l, g_list);

  unlocklists();

  return l;
}


void dl_destroy_list(dl_list_t * l)
{
  locklists();
  if (l->active)
    LIST_REMOVE(l, g_list);
  else
    LIST_REMOVE(l, g_list);
  unlocklists();

  free(l->heap);
  free(l);
}


int dl_set_active(dl_list_t * l, int active)
{
  int old;

  locklists();
  old = l->active;
  if (old == active)
    goto done;
  if (!active) {
    LIST_REMOVE(l, g_list);
    LIST_INSERT_HEAD(&dl_sleeping_lists, l, g_list);
  } else {
    LIST_REMOVE(l, g_list);
    LIST_INSERT_HEAD(&dl_active_lists, l, g_list);
  }

  l->active = active;
 done:
  unlocklists();

  return old;
}

int dl_get_active(dl_list_t * l)
{
  return l->active;
}

void * dl_alloc(dl_list_t * dl, size_t size)
{
  void * r = NULL;

  lock(dl);

  if (dl->heap_pos + size >= dl->heap_size)
    goto fail;

  r = dl->heap + dl->heap_pos;
  dl->heap_pos += size;

 fail:
  unlock(dl);
  return r;
}

void dl_clear(dl_list_t * dl)
{
  lock(dl);
  dl->command_list = 0;
  dl->heap_pos = 0;
  unlock(dl);
}

static void dl_render(int opaque)
{
  dl_list_t * l;

  locklists();

  LIST_FOREACH(l, &dl_active_lists, g_list) {
    dl_command_t * c;

    lock(l);

    c = l->command_list;
    while (c) {
      if (opaque) {
	if (c->render_opaque)
	  c->render_opaque(c);
      } else {
	if (c->render_transparent)
	  c->render_transparent(c);
      }
      c = c->next;
    }

    unlock(l);
  }

  unlocklists();
}

void dl_render_opaque()
{
  dl_render(1);
}


void dl_render_transparent()
{
  dl_render(0);
}

void dl_insert(dl_list_t * dl, void * pcom, dl_command_func_t opaque_func, dl_command_func_t transparent_func)
{
  dl_command_t * com = pcom;

  lock(dl);

  com->render_opaque = opaque_func;
  com->render_transparent = transparent_func;
  com->next = dl->command_list;
  dl->command_list = com;
  
  unlock(dl);
}

int dl_open()
{
  LIST_INIT(&dl_active_lists);
  LIST_INIT(&dl_sleeping_lists);

  return 0;
}

int dl_shutdown()
{
  // TODO : destroy all remaining display lists

  return 0;
}
