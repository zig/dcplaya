/**
 * @file      display_list.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id: display_list.c,v 1.4 2002-10-12 20:28:53 vincentp Exp $
 */


#include <malloc.h>

#include "display_list.h"

/** this is the list of displayed list */
static dl_lists_t dl_active_lists;

/** this is the list of non displayed list */
static dl_lists_t dl_sleeping_lists;

matrix_t dl_trans;
dl_color_t dl_color;

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
  l->color[0] = 1.0f;
  l->color[1] = 1.0f;
  l->color[2] = 1.0f;
  l->color[3] = 1.0f;

/*   l->clipbox[0] = l->clipbox[1] = 0; */
/*   l->clipbox[1] = 640; */
/*   l->clipbox[3] = 480; */

  MtxIdentity(l->trans);

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
  LIST_REMOVE(l, g_list);
  unlocklists();

  free(l->heap);
  free(l);
}


int dl_set_active(dl_list_t * l, int active)
{
  int old;

  old = l->active;
  if (old == active)
    return old;

  locklists();

  LIST_REMOVE(l, g_list);
  if (!active)
    LIST_INSERT_HEAD(&dl_sleeping_lists, l, g_list);
  else
    LIST_INSERT_HEAD(&dl_active_lists, l, g_list);

  l->active = active;

 done:

  unlocklists();

  return old;
}

int dl_get_active(dl_list_t * l)
{
  return l->active;
}

/* must be a power of two */
#define MIN_ALLOC 8

void * dl_alloc(dl_list_t * dl, size_t size)
{
  void * r = NULL;

  lock(dl);

  size += (-size) & (MIN_ALLOC - 1);

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

void draw_get_clipping(float*,float*,float*,float*);
void draw_set_clipping(const float,const float,const float,const float);

static void dl_render(int opaque)
{
  dl_list_t * l;

  locklists();

  LIST_FOREACH(l, &dl_active_lists, g_list) {
    dl_command_t * c;

    lock(l);

	draw_get_clipping(l->save_clip_box+0, l->save_clip_box+1,
					  l->save_clip_box+2, l->save_clip_box+3);
    memcpy(dl_trans, l->trans, sizeof(dl_trans));
    memcpy(dl_color, l->color, sizeof(dl_color));
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
 	draw_set_clipping(l->save_clip_box[0], l->save_clip_box[1],
 					  l->save_clip_box[2], l->save_clip_box[3]);

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


void dl_set_trans(dl_list_t * dl, matrix_t mat)
{
  memcpy(dl->trans, mat, sizeof(dl->trans));
}

float * dl_get_trans(dl_list_t * dl)
{
  return dl->trans[0];
}

void dl_set_color(dl_list_t * dl, dl_color_t col)
{
  memcpy(dl->color, col, sizeof(dl->color));
}

float * dl_get_color(dl_list_t * dl)
{
  return dl->color;
}

