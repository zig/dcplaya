/**
 * @ingroup  exe_plugin
 * @file     display_text.c
 * @author   Vincent Penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, text interface
 * 
 * $Id: display_text.c,v 1.1 2002-10-18 11:42:07 benjihan Exp $
 */

#include "gp.h"
#include "display_driver.h"

struct dl_draw_text_command {
  dl_command_t uc;
  
  char * text;
  float x, y, z, a, r, g, b;
};

void dl_draw_text_render_transparent(void * pcom)
{
  struct dl_draw_text_command * c = pcom;  

  text_set_color(c->a * dl_color[0], c->r * dl_color[1],
				 c->g * dl_color[2], c->b * dl_color[3]);
  text_draw_str(dl_trans[0][0] * c->x + dl_trans[3][0],
				dl_trans[1][1] * c->y + dl_trans[3][1],
				dl_trans[2][2] * c->z + dl_trans[3][2],
				c->text);
}

void dl_draw_text(dl_list_t * dl, 
		  float x, float y, float z,
		  float a, float r, float g, float b,
		  const char * text)
{
  struct dl_draw_text_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x = x;    c->y = y;    c->z = z;
    c->a = a;    c->r = r;    c->g = g;    c->b = b;
    c->text = dl_alloc(dl, strlen(text)+1);
    if (c->text) {
      strcpy(c->text, text);
      dl_insert(dl, c, 0, dl_draw_text_render_transparent);
    }
  }
}

DL_FUNCTION_START(draw_text)
{
  dl_draw_text(dl, 
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
  text_size_str(lua_tostring(L, 2), & w, & h);
  lua_pushnumber(L, w);
  lua_pushnumber(L, h);
  return 2;
}
DL_FUNCTION_END()
