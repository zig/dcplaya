/**
 * @ingroup  exe_plugin
 * @file     display_strip.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/17
 * @brief    graphics lua extension plugin, strip interface
 * 
 * $Id: display_strip.c,v 1.5 2003-03-10 22:55:33 ben Exp $
 */

#include "dcplaya/config.h"

#include "draw/gc.h"
#include "draw/primitives.h"

#include "display_driver.h"
#include "display_matrix.h"

#include "sysdebug.h"

struct dl_draw_strip_command {
  dl_command_t uc;
  int flags;
  int n;
  draw_vertex_t v[256];
};

static void dl_draw_strip_render(const struct dl_draw_strip_command * c,
									  const dl_context_t * context)
{
  draw_vertex_t v[256];
  int i;

  for (i=0; i<c->n; i++) {
	MtxVectMult(&v[i].x, &c->v[i].x, context->trans);
	v[i].u = c->v[i].u;
	v[i].v = c->v[i].v;
	v[i].a = context->color.a * c->v[i].a;
	v[i].r = context->color.r * c->v[i].r;
	v[i].g = context->color.g * c->v[i].g;
	v[i].b = context->color.b * c->v[i].b;
  }
  draw_strip(v, c->n, c->flags);
}

static dl_code_e dl_draw_strip_render_opaque(void * pcom,
											 dl_context_t * context)
{
  struct dl_draw_strip_command * c = pcom;
  if (DRAW_OPACITY(c->flags) == DRAW_OPAQUE) {
	dl_draw_strip_render(c, context);
  }
  return DL_COMMAND_OK;
}

static dl_code_e dl_draw_strip_render_transparent(void * pcom,
												  dl_context_t * context)
{
  struct dl_draw_strip_command * c = pcom;
  if (DRAW_OPACITY(c->flags) == DRAW_TRANSLUCENT) {
	dl_draw_strip_render(c, context);
  }
  return DL_COMMAND_OK;
}

void dl_draw_strip(dl_list_t * dl,
				   const draw_vertex_t v[256],
				   int n,
				   int flags)
{
  struct dl_draw_strip_command * c = dl_alloc(dl,
											  sizeof(*c)
											  - sizeof(c->v)
											  + n * sizeof(c->v[0]));

/*   SDDEBUG("[%s] %d %x\n", __FUNCTION__, n, flags); */

  if (c) {
	int i;
	c->n = n;
	c->flags = flags;
	for (i=0; i<n; ++i) {
	  c->v[i] = v[i];
/* 	  SDDEBUG("%02d %f %f %f %f\n\n", i, v[i].x, v[i].y, v[i].w, v[i].w); */
	}
    dl_insert(dl, c,
			  dl_draw_strip_render_opaque,
			  dl_draw_strip_render_transparent);
  }
}

/* i: table index in stack */
static int vertex_from_table(draw_vertex_t *v, lua_State * L, int i)
{
  int j;
  int top = lua_gettop(L);
  int n = lua_getn(L,i);
  float *f = &v->x;

  if (n > 4 + 4 + 2) {
	n = 4 + 4 + 2;
  }

  for (j=1; j<n; ++j) {
	lua_rawgeti(L,i,j);
	*f++ = lua_tonumber(L,-1);
  }
  lua_settop(L,top);
  return n;
}

static int vertex_from_floats(draw_vertex_t *v, const float *s, int n)
{
  const float * d = s;

  if (n >= 4) {
	v->x = *s++;  v->y = *s++;  v->z = *s++;  v->w = *s++;
	n -= 4;
  }
  if (n >= 4) {
	v->a = *s++;  v->r = *s++;  v->g = *s++;  v->b = *s++;
	n -= 4;
  } else {
	v->a = 1;  v->r = 1;  v->g = 1;  v->b = 1;
  }
  if (n >= 2) {
	v->u = *s++;  v->v = *s++;
  } else {
	v->u = v->v = 0;
  }

  return s-d;
} 

DL_FUNCTION_START(draw_strip)
{
  int i, n = lua_gettop(L);
  int flags = 0; /* => DRAW_NO_TEXTURE | DRAW_TRANSLUCENT | DRAW_GOURAUD */
  draw_vertex_t v[256];
  int nv = 0;

  if (lua_tag(L, 2) == matrix_tag) {
	lua_matrix_t *m;
	lua_matrix_def_t *md;
	int l, j;
	const float * li;
	GET_MATRIX_OR_VECTOR(m,md,2,0);
	  
	li = m->li;
	if (li) {
	  l  = (li - md->v) / md->c;
	  nv = lua_tonumber(L,3);
	  i  = 4;
	} else {
	  li = md->v;
	  l  = 0;
	  nv = md->l;
	  i  = 3;
	}
	if (nv > 256) {
	  nv = 256;
	}

	if (l+nv > md->l) {
	  printf("%s : missing matrix line.\n", __FUNCTION__);
	  return 0;
	}
	for (j=0; j<nv; ++j, li += md->c) {
	  vertex_from_floats(v+j, li, md->c);
	}
  } else if (lua_type(L,1) == LUA_TTABLE) {
	printf("%s : table not implemented\n", __FUNCTION__);
	return 0;
  } else {
	printf("%s : bad arguments\n", __FUNCTION__);
	return 0;
  }

  if (nv<3) {
	printf("%s : not enought vertrices (%d)\n", __FUNCTION__, nv);
	return 0;
  }

  if (n >= i) {
	/* Got a texture */
	flags |= ((unsigned int)lua_tonumber(L, i) << DRAW_TEXTURE_BIT)
				 & DRAW_TEXTURE_MASK;
	if (n >= i+1) {
	  /* Got a opacity mode */
	  flags |= ((unsigned int)lua_tonumber(L, i+1) << DRAW_OPACITY_BIT)
		& DRAW_OPACITY_MASK;

	  if (n >= i+2) {
		/* Got a filter mode */
		flags |= ((unsigned int)lua_tonumber(L, i+2) << DRAW_FILTER_BIT)
		  & DRAW_FILTER_MASK;
	  }
	}
  }

  dl_draw_strip(dl, v, nv, flags);
  return 0;
}
DL_FUNCTION_END()
