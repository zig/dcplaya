/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_driver.c,v 1.10 2003-04-05 16:33:31 ben Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "dcplaya/config.h"
#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "entrylist_driver.h"
#include "entrylist_path.h"
#include "entrylist_loader.h"


EL_FUNCTION_DECLARE(new);
EL_FUNCTION_DECLARE(lock);
EL_FUNCTION_DECLARE(unlock);
EL_FUNCTION_DECLARE(gc);
EL_FUNCTION_DECLARE(gettable);
EL_FUNCTION_DECLARE(settable);
EL_FUNCTION_DECLARE(clear);
EL_FUNCTION_DECLARE(load);
EL_FUNCTION_DECLARE(sort);
EL_FUNCTION_DECLARE(dump);

/** Entrylist user tag. */
int entrylist_tag;
/** Holds all entrylist. */
allocator_t * lists;
/** Holds standard entries. */
allocator_t * entries;
/** Lua side init fkags. */
static int init;

/* Iniatilize entrylist user tag. */
static void lua_init_el_type(lua_State * L)
{
  /* $$$ ben : Don't do that ! It may crash when loading a new driver and some
     entrylist still alive from the previous one.
  */
  /*   lua_getglobal(L,"entrylist_tag"); */
  /*   entrylist_tag = lua_tonumber(L,1); */
  /*   if (!entrylist_tag) { */
  entrylist_tag = lua_newtag(L);
  lua_pushnumber(L,entrylist_tag);
  lua_setglobal(L,"entrylist_tag");
  /*   } */
  
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

static int driver_shutdown(any_driver_t * d);

/* Driver init : not much to do. */ 
static int driver_init(any_driver_t *d)
{
  printf("%s_driver_init ... \n", d->name);
  entrylist_tag = -1;
  lists = 0;
  entries = 0;
  init = 0;

  if (elpath_init()) {
    goto error;
  }
  lists = allocator_create(8, sizeof(el_list_t),"el_lists");
  if (!lists) {
    printf(DRIVER_NAME "_driver_init : list allocator creation failed.\n");
    goto error;
  }
  entries = allocator_create(65536 / sizeof(el_entry_t), sizeof(el_entry_t),
			     "el_entries");
  if (!entries) {
    printf(DRIVER_NAME "_driver_init : entry allocator creation failed.\n");
    goto error;
  }
  if (el_loader_init() < 0) {
    goto error;
  }
  return 0;

 error:
  driver_shutdown(d);
  return -1;
}

/* Driver shutdown : Kill any remaining list any way */
static int driver_shutdown(any_driver_t * d)
{
  el_loader_shutdown();
  if (lists) {
    allocator_destroy(lists);
    lists = 0;
  }
  if (entries) {
    allocator_destroy(entries);
    entries = 0;
  }
  elpath_shutdown();

  return 0;
}

static driver_option_t * driver_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}


