/**
 * @ingroup  exe_plugin
 * @file     display_triangle.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/17
 * @brief    graphics lua extension plugin, triangle interface
 * 
 * $Id: display_triangle.c,v 1.6 2003-03-10 22:55:33 ben Exp $
 */

#include "dcplaya/config.h"
#include "draw/primitives.h"
#include "display_driver.h"
#include "display_matrix.h"

struct triangle_command {
  dl_command_t uc;
  int flags;
  draw_vertex_t v[3];
};

static void triangle_render(const struct triangle_command * c,
							dl_context_t * context)
{
  draw_vertex_t v[3];
  int i;
  for (i=0; i<3; i++) {
	MtxVectMult(&v[i].x, &c->v[i].x, context->trans);
	v[i].u = c->v[i].u;
	v[i].v = c->v[i].v;
	v[i].a = context->color.a * c->v[i].a;
	v[i].r = context->color.r * c->v[i].r;
	v[i].g = context->color.g * c->v[i].g;
	v[i].b = context->color.b * c->v[i].b;
  }
  draw_triangle(v+0, v+1, v+2, c->flags);
}

static dl_code_e triangle_render_opaque(void * pcom,
										dl_context_t * context)
{
  struct triangle_command * c = pcom;
  if (DRAW_OPACITY(c->flags) == DRAW_OPAQUE) {
	triangle_render(c, context);
  }
  return DL_COMMAND_OK;
}

static dl_code_e triangle_render_transparent(void * pcom,
											 dl_context_t * context)
{
  struct triangle_command * c = pcom;
  if (DRAW_OPACITY(c->flags) == DRAW_TRANSLUCENT) {
	triangle_render(c, context);
  }
  return DL_COMMAND_OK;
}

void triangle(dl_list_t * dl,
			  const draw_vertex_t v[3],
			  int flags)
{
  struct triangle_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
	c->v[0] = v[0];
	c->v[1] = v[1];
	c->v[2] = v[2];
	c->flags = flags;
    dl_insert(dl, c,
			  triangle_render_opaque,
			  triangle_render_transparent);
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

DL_FUNCTION_START(draw_triangle)
{
  int i, n = lua_gettop(L);
  int flags = 0; /* => DRAW_NO_TEXTURE | DRAW_TRANSLUCENT | DRAW_GOURAUD */
  draw_vertex_t v[3];
  int n1,n2,n3;

  if (lua_tag(L, 2) == matrix_tag) {
	lua_matrix_t *m1, *m2, *m3;
	lua_matrix_def_t *md1, *md2, *md3;
	if (lua_tag(L, 3) == matrix_tag) {
	  GET_VECTOR(m1,md1,2,0);
	  GET_VECTOR(m2,md2,3,0);
	  GET_VECTOR(m3,md3,4,0);
	  n1 = vertex_from_floats(v+0, m1->li, md1->c);
	  n2 = vertex_from_floats(v+1, m2->li, md2->c);
	  n3 = vertex_from_floats(v+2, m3->li, md3->c);
	  i = 5;
	} else {
	  int l;
	  const float * li;
	  GET_MATRIX_OR_VECTOR(m1,md1,2,0);
	  
	  li = m1->li;
	  if (li) {
		l = (li - md1->v) / md1->c;
	  } else {
		li = md1->v;
		l  = 0;
	  }
	  if (l+3 > md1->l) {
		printf("%s : missing matrix line.\n", __FUNCTION__);
		return 0;
	  }
	  n1 = vertex_from_floats(v+0, li, md1->c);
	  n2 = vertex_from_floats(v+1, li += md1->c, md1->c);
	  n3 = vertex_from_floats(v+2, li += md1->c, md1->c);
	  i = 3;
	}
  } else if (lua_type(L,1) == LUA_TTABLE) {
	printf("%s : table not implemented\n", __FUNCTION__);
	return 0;
  } else {
	printf("%s : bad arguments\n", __FUNCTION__);
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

  triangle(dl, v, flags);
  return 0;
}
DL_FUNCTION_END()
