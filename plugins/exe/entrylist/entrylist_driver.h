/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.h
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_driver.h,v 1.1 2002-10-23 02:07:44 benjihan Exp $
 */

#ifndef _ENTRYLIST_DRIVER_H_
#define _ENTRYLIST_DRIVER_H_

#include <stdio.h>
#include "lua.h"
#include "any_driver.h"

extern int dl_list_tag;
extern any_driver_t display_driver;

#define DRIVER_NAME "entrylist"

#define EL_FUNCTION_DECLARE(name) int lua_entrylist_##name(lua_State * L)

#define EL_FUNCTION_START(name) \
  int lua_entrylist_##name(lua_State * L) \
  { \
    entrylist_t * el; \
    if (lua_tag(L, 1) != entrylist_tag) { \
      printf("el_" #name " : first parameter is not an entry-list\n"); \
      return 0; \
    } \
    dl = lua_touserdata(L, 1);

#define EL_FUNCTION_END() }

#endif /* #define _ENTRYLIST_DRIVER_H_ */
