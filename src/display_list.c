/**
 * @ingroup   dcplaya_devel
 * @file      display_list.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @author    benjamin gerard <ben@sashipa.com>
 * @date      2002/09/12
 * @brief     thread safe display list support for dcplaya
 * @version   $Id: display_list.c,v 1.16 2002-12-26 07:12:57 ben Exp $
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

#define CHECK_INIT(RETURNVAL) if (!init) { \
   SDERROR("[%s] : display-list system not initialized.\n",__FUNCTION__); \
   return RETURNVAL; \
} else

struct sublist_command_t {
  dl_command_t uc;
  dl_list_t * sublist;
  dl_runcontext_t rc;
};

/* For debuggin' */
static const char * typestr[2] =  { "main", "sub" };
	
/* List of display list. */
static dl_lists_t dl_lists[2];

/* List of display list mutex. */
static spinlock_t listmutex;

/* Render context for main-list. */
static dl_runcontext_t listrc;

/* Set when display list system is not available */
static volatile int init;

/* #define lock(l) spinlock_lock(&(l)->mutex) */
/* #define unlock(l) spinlock_unlock(&(l)->mutex) */

#define lock(l)    totto
#define unlock(l)  werwef

static void real_destroy(dl_list_t * l, int force);
static void real_clear(dl_list_t * dl);
static int real_dereference(dl_list_t * dl);

static void locklists()
{
  spinlock_lock(&listmutex);
}

static void unlocklists()
{
  spinlock_unlock(&listmutex);
}

int dl_init(void)
{
  int err = -1;

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;
  if (init) {
    SDERROR("[%s] : already initialized.\n", __FUNCTION__);
    goto error;
  }
  /* Init mutex. */
  spinlock_init(&listmutex);

  /* Init lists. */
  LIST_INIT(&dl_lists[DL_MAIN_TYPE]);
  LIST_INIT(&dl_lists[DL_SUB_TYPE]);

  /* Init main-list render context. */
  listrc.gc_flags = 0;
  listrc.color_inherit = listrc.trans_inherit = DL_INHERIT_LOCAL;
  init = 1;
  err = 0;

 error:
  SDUNINDENT;
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, err);
  return err;
}

int dl_shutdown(void)
{
  dl_list_t *l, *next;
  int j;

  CHECK_INIT(-1);

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  locklists();
  init = 0;
  for (j=0; j<2; ++j) {
    SDDEBUG("Destroying [%s-lists]\n", typestr[j]);
    SDINDENT;
    for (l=LIST_FIRST(&dl_lists[j]); l; l=next) {
      SDDEBUG("destroying [%p,%d]\n", l, l->refcount);
      next = LIST_NEXT(l,g_list);
      real_destroy(l,0);
    }
    SDUNINDENT;
  }
  unlocklists();

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);
  return 0;
}

dl_list_t * dl_create(int heapsize, int active, int sub)
{
  dl_list_t * l;
  int type;

  CHECK_INIT(0);

  l = calloc(1, sizeof(dl_list_t));
  if (!l) {
    SDERROR("[%s] : alloc error\n", __FUNCTION__);
    return 0;
  }
  if (!heapsize) {
    heapsize = 128;
  }
  l->flags.active = !!active;
  type = l->flags.type = sub ? DL_SUB_TYPE : DL_MAIN_TYPE;

  l->heap = heapsize ? malloc(heapsize) : 0;
  l->heap_size = l->heap ? heapsize : 0;
  l->color.a = l->color.r = l->color.g = l->color.b = 1.0f;
  MtxIdentity(l->trans);
  l->first_comid = DL_COMID_ERROR; /* Set this before dl_clear() */
  real_clear(l);
  l->refcount = 1;
  locklists(type);
  LIST_INSERT_HEAD(&dl_lists[type], l, g_list);
  unlocklists(type);

  return l;
}

static void real_destroy(dl_list_t * l, int force)
{
  if (l->refcount) {
    SDWARNING("[%s] : [%p] refcount = [%d]\n", __FUNCTION__, l, l->refcount);
  }
  if (!l->refcount || force) {
    if (l->heap) free(l->heap);
    if (l) free(l);
  }
}

static dl_code_e sub_render_opaque(void * pcom, dl_context_t * context);

static void real_clear(dl_list_t * dl)
{
  dl_comid_t id;
  dl_command_t * c;
  char * const heap = dl->heap;

  for (c=0, id=dl->first_comid; id != DL_COMID_ERROR; id = c->next_id) {
    c = DLCOM(heap, id);
    /* $$$ Temporary : remove sub-list reference */
    if (c->render_opaque == sub_render_opaque) {
      dl_list_t * subdl = ((struct sublist_command_t*)c)->sublist;
      real_dereference(subdl);
    }
  }
  dl->first_comid = dl->last_comid = DL_COMID_ERROR;
  dl->heap_pos = 0;
  dl->n_commands = 0;
}

