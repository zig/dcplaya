/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_driver.c,v 1.1 2002-10-23 02:07:44 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "luashell.h"
#include "lef.h"
#include "driver_list.h"

#include "entrylist_driver.h"
#include "iarray.h"

EL_FUNCTION_DECLARE(new);
EL_FUNCTION_DECLARE(lock);
EL_FUNCTION_DECLARE(unlock);

typedef struct {
  iarray_t a;
} el_list_t;

int entrylist_tag; /* entrylist user tag */
static iarray_t lists;    /* hold all entrylist */
static int init;   /* lus side init flags */

/* The alloc function for lists : alloc el_list_t only ! */
static void * lists_alloc(unsigned int size, void * cookie)
{
  el_list_t * el;
  
  if (cookie != &lists) {
	printf("entrylists_alloc : Not my list !\n");
	return 0;
  }
  if (size != sizeof(el_list_t)) {
	printf("entrylists_alloc : Size does not match %d differs from %d\n",
		   size, sizeof(el_list_t));
	return 0;
  }
  el = malloc(size);
  if (el) {
	iarray_create(&el->a, 0, 0, cookie); /* $$$ not sure for the cookie */
  }
  return el;
}

/* The free function for lists : destroy el_list_t and free it. */
static void lists_free(void * addr, void * cookie)
{
  if (cookie != &lists) {
	printf("entrylists_free : Not my list !\n");
  } else if (!addr) {
	printf("entrylists_free : Null pointer !\n");
  } else {
	el_list_t * el = (el_list_t *) addr;
	if (!mutex_trylock(&el->a.mutex)) {
	  printf("entrylists_free : destroying a locked entrylist !!!\n");
	}
	iarray_destroy(&el->a);
	free(el);
  }
}

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
}

/* Shutdown  entrylist user tag. */
static void lua_shutdown_el_type(lua_State * L)
{
  lua_pushnil(L);
  lua_setglobal(L,"entrylist_tag");
  entrylist_tag = 0;
}

/* Driver init : not much to do. */ 
static int driver_init(any_driver_t *d)
{
  entrylist_tag = -1;
  init = 0;

  return 0;
}

/* Driver shutdown : Kill any remaining list any way */
static int driver_shutdown(any_driver_t * d)
{
  iarray_destroy(&lists);
  return 0;
}



static driver_option_t * driver_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}


static int lua_entrylist_init(lua_State * L)
{
  if (init) {
    goto ok;
  }

  if (iarray_create(&lists, lists_alloc, lists_free, &lists)) {
	printf("entrylist_driver_init : list container creation failed.\n");
	return 0;
  }

  lua_init_el_type(L);
  init = 1;
  printf("entrylist_driver initialized.\n");
ok:
  lua_settop(L,1);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_entrylist_shutdown(lua_State * L)
{
  int force;

  if (!init) {
	goto ok;
  }
  iarray_lock(&lists);
  force = lua_tonumber(L,1);
  if (lists.n)  {
	if (!force) {
	  printf("entrylist_shutdown : %d entrylist in used. "
			 "May be dangerous to shutdown.\n"
			 "Use force option to shutdown anyway.\n", lists.n);
	  return 0;
	} else {
	  printf("entrylist_shutdown : %d entrylist in used. "
			 "Force shutdown !!!\n", lists.n);
	}
  }
  iarray_destroy(&lists);
  init = 0;
  lua_shutdown_el_type(L);
  iarray_unlock(&lists);
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

  {0},                                       /* end of the command list */
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