int lua_entrylist_init(lua_State * L)
{
  if (init) {
    goto ok;
  }
  lua_init_el_type(L);
  printf("entrylist_driver initialized.\n");
  init = 1;
 ok:
  lua_settop(L,1);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_entrylist_shutdown(lua_State * L)
{
  int n;

  if (!init) {
    goto ok;
  }
  if (n=allocator_count_used(lists), n)  {
    printf("entrylist_shutdown : %d entrylist in used. "
	   "Should be OK anyway ...\n", n);
  }
  lua_shutdown_el_type(L);
  init = 0;
  printf("entrylist_driver shutdown.\n");
 ok:
  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

static luashell_command_description_t driver_commands[] = {

  /* Internal commands */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_driver_init", 0, 0,
    /* usage */
    DRIVER_NAME"_driver_init() : "
    "INTERNAL ; initialize lua side of " DRIVER_NAME " driver.",
    /* function */
    SHELL_COMMAND_C, lua_entrylist_init
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_driver_shutdown", 0, 0,
    /* usage */
    DRIVER_NAME"_driver_shutdown() : "
    "INTERNAL ; shutdown lua side of " DRIVER_NAME " driver.",
    /* function */
    SHELL_COMMAND_C, lua_entrylist_shutdown
  },

  /* Creation command. */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_new", "el_new", 0,
    /* usage */
    DRIVER_NAME"_new() : "
    "Create a new empty entry-list.",
    /* function */
    SHELL_COMMAND_C, lua_entrylist_new
  },

  /* Lock commands. */

  {
    /* long names, short names and topic */
    DRIVER_NAME"_lock", "el_lock", 0,
    /* usage */
    DRIVER_NAME"_lock(entrylist) : "
    "Recursive lock of an entry-list. CAUTION : do not forget to unlock"
    " the entry-list as many times it has been locked.",
    /* function */
    SHELL_COMMAND_C, lua_entrylist_lock
  },

  {
    /* long names, short names and topic */
    DRIVER_NAME"_unlock", "el_unlock", 0,
    /* usage */
    DRIVER_NAME"_unlock(entrylist) : "
    "Unlock an entry-list. CAUTION : Entry-list must be unlocked as many times"
    " it has been locked. Not more, not less !",
    /* function */
    SHELL_COMMAND_C, lua_entrylist_unlock 
  },

  /* clear command */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_clear", "el_clear", 0,
    /* usage */
    DRIVER_NAME"_clear(entrylist) : "
    "Clear an entry-list : remove all entries. Keep path and loading stat.",
    /* function */
    SHELL_COMMAND_C, lua_entrylist_clear
  },

  /* load dir command */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_load", "el_load", 0,
    /* usage */
    DRIVER_NAME"_load(entrylist, path [,filter]) : "
    "Load a directory into entry-list.\n"
    "filter is a string composed of control characters. Lowercase/uppercase "
    "chars respectively reject/accept a major filetype.\n"
    " <d> dir\n"
    " <f> file\n"
    " <x> plugin\n"
    " <i> image\n"
    " <m> music\n"
    " <p> playlist\n"
    " <l> lua\n"
    " <t> text\n"
    " <a> all types",
    /* function */   
    SHELL_COMMAND_C, lua_entrylist_load
  },

  /* sort command */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_sort", "el_sort", 0,
    /* usage */
    DRIVER_NAME"_sort(entrylist [,order [,start [,len  ] ] ]) : "
    "Sort an entry-list in given order.\n"
    "order string should be composed with chars among :\n"
    " <f> file name (leaf)\n"
    " <n> entry name (leaf)\n"
    " <p> path (full path name)\n"
    " <s> size\n"
    " <t> type\n"
    "Uppercase letters invert sorting direction.",
    /* function */   
    SHELL_COMMAND_C, lua_entrylist_sort
  },

  /* dump command */
  {
    /* long names, short names and topic */
    DRIVER_NAME"_dump", "el_dump", 0,
    /* usage */
    DRIVER_NAME"_dump(entrylist [,level]) : "
    "Dump an entry-list.\n",
    /* function */   
    SHELL_COMMAND_C, lua_entrylist_dump
  },

 
  {0},                                    /* end of the command list */
};

any_driver_t entrylist_driver =
  {

    0,                     /**< Next driver                     */
    EXE_DRIVER,            /**< Driver type                     */      
    0x0100,                /**< Driver version                  */
    DRIVER_NAME,           /**< Driver name                     */
    "Benjamin Gerard",     /**< Driver authors                  */
    "Entry-list LUA "      /**< Description                     */
    "extension provides "
    "function for loading "
    "directory in a "
    "separate thread.",
    0,                     /**< DLL handler                     */
    driver_init,          /**< Driver init                     */
    driver_shutdown,      /**< Driver shutdown                 */
    driver_options,       /**< Driver options                  */
    driver_commands,      /**< Lua shell commands              */
  
  };

EXPORT_DRIVER(entrylist_driver)
