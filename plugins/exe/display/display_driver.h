/**
 * @ingroup  exe_plugin
 * @file     display_driver.h
 * @author   Vincent Penne <ziggy@sashipa.com>
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display_driver.h,v 1.1 2002-10-18 11:42:07 benjihan Exp $
 */

#ifndef _DISPLAY_DRIVER_H_
#define _DISPLAY_DRIVER_H_

#include <stdio.h>
#include "lua.h"
#include "any_driver.h"
#include "display_list.h"

extern int dl_list_tag;
extern any_driver_t display_driver;

#define DRIVER_NAME "display"

#define DL_FUNCTION_DECLARE(name) int lua_##name(lua_State * L)

#define DL_FUNCTION_START(name) \
  /*static*/ int lua_##name(lua_State * L) \
  { \
    dl_list_t * dl; \
    if (lua_tag(L, 1) != dl_list_tag) { \
      printf("dl_" #name " : first parameter is not a list\n"); \
      return 0; \
    } \
    dl = lua_touserdata(L, 1);

#define DL_FUNCTION_END() }

#endif /* #define _DISPLAY_DRIVER_H_ */
