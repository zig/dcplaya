/**
 * @ingroup  exe_plugin
 * @file     display_clipping.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display_clipping.c,v 1.3 2002-11-27 09:58:09 ben Exp $
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
#if 0
  dl->clip_box.x1 = lua_tonumber(L, 2);  /* X1 */
  dl->clip_box.y1 = lua_tonumber(L, 3);  /* Y1 */
  dl->clip_box.x2 = lua_tonumber(L, 4);  /* X2 */
  dl->clip_box.y2 = lua_tonumber(L, 5);  /* Y2 */
#endif
  //  unlock(dl);

  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_clipping)
{
  int i;

  lua_settop(L,1);
  lua_newtable(L);
#if 0
  //  lock(dl);
  lua_pushnumber(L, dl->clip_box.x1);
  lua_settable(L, 1);
  lua_pushnumber(L, dl->clip_box.y1);
  lua_settable(L, 1);
  lua_pushnumber(L, dl->clip_box.x2);
  lua_settable(L, 1);
  lua_pushnumber(L, dl->clip_box.y2);
  lua_settable(L, 1);
#endif
  //  unlock(dl);
  return 1;
}
DL_FUNCTION_END()


