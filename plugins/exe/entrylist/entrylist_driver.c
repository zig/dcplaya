/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_driver.c,v 1.2 2002-10-24 18:57:07 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "entrylist_driver.h"
#include "entrylist_loader.h"


EL_FUNCTION_DECLARE(new);
EL_FUNCTION_DECLARE(lock);
EL_FUNCTION_DECLARE(unlock);
EL_FUNCTION_DECLARE(gc);
EL_FUNCTION_DECLARE(gettable);
EL_FUNCTION_DECLARE(settable);
EL_FUNCTION_DECLARE(clear);
EL_FUNCTION_DECLARE(load);

/**< Entrylist user tag. */
int entrylist_tag;
/**< Holds all entrylist. */
allocator_t * lists;
/**< Holds standard entries. */
allocator_t * entries;

/* Iniatilize entrylist user tag. */
static void lua_init_el_type(lua_State * L)
{
  lua_getglobal(L,"entrylist_tag");
  entrylist_tag = lua_tonumber(L,1);
  if (!entrylist_tag) {
	entrylist_tag = lua_newtag(L);
	lua_pushnumber(L,entrylist_tag);
	lua_setglobal(L,"entrylist_tag");
  }
  
  /* Setup tag functions */
  lua_pushcfunction(L, lua_entrylist_gc);
  lua_settagmethod(L, entrylist_tag, "gc");
  
  lua_pushcfunction(L, lua_entrylist_gettable);
  lua_settagmethod(L, entrylist_tag, "gettable");

  lua_pushcfunction(L, lua_entrylist_settable);
  lua_settagmethod(L, entrylist_tag, "settable");
}

/* Shutdown  entrylist user tag. */
static void lua_shutdown_el_type(lua_State * L)
{
  /* Unset tag functions */
  lua_pushnil(L);
  lua_settagmethod(L, entrylist_tag, "gc");
  
  lua_pushnil(L);
  lua_settagmethod(L, entrylist_tag, "gettable");

  lua_pushnil(L);
  lua_settagmethod(L, entrylist_tag, "settable");

  /* Unset entrylist user tag */
  lua_pushnil(L);
  lua_setglobal(L,"entrylist_tag");
  entrylist_tag = 0;
}

/* Driver init : not much to do. */ 
static int driver_init(any_driver_t *d)
{
  printf("driver_init(%s)\n", d->name);
  entrylist_tag = -1;
  lists = 0;
  entries = 0;
  return el_loader_init();
}

/* Driver shutdown : Kill any remaining list any way */
static int driver_shutdown(any_driver_t * d)
{
  el_loader_shutdown();
  if (lists) {
	printf(DRIVER_NAME "_driver_shutdown : lua side has not been"
		   " shutdown correctly.\n");
	allocator_destroy(lists);
	lists = 0;
  }
  if (entries) {
	allocator_destroy(entries);
	entries = 0;
  }
  return 0;
}

static driver_option_t * driver_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}


int lua_entrylist_init(lua_State * L)
{
  if (lists) {
    goto ok;
  }

  lists = allocator_create(8, sizeof(el_list_t));
  if (!lists) {
	printf("entrylist_driver_init : list allocator creation failed.\n");
	return 0;
  }
  entries = allocator_create(65536 / sizeof(el_entry_t), sizeof(el_entry_t));
  if (!entries) {
	printf("entrylist_driver_init : entry allocator creation failed.\n");
	allocator_destroy(lists);
	lists = 0;
	return 0;
  }

  lua_init_el_type(L);
  printf("entrylist_driver initialized.\n");
ok:
  lua_settop(L,1);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_entrylist_shutdown(lua_State * L)
{
  int force, n;

  if (!lists) {
	goto ok;
  }
  force = lua_tonumber(L,1);
  if (n=allocator_count_used(lists), n)  {
	if (!force) {
	  printf("entrylist_shutdown : %d entrylist in used. "
			 "May be dangerous to shutdown.\n"
			 "Use force option to shutdown anyway.\n", n);
	  return 0;
	} else {
	  printf("entrylist_shutdown : %d entrylist in used. "
			 "Force shutdown !!!\n", n);
	}
  }
  allocator_destroy(lists);
  allocator_destroy(entries);
  lists = 0;
  entries = 0;
  lua_shutdown_el_type(L);
  printf("entrylist_driver shutdown.\n");
 ok:
  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

static luashell_command_description_t driver_commands[] = {

  /* Internal commands */
  {
    DRIVER_NAME"_driver_init", 0,         /* long and short names */
    "print [["
	DRIVER_NAME"_driver_init : "
	"INTERNAL ; initialize lua side of " DRIVER_NAME " driver."
    "]]",                                 /* usage */
    SHELL_COMMAND_C, lua_entrylist_init   /* function */
  },

  {
    DRIVER_NAME"_driver_shutdown", 0,        /* long and short names */
    "print [["
	DRIVER_NAME"_driver_shutdown : "
	"INTERNAL ; shutdown lua side of " DRIVER_NAME " driver."
    "]]",                                    /* usage */
    SHELL_COMMAND_C, lua_entrylist_shutdown  /* function */
  },

  /* Creation command. */
  {
    DRIVER_NAME"_new", 0,         /* long and short names */
    "print [["
	DRIVER_NAME"_new : "
	"Create a new empty entry-list."
    "]]",                                 /* usage */
    SHELL_COMMAND_C, lua_entrylist_new    /* function */
  },

  /* Lock commands. */

  {
    DRIVER_NAME"_lock", 0,                /* long and short names */
    "print [["
	DRIVER_NAME"_lock(entrylist) : "
	"Recursive lock of an entry-list. CAUTION : do not forget to unlock"
	" the entry-list as many times it has been locked."
    "]]",                                 /* usage */
    SHELL_COMMAND_C, lua_entrylist_lock   /* function */
  },

  {
    DRIVER_NAME"_unlock", 0,              /* long and short names */
    "print [["
	DRIVER_NAME"_unlock(entrylist) : "
	"Unlock an entry-list. CAUTION : Entry-list must be unlocked as many times"
	" it has been locked. Not more, not less !"
    "]]",                                 /* usage */
    SHELL_COMMAND_C, lua_entrylist_unlock /* function */
  },

  /* clear command */
  {
    DRIVER_NAME"_clear", 0,                /* long and short names */
    "print [["
	DRIVER_NAME"_clear(entrylist) : "
	"Clear an entry-list : remove all entries. Keep path and loading stat."
    "]]",                                 /* usage */
    SHELL_COMMAND_C, lua_entrylist_clear  /* function */
  },

  /* load dir command */
  {
    DRIVER_NAME"_load", 0,                /* long and short names */
    "print [["
	DRIVER_NAME"_load(entrylist, path) : "
	"Load a directory into entry-list."
    "]]",                                 /* usage */
    SHELL_COMMAND_C, lua_entrylist_load   /* function */
  },

 
  {0},                                    /* end of the command list */
};

static any_driver_t entrylist_driver =
{

  0,                     /**< Next driver                     */
  EXE_DRIVER,            /**< Driver type                     */      
  0x0100,                /**< Driver version                  */
  DRIVER_NAME,           /**< Driver name                     */
  "Benjamin Gerard",     /**< Driver authors                  */
  "Entry-list LUA "      /**< Description                     */
  "extension ",
  0,                     /**< DLL handler                     */
  driver_init,          /**< Driver init                     */
  driver_shutdown,      /**< Driver shutdown                 */
  driver_options,       /**< Driver options                  */
  driver_commands,      /**< Lua shell commands              */
  
};

EXPORT_DRIVER(entrylist_driver)
