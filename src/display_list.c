/**
 * @file      display_list.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id: display_list.c,v 1.9 2002-11-27 09:58:09 ben Exp $
 */

#include <malloc.h>
#include "display_list.h"
#include "draw/draw.h"
#include "draw/gc.h"
#include "sysdebug.h"

#define DLCOM(HEAP,OFFSET) ((dl_command_t *)((char*)(HEAP)+(OFFSET)))
#define DLID(HEAP,COM)     ((dl_comid_t)((char*)(COM)-(char*)(HEAP)))

/* List of display list. */
static dl_lists_t dl_lists;

/* List of display list mutex. */
static spinlock_t listmutex;

#define lock(l) spinlock_lock(&(l)->mutex)
#define unlock(l) spinlock_unlock(&(l)->mutex)


static void locklists(void)
{
  spinlock_lock(&listmutex);
}

static void unlocklists(void)
{
  spinlock_unlock(&listmutex);
}

int dl_init(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  spinlock_init(&listmutex);
  LIST_INIT(&dl_lists);

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);

  return 0;
}

static void real_destroy(dl_list_t * l);

int dl_shutdown(void)
{
  dl_list_t *l, *next;

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  locklists();
  for (l=LIST_FIRST(&dl_lists); l; l=next) {
	next = LIST_NEXT(l,g_list);
	SDDEBUG("destroying list [%p], next:[%p]\n", l,next);
	real_destroy(l);
  }
  unlocklists();

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);
  return 0;
}


void dl_reference(dl_list_t * dl)
{
  lock(dl);
  ++dl->refcount;
  unlock(dl);
}

void dl_dereference(dl_list_t * dl)
{
  lock(dl);
  if (--dl->refcount <= 0) {
	dl_destroy(dl);
  } else {
	unlock(dl);
  }
}

dl_list_t * dl_create(int heapsize, int active)
{
  dl_list_t * l;

  if (!heapsize) {
    heapsize = 1024;
  }
  //$$$ test enlarge
  //  heapsize = 8;

  l = calloc(1, sizeof(dl_list_t));
  if (!l) {
	return 0;
  }

  spinlock_init(&l->mutex);
  l->flags.active = !!active;
  l->heap = malloc(heapsize);
  l->heap_size = l->heap ? heapsize : 0;
  l->color.a = l->color.r = l->color.g = l->color.b = 1.0f;
  MtxIdentity(l->trans);
  dl_clear(l);
  l->refcount = 1;

  locklists();
  LIST_INSERT_HEAD(&dl_lists, l, g_list);
  unlocklists();

  return l;
}


static void real_destroy(dl_list_t * l)
{
  if (l->heap) free(l->heap);
  if (l) free(l);
}

void dl_destroy(dl_list_t * l)
{
  locklists();
  LIST_REMOVE(l, g_list);
  unlocklists();
  real_destroy(l);
}



int dl_set_active(dl_list_t * l, int active)
{
  int old;

  lock(l);
  old = l->flags.active;
  l->flags.active = !!active;
  unlock(l);

  return old;
}

int dl_set_active2(dl_list_t * l1, dl_list_t * l2, int active)
{
  int old;

  locklists();
  old = dl_set_active(l1, active&1) | (dl_set_active(l2, active&2) << 1);
  unlocklists();
  return old;
}

int dl_get_active(dl_list_t * l)
{
  return l->flags.active;
}

static int dl_enlarge(dl_list_t * dl, int min_size)
{
  int size;
  char * new_heap;


  size = dl->heap_size << 1;
  if (size < min_size) {
	size = min_size;
  }
  new_heap = realloc(dl->heap, size);

/*   SDDEBUG("[%s] [%p,%d] [%p %d] -> [%p %d]\n", __FUNCTION__, */
/* 		  dl, min_size, dl->heap, dl->heap_size, new_heap, size); */

  if (new_heap) {
	dl->heap = new_heap;
	dl->heap_size = size;
	return 0;
  }

  return -1;
}

/* must be a power of two */
#define MIN_ALLOC 16

