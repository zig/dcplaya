/**
 * @ingroup  exe_plugin
 * @file     display.c
 * @author   Vincent Penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display.c,v 1.1 2002-09-25 21:36:45 vincentp Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lua.h"
#include "any_driver.h"
#include "obj3d.h"
#include "gp.h"
#include "draw_object.h"
#include "display_list.h"


/* display list LUA interface */
static int dl_list_tag;

static void lua_init_dl_type(lua_State * L)
{
  dl_list_tag = lua_newtag(L);

  /* Initialize an empty list of display list */
  lua_dostring(L, "dl_lists = { }");
}

static void lua_shutdown_dl_type(lua_State * L)
{
  /* Destroy all allocated display lists */
  lua_dostring(L, 
	       "if type(dl_lists)==[[table]] then "
	       "  foreach(dl_lists, function(i, v) dl_destroy_list(v) end) "
	       "  dl_lists = { } "
	       "end");
}

static int lua_new_list(lua_State * L)
{
  dl_list_t * l;
  int heapsize = lua_tonumber(L, 1);
  int active = lua_tonumber(L, 2);

  printf("Creating new list %d %d\n", heapsize, active);

  lua_settop(L, 0);
  l = dl_new_list(heapsize, active);
  if (l) {

    /* insert the new display list in list of all display lists (used in shutdown) */
    lua_getglobal(L, "dl_lists");
    if (!lua_istable(L, 1)) {
      lua_remove(L, 1);
      lua_newtable(L);
    }
    lua_pushusertag(L, l, dl_list_tag);
    lua_rawseti(L, lua_getn(L, 1)+1, 1);
    lua_setglobal(L, "dl_lists");
    
    /* return the display list to the happy user */
    lua_pushusertag(L, l, dl_list_tag);
    return 1;
  }

  return 0;
}


#define DL_FUNCTION_START(name) \
  static int lua_##name(lua_State * L) \
  { \
    dl_list_t * dl; \
    if (lua_tag(L, 1) != dl_list_tag) { \
      printf("dl_" #name " : first parameter is not a list\n"); \
      return 0; \
    } \
    dl = lua_touserdata(L, 1);

#define DL_FUNCTION_END() }


DL_FUNCTION_START(destroy_list)
{
  printf("destroying list %x\n", dl);
  dl_destroy_list(dl);
  return 0;
}
DL_FUNCTION_END()


DL_FUNCTION_START(get_active)
{
  lua_settop(L, 0);
  lua_pushnumber(L, dl_get_active(dl));
  return 1;
}
DL_FUNCTION_END()


DL_FUNCTION_START(set_active)
{
  dl_set_active(dl, lua_tonumber(L, 2));
  return 0;
}
DL_FUNCTION_END()


DL_FUNCTION_START(clear)
{
  dl_clear(dl);
  return 0;
}
DL_FUNCTION_END()


struct dl_draw_box_command {
  dl_command_t uc;

  float x1, y1, x2, y2, z, a1, r1, g1, b1, a2, r2, g2, b2;
};

void dl_draw_box_render_transparent(void * pcom)
{
  struct dl_draw_box_command * c = pcom;  

/*  static int toto;
  if ( (toto++) % 60 == 1) {
    printf("%g, %g, %g, %g, %g, %g\n", c->x1, c->y1, c->x2, c->y2, c->z, c->a1);
  }*/

  draw_poly_box(c->x1, c->y1, c->x2, c->y2, c->z, c->a1, c->r1, c->g1, c->b1, c->a2, c->r2, c->g2, c->b2);
}

void dl_draw_box(dl_list_t * dl, 
		 float x1, float y1, float x2, float y2, float z,
		 float a1, float r1, float g1, float b1,
		 float a2, float r2, float g2, float b2)
{
  struct dl_draw_box_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x1 = x1;
    c->y1 = y1;
    c->x2 = x2;
    c->y2 = y2;
    c->z = z;
    c->a1 = a1;
    c->r1 = r1;
    c->g1 = g1;
    c->b1 = b1;
    c->a2 = a2;
    c->r2 = r2;
    c->g2 = g2;
    c->b2 = b2;
    dl_insert(dl, c, 0, dl_draw_box_render_transparent);
  }
}

DL_FUNCTION_START(draw_box)
{
  dl_draw_box(dl, 
	      lua_tonumber(L, 2),
	      lua_tonumber(L, 3),
	      lua_tonumber(L, 4),
	      lua_tonumber(L, 5),
	      lua_tonumber(L, 6),
	      lua_tonumber(L, 7),
	      lua_tonumber(L, 8),
	      lua_tonumber(L, 9),
	      lua_tonumber(L, 10),
	      lua_tonumber(L, 11),
	      lua_tonumber(L, 12),
	      lua_tonumber(L, 13),
	      lua_tonumber(L, 14)
	      );
  return 0;
}
DL_FUNCTION_END()


struct dl_draw_text_command {
  dl_command_t uc;
  
  char * text;
  float x, y, z, a, r, g, b;
};

void dl_draw_text_render_transparent(void * pcom)
{
  struct dl_draw_text_command * c = pcom;  

  draw_poly_text(c->x, c->y, c->z, c->a, c->r, c->g, c->b, c->text);
}

void dl_draw_text(dl_list_t * dl, 
		 float x, float y, float z,
		 float a, float r, float g, float b,
		 const char * text)
{
  struct dl_draw_text_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x = x;
    c->y = y;
    c->z = z;
    c->a = a;
    c->r = r;
    c->g = g;
    c->b = b;
    c->text = dl_alloc(dl, strlen(text)+1);
    strcpy(c->text, text);
    dl_insert(dl, c, 0, dl_draw_text_render_transparent);
  }
}

