/**
 * @ingroup  exe_plugin
 * @file     display_clipping.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display_clipping.c,v 1.1 2002-10-18 11:42:07 benjihan Exp $
 */

#include "display_driver.h"

struct dl_clipping_command {
  dl_command_t uc;

  float x1, y1, x2, y2;
};

#define lock(l) spinlock_lock(&(l)->mutex)
#define unlock(l) spinlock_unlock(&(l)->mutex)

DL_FUNCTION_START(set_clipping)
{
  //  lock(dl);
  dl->clip_box[0] = lua_tonumber(L, 2);  /* X1 */
  dl->clip_box[1] = lua_tonumber(L, 3);  /* Y1 */
  dl->clip_box[2] = lua_tonumber(L, 4);  /* X2 */
  dl->clip_box[3] = lua_tonumber(L, 5);  /* Y2 */
  //  unlock(dl);

  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_clipping)
{
  int i;

  lua_settop(L,1);
  lua_newtable(L);
  //  lock(dl);
  for (i=0; i<4; ++i) {
    lua_pushnumber(L, dl->clip_box[i]);
	lua_settable(L, 1);
  }
  //  unlock(dl);
  return 1;
}
DL_FUNCTION_END()