int dl_data(dl_heap_transfert_t * hb, void * data, size_t size)
{
  int err = -1;

  lock(hb->dl);
  memcpy(hb->dl->heap + hb->cur, data, size);
  hb->cur += size;
  unlock(hb->dl);

  return err;
}

dl_comid_t dl_alloc2(dl_list_t * dl, size_t size, dl_heap_transfert_t * hb)
{
  dl_comid_t err;
  int req;

  lock(dl);
  size += (-size) & (MIN_ALLOC - 1);
  req = dl->heap_pos + size;
  if (req > dl->heap_size &&
	  dl_enlarge(dl, req) < 0) { 
    err = DL_COMID_ERROR;
  } else {
	hb->dl = dl;
	err = hb->cur = hb->start = dl->heap_pos;
	hb->end = hb->start + size;
	dl->heap_pos += size;
  }
  unlock(dl);

  return err;
}

void * dl_alloc(dl_list_t * dl, size_t size)
{
  void * r;
  int req;


  lock(dl);
  size += (-size) & (MIN_ALLOC - 1);
  req = dl->heap_pos + size;
  if (req > dl->heap_size &&
	  dl_enlarge(dl, req) < 0) { 
    r = 0;
  } else {
	r = dl->heap + dl->heap_pos;
	dl->heap_pos += size;
  }
/*   SDDEBUG("[%s] : [%p] [%d] := [%p]\n", __FUNCTION__, dl, size, r); */

  return r;
}

void dl_clear(dl_list_t * dl)
{
  lock(dl);
  dl->first_comid = dl->last_comid = DL_COMID_ERROR;
  dl->heap_pos = 0;
  dl->n_commands = 0;
  unlock(dl);
}


void dl_insert(dl_list_t * dl, void * pcom,
			   dl_command_func_t o_render, dl_command_func_t t_render)

{
  dl_command_t * com = pcom;
  dl_comid_t lastid;
  dl_comid_t id;

/*   SDDEBUG("[%s] : [%p] [%p]\n", __FUNCTION__, dl, pcom); */

  if (!com) {
	return;
  }
  com->next_id = DL_COMID_ERROR;
  com->flags.all = 0;
  com->render_opaque = o_render;
  com->render_transparent = t_render;

  id = DLID(dl->heap, com);
  lastid = dl->last_comid;
  if (lastid == DL_COMID_ERROR) {
	dl->first_comid = id;
  } else {
	dl_command_t * last = DLCOM(dl->heap, lastid);
	last->next_id = id;
  }
  dl->last_comid = id;
  ++dl->n_commands;
 
  unlock(dl);
}

void dl_insert2(dl_list_t * dl, dl_comid_t id,
			   dl_command_func_t o_render, dl_command_func_t t_render)
{
  if (id == DL_COMID_ERROR) {
	return;
  }
  lock(dl);
  dl_insert(dl, DLCOM(dl->heap,id), o_render, t_render);
}


static void dl_render_sub(dl_context_t * parent, dl_list_t * l, int opaque)
{
  dl_comid_t id;
  dl_command_t * c;
  dl_context_t context;
  int i, start_idx, end_idx;
  char * heap;

  if (!l) {
	return;
  }
  lock(l);
  heap = l->heap;

  if (!l->flags.active) {
	unlock(l);
	return;
  }

  switch (l->flags.trans_herit) {
  case DL_MODE_PARENT:
	MtxCopy(context.trans, parent->trans);
	break;
  case DL_MODE_LOCAL:
	MtxCopy(context.trans, l->trans);
	break;
  default:
	MtxMult3(context.trans, parent->trans, l->trans);
  }

  switch (l->flags.color_herit) {
  case DL_MODE_PARENT:
	context.color = parent->color;
	break;
  case DL_MODE_LOCAL:
	context.color = l->color;
	break;
  case DL_MODE_ADD:
	draw_color_add_clip(&context.color, &parent->color, &l->color);
	break;
  default:
	draw_color_add_clip(&context.color, &parent->color, &l->color);
  }

  start_idx = 0;
  end_idx = l->n_commands;

  /* Skipped at start. */
  for (i=0, c=0, id=l->first_comid;
	   i<start_idx/* && id != DL_COMID_ERROR*/;
	   ++i, id = c->next_id) {
	c = DLCOM(heap, id);
  }

  for ( ; i<end_idx/* && id != DL_COMID_ERROR*/;
		++i, id = c->next_id) {
	c = DLCOM(heap, id);

	if (c->flags.inactive) {
	  continue;
	}
	if (opaque) {
	  if (c->render_opaque)
		c->render_opaque(c, &context);
	} else {
	  if (c->render_transparent)
		c->render_transparent(c, &context);
	}
  }

  unlock(l);
}

