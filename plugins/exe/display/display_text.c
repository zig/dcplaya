/**
 * @ingroup  exe_plugin
 * @file     display_text.c
 * @author   Vincent Penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, text interface
 * 
 * $Id: display_text.c,v 1.3 2002-11-27 09:58:09 ben Exp $
 */

#include <string.h>

#include "draw/text.h"
#include "display_driver.h"

#include "sysdebug.h"

struct text_command {
  dl_command_t uc;
  float x, y, z, a, r, g, b;
  char text[1];
};

struct textprop_command {
  dl_command_t uc;
  fontid_t font;
  float size;
  float aspect;
};

static dl_code_e text_render_transparent(void * pcom,
										 dl_context_t * context)
{
  struct text_command * c = pcom;  

  ///$$$
  text_set_properties(0,16,1);

  text_set_color(c->a * context->color.a, c->r * context->color.r,
				 c->g * context->color.g, c->b * context->color.b);
  text_draw_str(context->trans[0][0] * c->x + context->trans[3][0],
				context->trans[1][1] * c->y + context->trans[3][1],
				context->trans[2][2] * c->z + context->trans[3][2],
				c->text);
  return DL_COMMAND_OK;
}

static dl_code_e properties_render_transparent(void * pcom,
											   dl_context_t * context)
{
  struct textprop_command * c = pcom;

  text_set_properties(c->font, c->size, c->aspect);
  return DL_COMMAND_OK;
}

static void properties(dl_list_t * dl,
					   fontid_t fontid, const float size, const float aspect)
{
  struct textprop_command * c;

  if (fontid < 0 && size <= 0) {
	return;
  }
  if (c = dl_alloc(dl, sizeof(*c)), c) {
    c->font = fontid;
	c->size = size;
	c->aspect = aspect;
	dl_insert(dl, c, 0, properties_render_transparent);
  }
}

static void text(dl_list_t * dl, 
				 float x, float y, float z,
				 float a, float r, float g, float b,
				 const char * text)
{
  struct text_command * c;
  int len;

  if (!text) {
	return;
  }
  if (len = strlen(text), len < 1) {
	return;
  }
  len += sizeof(*c);
  if (c = dl_alloc(dl, len), c) {
    c->x = x;    c->y = y;    c->z = z;
    c->a = a;    c->r = r;    c->g = g;    c->b = b;
	strcpy(c->text, text);
	/* 	SDDEBUG("[dltext: [%s]]\n",c->text); */
	dl_insert(dl, c, 0, text_render_transparent);
  }
}

DL_FUNCTION_START(draw_text)
{
  text(dl, 
	   lua_tonumber(L, 2),
	   lua_tonumber(L, 3),
	   lua_tonumber(L, 4),
	   lua_tonumber(L, 5),
	   lua_tonumber(L, 6),
	   lua_tonumber(L, 7),
	   lua_tonumber(L, 8),
	   lua_tostring(L, 9)
	   );
  return 0;
}
DL_FUNCTION_END()


DL_FUNCTION_START(measure_text)
{
  float w,h;
  ///$$$
  text_set_properties(0,16,1);

  text_size_str(lua_tostring(L, 2), & w, & h);
  lua_pushnumber(L, w);
  lua_pushnumber(L, h);
  return 2;
}
DL_FUNCTION_END()

DL_FUNCTION_START(text_prop)
{
  int fontid = -1;
  float size = -1;
  float aspect = -1;

  if (lua_type(L,2) == LUA_TNUMBER) {
	fontid = (int)lua_tonumber(L, 2);
  }
  if (lua_type(L,3) == LUA_TNUMBER) {
	size = (int)lua_tonumber(L, 3);
  }
  if (lua_type(L,4) == LUA_TNUMBER) {
	aspect = (int)lua_tonumber(L, 4);
  }

  properties(dl, fontid, size, aspect);
  return 0;
}
DL_FUNCTION_END()
