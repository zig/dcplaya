/**
 * @ingroup  exe_plugin
 * @file     display_triangle.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/17
 * @brief    graphics lua extension plugin, triangle interface
 * 
 * $Id: display_triangle.c,v 1.1 2002-10-18 11:42:07 benjihan Exp $
 */

#include "gp.h"
#include "display_driver.h"

struct dl_draw_triangle_command {
  dl_command_t uc;
  int flags;
  draw_vertex_t v[3];
};

static void dl_draw_triangle_render(const struct dl_draw_triangle_command * c)
{
  draw_vertex_t v[3];
  int i;
  for (i=0; i<3; i++) {
	MtxVectMult(&v[i].x, &c->v[i].x, dl_trans);
	v[i].u = c->v[i].u;
	v[i].v = c->v[i].v;
	v[i].a = dl_color[0] * c->v[i].a;
	v[i].r = dl_color[1] * c->v[i].r;
	v[i].g = dl_color[2] * c->v[i].g;
	v[i].b = dl_color[3] * c->v[i].b;
  }
  draw_triangle(v+0, v+1, v+2, c->flags);
}

static void dl_draw_triangle_render_opaque(void * pcom)
{
  struct dl_draw_triangle_command * c = pcom;
  if (DRAW_OPACITY(c->flags) == DRAW_OPAQUE) {
	dl_draw_triangle_render(c);
  }
}

static void dl_draw_triangle_render_transparent(void * pcom)
{
  struct dl_draw_triangle_command * c = pcom;
  if (DRAW_OPACITY(c->flags) == DRAW_TRANSLUCENT) {
	dl_draw_triangle_render(c);
  }
}

void dl_draw_triangle(dl_list_t * dl,
					  const draw_vertex_t v[3],
					  int flags)
{
  struct dl_draw_triangle_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
	c->v[0] = v[0];
	c->v[1] = v[1];
	c->v[2] = v[2];
	c->flags = flags;
    dl_insert(dl, c,
			  dl_draw_triangle_render_opaque,
			  dl_draw_triangle_render_transparent);
  }
}

DL_FUNCTION_START(draw_triangle)
{
  int i, n = lua_gettop(L);
  int flags = 0; /* => DRAW_NO_TEXTURE | DRAW_TRANSLUCENT | DRAW_GOURAUD */
  draw_vertex_t v[3];


/*   lua_matrix_t *m; */
/*   if ( lua_tag(L, 1) == matrix_tag ) { */
/* 	GET_MATRIX(m,1,0,0); */

  if (n < 10 || n > 24) {
	printf("lua_draw_triangle : bad number of arguments\n");
	return 0;
  }

  /* Got 3 vertrices */
  v[0].x = lua_tonumber(L, 2);
  v[0].y = lua_tonumber(L, 3);
  v[0].z = lua_tonumber(L, 4);
  v[0].w = 1;

  v[1].x = lua_tonumber(L, 5);
  v[1].y = lua_tonumber(L, 6);
  v[1].z = lua_tonumber(L, 7);
  v[1].w = 1;

  v[2].x = lua_tonumber(L, 8);
  v[2].y = lua_tonumber(L, 9);
  v[2].z = lua_tonumber(L, 10);
  v[2].w = 1;

  i = 10;
  if (i+4 < n) {
	/* Got a color */
	v[0].a = lua_tonumber(L, 11);
	v[0].r = lua_tonumber(L, 12);
	v[0].g = lua_tonumber(L, 13);
	v[0].b = lua_tonumber(L, 14);
	i=14;
	if (i+8 < n) {
	  /* Got 2 more colors */
	  v[1].a = lua_tonumber(L, 15);
	  v[1].r = lua_tonumber(L, 16);
	  v[1].g = lua_tonumber(L, 17);
	  v[1].b = lua_tonumber(L, 18);
	  v[2].a = lua_tonumber(L, 19);
	  v[3].r = lua_tonumber(L, 20);
	  v[4].g = lua_tonumber(L, 21);
	  v[5].b = lua_tonumber(L, 22);
	  i = 22;
	}
  }

  if (i+1 < n) {
	/* Got a texture */
	flags |= ((unsigned int)lua_tonumber(L, i+1) << DRAW_TEXTURE_BIT)
				 & DRAW_TEXTURE_MASK;
	if (i+2 < n) {
	  /* Got a opacity mode */
	  flags |= ((unsigned int)lua_tonumber(L, i+2) << DRAW_OPACITY_BIT)
		& DRAW_OPACITY_MASK;
	}
  }

  /* Determine type of shading depending of number of color arguments */
  if (i <= 14) {
	flags |= DRAW_FLAT;
	/* No color : set a default white */
	if (i==10) {
	  v[0].a = v[0].r = v[0].g = v[0].b = 1.0f;
	}
	/* Copy color vertex 0 to 1 and 2 */
	v[1].a = v[2].a = v[0].a;
	v[1].r = v[2].r = v[0].r;
	v[1].g = v[2].g = v[0].g;
	v[1].b = v[2].b = v[0].b;
  }
  dl_draw_triangle(dl, v, flags);
  return 0;
}
DL_FUNCTION_END()