static void dl_render(int opaque)
{
#if 1
  dl_list_t * l;
  dl_context_t context;

  static int debug = 0;

  locklists();

  debug = (debug+1) & 1023;

  gc_push();

  LIST_FOREACH(l, &dl_lists, g_list) {
/*     dl_command_t * c; */
/* 	float clipx1,clipy1,clipx2,clipy2; */

    lock(l);

/* 	if (debug == 1) { */
/* 	  SDDEBUG("[%s] : active:[%s] [%d]\n", __FUNCTION__, */
/* 			  l->flags.active ? "ON" : "OFF", l->n_commands); */
/* 	} */

	if (l->flags.active) {
	  
	  memcpy(context.trans, l->trans, sizeof(context.trans));
	  memcpy(&context.color, &l->color, sizeof(context.color));

#if 0	
	  if (l->clip_box.x1 < l->clip_box.x2) {
		clipx1 = dl_trans[0][0] * l->clip_box.x1 + dl_trans[3][0];
		clipx2 = dl_trans[0][0] * l->clip_box.x2 + dl_trans[3][0];
	  } else {
		clipx1 = 0;
		clipx2 = draw_screen_width;
	  }
	  if(l->clip_box.y1 < l->clip_box.y2) {
		clipy1 = dl_trans[1][1] * l->clip_box.y1 + dl_trans[3][1];
		clipy2 = dl_trans[1][1] * l->clip_box.y2 + dl_trans[3][1];
	  } else {
		clipy1 = 0;
		clipy2 = draw_screen_height;
	  }
	  draw_set_clipping4(clipx1,clipy1,clipx2,clipy2);
#endif
	  draw_set_clipping4(0,0,draw_screen_width,draw_screen_height);

	  {
		dl_comid_t id;
		const char * const heap = l->heap;
		id = l->first_comid;

		while (id != DL_COMID_ERROR) {
		  dl_command_t * c = DLCOM(heap,id);

		  if (opaque) {
			if (c->render_opaque)
			  c->render_opaque(c, &context);
		  } else {
			if (c->render_transparent)
			  c->render_transparent(c, &context);
		  }
		  id = c->next_id;
		}

	
	  }
	}
    unlock(l);
  }
  gc_pop(GC_RESTORE_ALL);
  unlocklists();
#endif 
}

void dl_render_opaque()
{
  dl_render(1);
}


void dl_render_transparent()
{
  dl_render(0);
}

void dl_set_trans(dl_list_t * dl, const matrix_t mat)
{
  memcpy(dl->trans, mat, sizeof(dl->trans));
}

float * dl_get_trans(dl_list_t * dl)
{
  return dl->trans[0];
}

void dl_set_color(dl_list_t * dl, const dl_color_t * col)
{
  memcpy(&dl->color, col, sizeof(dl->color));
}

dl_color_t * dl_get_color(dl_list_t * dl)
{
  return &dl->color;
}

/* void dl_set_clipping(dl_list_t * dl, const dl_clipbox_t box) */
/* { */
/*   memcpy(&dl->clip_box, &box, sizeof(dl->clip_box)); */
/* } */

/* float * dl_get_clipping(dl_list_t * dl) */
/* { */
/*   return &dl->clip_box.x1; */
/* } */

