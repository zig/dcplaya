/**
 * @ingroup  dcplaya_el_exe_plugin_devel
 * @file     entrylist_driver.h
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/23
 * @brief    entry-list lua extensions.
 * 
 * $Id: entrylist_driver.h,v 1.5 2003-03-19 05:16:16 ben Exp $
 */

#ifndef _ENTRYLIST_DRIVER_H_
#define _ENTRYLIST_DRIVER_H_

/** @defgroup  dcplaya_el_exe_plugin_devel entry-list lua extensions
 *  @ingroup   dcplaya_exe_plugin_devel
 *  @brief     entry-list lua extensions.
 *
 *    entry-list plugin adds LUA extensions to support multi-threaded
 *    directory content reading.
 *
 *  @author    Benjamin Gerard <ben@sashipa.com>
 */

#include <stdio.h>
#include "lua.h"
#include "any_driver.h"
#include "allocator.h"
#include "entrylist.h"

/** entry-list user tag.
 *  @ingroup dcplaya_el_exe_plugin_devel
 */
extern int entrylist_tag;

/** Holds all entrylist.
 *  @ingroup dcplaya_el_exe_plugin_devel
 */
extern allocator_t * lists;

/** Holds standard entries.
 *  @ingroup dcplaya_el_exe_plugin_devel
 */
extern allocator_t * entries;

/** The driver.
 *  @ingroup dcplaya_el_exe_plugin_devel
 */
any_driver_t entrylist_driver;

int lua_entrylist_init(lua_State * L);

/** entry-list driver name.
 *  @ingroup dcplaya_el_exe_plugin_devel
 */
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
