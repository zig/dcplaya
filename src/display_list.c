/**
 * @ingroup   dcplaya_devel
 * @file      display_list.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @author    benjamin gerard <ben@sashipa.com>
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id: display_list.c,v 1.12 2002-12-01 19:19:14 ben Exp $
 */

#include <malloc.h>
#include "display_list.h"
#include "draw/draw.h"
#include "draw/gc.h"
#include "draw/text.h"
#include "sysdebug.h"

#define DLCOM(HEAP,OFFSET) ((dl_command_t *)((char*)(HEAP)+(OFFSET)))
#define DLID(HEAP,COM)     ((dl_comid_t)((char*)(COM)-(char*)(HEAP)))

#define DL_MAIN_TYPE  0
#define DL_SUB_TYPE   1
#define DL_DEAD_TYPE  2

static const char * typestr[3] =  { "main", "sub", "dead" };
	
/* List of display list. */
static dl_lists_t dl_lists[3];

/* List of display list mutex. */
static spinlock_t listmutex;

static dl_runcontext_t listrc;


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

static void changetype(dl_list_t *l, int type)
{
  if (l->flags.type != type) {
	LIST_REMOVE(l, g_list);
	l->flags.type = type;
	LIST_INSERT_HEAD(&dl_lists[type], l, g_list);
  }
}

int dl_init(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  spinlock_init(&listmutex);
  LIST_INIT(&dl_lists[0]);
  LIST_INIT(&dl_lists[1]);
  LIST_INIT(&dl_lists[2]);

  *(int *) &listrc = 0;
  listrc.color_inherit = listrc.trans_inherit = DL_INHERIT_LOCAL;

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);

  return 0;
}

static void real_destroy(dl_list_t * l);