DL_FUNCTION_START(draw_text)
{
  dl_draw_text(dl, 
	      lua_tonumber(L, 2),
	      lua_tonumber(L, 3),
	      lua_tonumber(L, 4),
	      lua_tonumber(L, 5),
	      lua_tonumber(L, 6),
	      lua_tonumber(L, 7),
	      lua_tonumber(L, 8),
	      lua_tostring(L, 9)
	      );
  return 0;
}
DL_FUNCTION_END()


static int display_init(any_driver_t *d)
{

  return 0;
}

static int display_shutdown(any_driver_t * d)
{

  return 0;
}


static driver_option_t * display_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

#include "luashell.h"

static int init;

static int lua_init(lua_State * L)
{

  if (init) {
    printf("display_driver_init called more than once !\n");

    return 0;
  }

  init = 1;

  printf("display_driver_init called\n");


  lua_init_dl_type(L);
  

  return 0;
}

static int lua_shutdown(lua_State * L)
{

  if (!init) {
    printf("display_driver_shutdown called more than once !\n");

    return 0;
  }

  init = 0;

  printf("display_driver_shutdown called\n");


  lua_shutdown_dl_type(L);
  

  return 0;
}

static luashell_command_description_t display_commands[] = {
  {
    "display_driver_init", 0,            /* long and short names */
    "print [["
      "display_driver_init : INTERNAL ; initialize lua side display driver "
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_init            /* function */
  },
  {
    "display_driver_shutdown", 0,            /* long and short names */
    "print [["
      "display_driver_shutdown : INTERNAL ; shut down lua side display driver "
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_shutdown        /* function */
  },
  {
    "dl_new_list", 0,                    /* long and short names */
    "print [["
      "dl_new_list(heapsize, active) : create a new display list, return handle on display list"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_new_list        /* function */
  },
  {
    "dl_destroy_list", 0,                /* long and short names */
    "print [["
      "dl_destroy_list(list) : destroy the given list"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_destroy_list    /* function */
  },
  {
    "dl_set_active", 0,                  /* long and short names */
    "print [["
      "dl_destroy_list(list, active) : set active state"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_set_active      /* function */
  },
  {
    "dl_get_active", 0,                  /* long and short names */
    "print [["
      "dl_get_active(list) : get active state"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_get_active      /* function */
  },
  {
    "dl_clear", 0,                       /* long and short names */
    "print [["
      "dl_clear(list) : get clear"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_clear           /* function */
  },
  {
    "dl_draw_box", 0,                    /* long and short names */
    "print [["
      "dl_draw_box(list, x1, y1, x2, y2, z, a1, r1, g1, b1, a2, r2, g2, b2) : draw a box"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_box        /* function */
  },
  {
    "dl_draw_text", 0,                   /* long and short names */
    "print [["
      "dl_draw_text(list, x, y, z, a, r, g, b, string) : draw text"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_text       /* function */
  },
  {0},                                   /* end of the command list */
};

static any_driver_t display_driver =
{

  0,                     /**< Next driver                     */
  EXE_DRIVER,            /**< Driver type                     */      
  0x0100,                /**< Driver version                  */
  "display",             /**< Driver name                     */
  "Vincent Penne, ",     /**< Driver authors                  */
  "Graphical LUA "       /**< Description                     */
  "extension ",
  0,                     /**< DLL handler                     */
  display_init,          /**< Driver init                     */
  display_shutdown,      /**< Driver shutdown                 */
  display_options,       /**< Driver options                  */
  display_commands,      /**< Lua shell commands              */
  
};

EXPORT_DRIVER(display_driver)
