/**
 * @ingroup  exe_plugin
 * @file     display_box.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, box functions
 * 
 * $Id: display_color.c,v 1.1 2002-11-27 09:58:09 ben Exp $
 */

#include "draw/color.h"
#include "display_driver.h"

struct color1_com {
  dl_command_t uc;
  draw_color_t color[1];
};

struct color2_com {
  dl_command_t uc;
  draw_color_t color[2];
};

struct color4_com {
  dl_command_t uc;
  draw_color_t color[4];
};

static dl_code_e render(const struct color4_com * c, const int n,
						const dl_context_t * context)
{
  draw_color_t color;
  int i;

  for (i=0; i<n; ++i) {
	draw_color_mul_clip(&color, &context->color, c->color + i);
	draw_set_color(i, &color);
  }
  return DL_COMMAND_OK;
}


static dl_code_e dl_color1_render(void * pcom, dl_context_t * context)
{
  return render((struct color4_com *)pcom,1,context);
}

static dl_code_e dl_color2_render(void * pcom, dl_context_t * context)
{
  return render((struct color4_com *)pcom,2,context);
}

static dl_code_e dl_color4_render(void * pcom, dl_context_t * context)
{
  return render((struct color4_com *)pcom,4,context);
}

static void setcolor1(dl_list_t * dl, const draw_color_t * color)
{
  struct color1_com * c;

  if(c = dl_alloc(dl, sizeof(*c)), !c) {
	return;
  }
  c->color[0] = color[0];
  dl_insert(dl, c, dl_color1_render, dl_color1_render);
}

static void setcolor2(dl_list_t * dl, const draw_color_t * color)
{
  struct color2_com * c;

  if(c = dl_alloc(dl, sizeof(*c)), !c) {
	return;
  }
  c->color[0] = color[0];
  c->color[1] = color[1];
  dl_insert(dl, c, dl_color2_render, dl_color2_render);
}

static void setcolor4(dl_list_t * dl, const draw_color_t * color)
{
  struct color4_com * c;

  if(c = dl_alloc(dl, sizeof(*c)), !c) {
	return;
  }
  c->color[0] = color[0];
  c->color[1] = color[1];
  c->color[2] = color[2];
  c->color[3] = color[3];
  dl_insert(dl, c, dl_color4_render, dl_color4_render);
}

static void getcolor_from_float(draw_color_t * c, lua_State * L, const int i)
{
  c->a = lua_tonumber(L, i + 0); /* A  */
  c->r = lua_tonumber(L, i + 1); /* R  */
  c->b = lua_tonumber(L, i + 2); /* G  */
  c->b = lua_tonumber(L, i + 3); /* B  */
}

static void getcolor_from_table(draw_color_t * c, lua_State * L, const int i)
{
  lua_rawgeti(L,i,1);
  c->a = lua_tonumber(L,-1);
  lua_rawgeti(L,i,2);
  c->r = lua_tonumber(L,-1);
  lua_rawgeti(L,i,3);
  c->g = lua_tonumber(L,-1);
  lua_rawgeti(L,i,4);
  c->b = lua_tonumber(L,-1);
  lua_pop(L,4);
}

static int getcolor(draw_color_t * c, lua_State * L, int i)
{
  switch(lua_type(L, i)) {
  case LUA_TTABLE:
	getcolor_from_table(c, L, i);
	return i+1;
  case LUA_TNUMBER:
	getcolor_from_float(c, L, i);
	return i+4;
  }
  return 0;
}

DL_FUNCTION_START(setcolor)
{
  int i = 0;
  int j = 2;
  int n = lua_gettop(L);
  draw_color_t colors[4];

  while (i<4 && j && j<=n) {
	j = getcolor(colors + i++, L, j);
  }

  switch(i) {
  case 1:
	setcolor1(dl,colors);
	break;
  case 2:
	setcolor2(dl,colors);
	break;
  case 4:
	setcolor4(dl,colors);
	break;
  }
	
  return 0;
}
DL_FUNCTION_END()