int dl_shutdown(void)
{
  dl_list_t *l, *next;
  int j;

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  locklists();
  for (j=0; j<3; ++j) {
	SDDEBUG("Destroying [%s-lists]\n", typestr[j]);
	SDINDENT;
	for (l=LIST_FIRST(&dl_lists[j]); l; l=next) {
	  next = LIST_NEXT(l,g_list);
	  SDDEBUG(" - destroying [%p,%d]\n", l, l->refcount);
	  real_destroy(l);
	}
	SDUNINDENT;
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

int dl_dereference(dl_list_t * dl)
{
  int ref;

  lock(dl);
  ref = --dl->refcount;
  unlock(dl);
  if (ref <= 0) {
	ref = 0;
	dl_destroy(dl);
  }
  return ref;
}

dl_list_t * dl_create(int heapsize, int active, int sub)
{
  dl_list_t * l;

  l = calloc(1, sizeof(dl_list_t));
  if (!l) {
	return 0;
  }

  spinlock_init(&l->mutex);
  l->flags.active = !!active;
  l->flags.type = sub ? DL_SUB_TYPE : DL_MAIN_TYPE;

  l->heap = heapsize ? malloc(heapsize) : 0;
  l->heap_size = l->heap ? heapsize : 0;
  l->color.a = l->color.r = l->color.g = l->color.b = 1.0f;
  MtxIdentity(l->trans);
  dl_clear(l);
  l->refcount = 1;

  locklists();
  LIST_INSERT_HEAD(&dl_lists[l->flags.type], l, g_list);
  unlocklists();

  return l;
}

static void real_destroy(dl_list_t * l)
{
  if (l->refcount) {
	SDWARNING("[%s] : [%p] refcount = [%d]\n", __FUNCTION__, l, l->refcount);
  } 
  if (l->heap) free(l->heap);
  if (l) free(l);
}

void dl_destroy(dl_list_t * l)
{
  locklists();
  lock(l);
  if (l->refcount) {
	changetype(l, DL_DEAD_TYPE);
	l->flags.active = 0;
	SDWARNING("[%s] : [%p,%d] added to dead-lists.\n", __FUNCTION__,
			  l, l->refcount);
	unlock(l);
  } else {
	SDDEBUG("[%s] : [%p, %s-list]\n", __FUNCTION__,
			l, typestr[l->flags.type]);
	LIST_REMOVE(l, g_list);
	real_destroy(l);
  }
  unlocklists();
}

int dl_set_active(dl_list_t * l, int active)
{
  int old;

  lock(l);
  old = l->flags.active;
  l->flags.active = active && (l->flags.type != DL_DEAD_TYPE);
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

dl_comid_t dl_insert(dl_list_t * dl, void * pcom,
					 dl_command_func_t o_render, dl_command_func_t t_render)

{
  dl_command_t * com = pcom;
  dl_comid_t lastid;
  dl_comid_t id;

/*   SDDEBUG("[%s] : [%p] [%p]\n", __FUNCTION__, dl, pcom); */

  if (!com) {
	return DL_COMID_ERROR;
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

  return id;
}

void dl_insert2(dl_list_t * dl, dl_comid_t id,
			   dl_command_func_t o_render, dl_command_func_t t_render)
{
  if (id != DL_COMID_ERROR) {
	return;
  }
  lock(dl);
  dl_insert(dl, DLCOM(dl->heap,id), o_render, t_render);
}

static dl_code_e dl_render_list(dl_runcontext_t * rc, dl_context_t * parent,
								dl_list_t * l, int opaque)
{
  dl_comid_t id;
  dl_command_t * c;
  dl_context_t context;
  dl_code_e code;
  
  int i, start_idx, end_idx;
  char * const heap = l->heap;

  if (!l->flags.active) {
	return DL_COMMAND_OK;
  }

  if ((start_idx = 0) >= (end_idx = l->n_commands)) {
	return DL_COMMAND_OK;
  }

/*   if (rc->gc_flags) { */
/* 	gc_push(rc->gc_flags); */
/*   } */

/*   SDDEBUG("[%s] [%p,%s,%d]\n", __FUNCTION__, */
/* 		  l, typestr[l->flags.type], l->n_commands); */
/*   SDINDENT; */

  switch (rc->trans_inherit) {
  case DL_INHERIT_PARENT:
	MtxCopy(context.trans, parent->trans);
	break;
  case DL_INHERIT_LOCAL:
	MtxCopy(context.trans, l->trans);
	break;
  default:
	MtxMult3(context.trans, l->trans, parent->trans);
  }

  switch (rc->color_inherit) {
  case DL_INHERIT_PARENT:
	context.color = parent->color;
	break;
  case DL_INHERIT_LOCAL:
	context.color = l->color;
	break;
  case DL_INHERIT_ADD:
	draw_color_add_clip(&context.color, &parent->color, &l->color);
	break;
  default:
	draw_color_mul_clip(&context.color, &parent->color, &l->color);
  }

  /* Skipped at start. */
  for (i=0, c=0, id=l->first_comid;
	   i<start_idx/* && id != DL_COMID_ERROR*/;
	   ++i, id = c->next_id) {
	c = DLCOM(heap, id);
  }

  for (code = DL_COMMAND_OK; i < end_idx /*&& id != DL_COMID_ERROR*/;
		++i, id = c->next_id) {

	c = DLCOM(heap, id);

	if (c->flags.inactive) {
	  SDDEBUG("[%p,%d/%d], INACTVE\n", l, i, end_idx);
	  continue;
	}
	
	code = DL_COMMAND_OK; 
	if (opaque) {
	  if (c->render_opaque)
		code = c->render_opaque(c, &context);
	} else {
	  if (c->render_transparent)
		code = c->render_transparent(c, &context);
	}

	switch(code) {
	case DL_COMMAND_OK:
	case DL_COMMAND_ERROR:
	  break;
  
	case DL_COMMAND_BREAK:
	  SDDEBUG("[%s] : [%p,%d/%d] : BREAK\n",__FUNCTION__,l,i,end_idx);
	  i = end_idx;
	  break;

	case DL_COMMAND_SKIP:
	  SDDEBUG("[%s] : [%p,%d/%d] : SKIP\n",__FUNCTION__,l,i,end_idx);
	  if (++i < end_idx) {
		c = DLCOM(heap, c->next_id);
	  }
	  break;
	}
  }

/*   SDUNINDENT; */

/*   if (rc->gc_flags) { */
/* 	gc_pop(rc->gc_flags); */
/*   } */

  return code;
}

static void dl_render(int opaque)
{
  dl_list_t * l;

  locklists();
  LIST_FOREACH(l, &dl_lists[DL_MAIN_TYPE], g_list) {
	gc_reset();
	lock(l);
	dl_render_list(&listrc, 0, l, opaque);
	unlock(l);
  }
  gc_reset();
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

/* NOP command */

static dl_code_e nop_render(void * pcom, dl_context_t * context)
{
  return DL_COMMAND_OK;
}

dl_comid_t dl_nop_command(dl_list_t * dl)
{
  dl_comid_t err = DL_COMID_ERROR;
  struct  {
	 dl_command_t uc;
  } * c;

  c = dl_alloc(dl, sizeof(*c));
  if (c) {
	err = dl_insert(dl, c, nop_render, nop_render);
  }
  return err;
}

/* SUBLIST command */

struct sublist_command_t {
  dl_command_t uc;
  dl_list_t * sublist;
  dl_runcontext_t rc;
};

static dl_code_e sub_render(struct sublist_command_t * c,
							dl_context_t * context,
							int opaque)
{
  dl_code_e code;
  dl_list_t * dl = c->sublist;

  lock(dl);
  code = dl_render_list(&c->rc, context, dl, opaque);
  unlock(dl);

  return code;
}

static dl_code_e sub_render_opaque(void * pcom, dl_context_t * context)
{
  struct sublist_command_t * c = pcom;
  return sub_render(c, context, 1);
}

static dl_code_e sub_render_transparent(void * pcom, dl_context_t * context)
{
  struct sublist_command_t * c = pcom;
  return sub_render(c, context, 0);
}

dl_comid_t dl_sublist_command(dl_list_t * dl, dl_list_t * sublist,
							  const dl_runcontext_t * rc)
{
  dl_comid_t err = DL_COMID_ERROR;
  struct sublist_command_t * c = 0;

  if (dl == sublist) {
	SDERROR("[%s] : Nested lists.\n", __FUNCTION__);
	goto finish;
  }

  lock(sublist);
  changetype(sublist, DL_SUB_TYPE);
  ++sublist->refcount;
  unlock(sublist);

  c = dl_alloc(dl, sizeof(*c));
  if (c) {
	c->sublist = sublist;
	c->rc = *rc;
	err = dl_insert(dl, c, sub_render_opaque, sub_render_transparent);
  } else {
	dl_dereference(sublist);
  }
  
 finish:

  SDDEBUG("[%s] : [%p,%p] := [%d]\n", __FUNCTION__, dl, sublist, err);

  return err;
}