void dl_destroy(dl_list_t * l)
{
  SDCRITICAL("[%s] : OBSOLETE !!! DO NOT USE ANY MORE PLEASE !!!");
  BREAKPOINT(0xDEADD1D1);
}

void dl_reference(dl_list_t * dl)
{
  CHECK_INIT();
  locklists();
  ++dl->refcount;
  unlocklists();
}

static int real_dereference(dl_list_t * dl)
{
  int ref;

  ref = --dl->refcount;
  if (ref < 0) {
    SDWARNING("[%s] : [%p] refcount = [%d].\n",__FUNCTION__,dl, ref);
    ref = 0;
  }
  if (!ref) {
    dl->flags.active = 0;
    LIST_REMOVE(dl, g_list);
    real_clear(dl);
    real_destroy(dl,0);
  }
  return ref;
}

int dl_dereference(dl_list_t * dl)
{
  int ref;

  locklists();
  ref = real_dereference(dl);
  unlocklists();
  return ref;
}


int dl_set_active(dl_list_t * l, int active)
{
  int old;

  locklists();
  old = l->flags.active;
  l->flags.active = !!active;
  unlocklists();

  return old;
}

int dl_set_active2(dl_list_t * l1, dl_list_t * l2, int active)
{
  int old;

  locklists();
  old = l1->flags.active | (l2->flags.active<<1);
  l1->flags.active = active;
  l2->flags.active = (active>>1);
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
  if (new_heap) {
    dl->heap = new_heap;
    dl->heap_size = size;
    return 0;
  }
  return -1;
}

/* must be a power of two */
#define MIN_ALLOC 16

static void * real_alloc(dl_list_t * dl, size_t size)
{
  void * r;
  int req;

  size += (-size) & (MIN_ALLOC - 1);
  req = dl->heap_pos + size;
  if (req > dl->heap_size &&
      dl_enlarge(dl, req) < 0) { 
    r = 0;
  } else {
    r = dl->heap + dl->heap_pos;
    dl->heap_pos += size;
  }
  return r;
}

void * dl_alloc(dl_list_t * dl, size_t size)
{
  CHECK_INIT(0);
  locklists();
  /* Do not unlock on ourpose. */
  return real_alloc(dl, size);
}

void dl_clear(dl_list_t * dl)
{
  CHECK_INIT();
  locklists();
  real_clear(dl);
  unlocklists();
}

dl_comid_t dl_insert(dl_list_t * dl, void * pcom,
		     dl_command_func_t o_render, dl_command_func_t t_render)

{
  dl_command_t * com = pcom;
  dl_comid_t lastid;
  dl_comid_t id;

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
 
  unlocklists();
  return id;
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

  if (rc->gc_flags) {
    gc_push(rc->gc_flags);
  }

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
  if (rc->gc_flags) {
    gc_pop(rc->gc_flags);
  }

  return code;
}

static void dl_render(int opaque)
{
  dl_list_t * l, * next;

  CHECK_INIT();
  locklists();
  for (l=LIST_FIRST(&dl_lists[DL_MAIN_TYPE]); l; l=next) {
    next = LIST_NEXT(l,g_list);
    gc_reset();
    dl_render_list(&listrc, 0, l, opaque);
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

static dl_code_e sub_render(struct sublist_command_t * c,
			    dl_context_t * context,
			    int opaque)
{
  dl_code_e code;
  dl_list_t * dl = c->sublist;

  code = dl_render_list(&c->rc, context, dl, opaque);
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

  /* $$$ Trivial circular reference test. We need to be smarter if we want
     to avoid a system crash ! */
  if (dl == sublist) {
    SDERROR("[%s] : Nested lists.\n", __FUNCTION__);
    goto finish;
  }

  locklists();
  if (sublist->flags.type == DL_MAIN_TYPE) {
    /* Change mainlist to sublist type. */
    LIST_REMOVE(sublist, g_list);
    sublist->flags.type = DL_SUB_TYPE;
    LIST_INSERT_HEAD(&dl_lists[DL_SUB_TYPE], sublist, g_list);
  }
  ++sublist->refcount;

  c = real_alloc(dl, sizeof(*c));
  if (c) {
    c->sublist = sublist;
    c->rc = *rc;
    err = dl_insert(dl, c, sub_render_opaque, sub_render_transparent);
  } else {
    --sublist->refcount;
    unlocklists();
  }
 finish:
  return err;
}
