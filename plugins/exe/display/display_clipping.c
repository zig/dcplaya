/**
 * @ingroup  dcplaya_exe_plugin_devel
 * @file     display_clipping.c
 * @author   benjamin gerard
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display_clipping.c,v 1.5 2003-03-26 23:02:50 ben Exp $
 */

#include "display_driver.h"
#include "draw/draw.h"

struct dl_clipping_command {
  dl_command_t uc;
  float x1,y1,x2,y2;
};

static dl_code_e setclip(void * pcom, dl_context_t * context)
{
  struct dl_clipping_command * c = pcom;

  draw_set_clipping4(
	context->trans[0][0] * c->x1 + context->trans[3][0], 
	context->trans[1][1] * c->y1 + context->trans[3][1], 
	context->trans[0][0] * c->x2 + context->trans[3][0], 
	context->trans[1][1] * c->y2 + context->trans[3][1]
	);

  return DL_COMMAND_OK;
}

DL_FUNCTION_START(set_clipping)
{
  struct dl_clipping_command * c;
  float x1, y1, x2, y2;

  x1 = lua_tonumber(L, 2);  /* X1 */
  y1 = lua_tonumber(L, 3);  /* Y1 */
  x2 = lua_tonumber(L, 4);  /* X2 */
  y2 = lua_tonumber(L, 5);  /* Y2 */

  if (x1>=x2) {
	x1 = 0;
	x2 = draw_screen_width;
  }
  if (y1>=y2) {
	y1 = 0;
	y2 = draw_screen_height;
  }

  c = dl_alloc(dl, sizeof(*c));
  if (c) {
	c->x1 = x1;
	c->y1 = y1;
	c->x2 = x2;
	c->y2 = y2;
	dl_insert(dl, c, setclip, setclip);
  }

  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_clipping)
{
  draw_clipbox_t cb; 

  draw_get_clipping(&cb);
  lua_settop(L,1);
  lua_newtable(L);
  lua_pushnumber(L, cb.x1);
  lua_settable(L, 1);
  lua_pushnumber(L, cb.y1);
  lua_settable(L, 1);
  lua_pushnumber(L, cb.x2);
  lua_settable(L, 1);
  lua_pushnumber(L, cb.y2);
  lua_settable(L, 1);

  return 1;
}
DL_FUNCTION_END()

