/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.h
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_driver.h,v 1.4 2002-11-04 22:41:53 benjihan Exp $
 */

#ifndef _ENTRYLIST_DRIVER_H_
#define _ENTRYLIST_DRIVER_H_

#include <stdio.h>
#include "lua.h"
#include "any_driver.h"
#include "allocator.h"
#include "entrylist.h"

/** Entrylist user tag. */
extern int entrylist_tag;
/** Holds all entrylist. */
extern allocator_t * lists;
/** Holds standard entries. */
extern allocator_t * entries;
/** The driver. */
any_driver_t entrylist_driver;

int lua_entrylist_init(lua_State * L);

/** Entrylist driver name. */
#define DRIVER_NAME "entrylist"

#define EL_FUNCTION_DECLARE(name) int lua_entrylist_##name(lua_State * L)

#define EL_FUNCTION_START(name) \
  int lua_entrylist_##name(lua_State * L) \
  { \
    el_list_t * el; \
    if (lua_tag(L, 1) != entrylist_tag) { \
      printf("el_" #name " : first parameter is not an entry-list\n"); \
      return 0; \
    } \
    if (el = lua_touserdata(L, 1), !el) { \
      printf("el_" #name " : Null pointer.\n"); \
      return 0; \
    }

#define EL_FUNCTION_END() }

#define GET_ENTRYLIST(EL,N) \
    if (lua_tag(L, N) != entrylist_tag) { \
      printf("%s : parameter #%d is not an entry-list\n",__FUNCTION__, N); \
      return 0; \
    } \
    if ((EL) = lua_touserdata(L, 1), !(EL)) { \
      printf("%s : parameter #%d, null pointer.\n", __FUNCTION__, N); \
      return 0; \
    }

#endif /* #define _ENTRYLIST_DRIVER_H_ */
