/**
 * @ingroup  exe_plugin
 * @file     display.c
 * @author   Vincent Penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display.c,v 1.3 2002-10-04 21:04:32 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "lef.h"
#include "lua.h"
#include "any_driver.h"
#include "driver_list.h"
#include "obj3d.h"
#include "gp.h"
#include "draw_object.h"
#include "display_list.h"

extern any_driver_t display_driver;

#define DRIVER_NAME "display"


/* matrix LUA interface */

static int matrix_tag;

#define CHECK_MATRIX(i) \
  if (lua_tag(L, i) != matrix_tag) { \
    printf("%s : argument #%d is not a matrix\n", __FUNCTION__, i); \
    return 0; \
  } else

static float * NEW_MATRIX()
{
  driver_reference(&display_driver);
  return malloc(sizeof(matrix_t));
}


static int lua_mat_gc(lua_State * L)
{
  free(lua_touserdata(L, 1));
  driver_dereference(&display_driver);
  return 0;
}

static int lua_mat_mult(lua_State * L)
{
  float * r;

  CHECK_MATRIX(1);
  CHECK_MATRIX(2);

  r = NEW_MATRIX();
  memcpy(r, lua_touserdata(L, 1), sizeof(matrix_t));
  MtxMult(* (matrix_t *) r, * (matrix_t *) (float *) lua_touserdata(L, 2));

  lua_settop(L, 0);
  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

static void lua_init_matrix_type(lua_State * L)
{
  matrix_tag = lua_newtag(L);

  lua_pushcfunction(L, lua_mat_gc);
  lua_settagmethod(L, matrix_tag, "gc");
  
  lua_pushcfunction(L, lua_mat_mult);
  lua_settagmethod(L, matrix_tag, "mul");
  
}

static void lua_shutdown_matrix_type(lua_State * L)
{
}

static int lua_mat_new(lua_State * L)
{
  float * r;

  r = NEW_MATRIX();

  MtxIdentity(* (matrix_t *) r);

  lua_settop(L, 0);
  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

#define Cos fcos
#define Sin fsin
#define Inv(a) (1.0f/(a))

static void myMtxRotateX(matrix_t m2, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  MtxIdentity(m2);
  m2[1][1] = m2[2][2] = ca;
  m2[1][2] = sa;
  m2[2][1] = -sa;
}

static void myMtxRotateY(matrix_t m2, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  MtxIdentity(m2);
  m2[0][0] = m2[2][2] = ca;
  m2[2][0] = sa;
  m2[0][2] = -sa;
}

static void myMtxRotateZ(matrix_t m2, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  MtxIdentity(m2);
  m2[0][0] = m2[1][1] = ca;
  m2[0][1] = sa;
  m2[1][0] = -sa;
}

static int lua_mat_rotx(lua_State * L)
{
  float * r;

  r = NEW_MATRIX();

  myMtxRotateX(* (matrix_t *) r, lua_tonumber(L, 1));

  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

static int lua_mat_roty(lua_State * L)
{
  float * r;

  r = NEW_MATRIX();

  myMtxRotateY(* (matrix_t *) r, lua_tonumber(L, 1));

  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

static int lua_mat_rotz(lua_State * L)
{
  float * r;

  r = NEW_MATRIX();

  myMtxRotateZ(* (matrix_t *) r, lua_tonumber(L, 1));

  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

static int lua_mat_scale(lua_State * L)
{
  float * r;

  r = NEW_MATRIX();

  memset(r, 0, sizeof(matrix_t));
  r[0] = lua_tonumber(L, 1);
  r[5] = lua_tonumber(L, 2);
  r[10] = lua_tonumber(L, 3);
  r[15] = 1.0f;

  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

static int lua_mat_trans(lua_State * L)
{
  float * r;

  r = NEW_MATRIX();

  MtxIdentity(* (matrix_t *) r);
  r[12] = lua_tonumber(L, 1);
  r[13] = lua_tonumber(L, 2);
  r[14] = lua_tonumber(L, 3);
  r[15] = 1.0f/* + lua_tonumber(L, 4)*/;

  lua_pushusertag(L, r, matrix_tag);
  return 1;
}

static int lua_mat_li(lua_State * L)
{
  int n = lua_gettop(L), l;
  matrix_t *m;
  
  CHECK_MATRIX(1);
  m = (matrix_t *) (float *) lua_touserdata(L, 1);

  if (n != 2) {
	printf("mat_li : bad arguments\n");
	return 0;
  }

  l = lua_tonumber(L,2);
  if (l<0 || l>3) {
	printf("mat_li : invalid index %d\n", l);
	return 0;
  }

  lua_settop(L,0);
  lua_newtable(L);
  for (n=0; n<4; ++n) {
	lua_pushnumber(L, n+1);
	lua_pushnumber(L, (*m)[l][n]);
	lua_rawset(L, 1);
  }
  return 1;
}

static int lua_mat_co(lua_State * L)
{
  int n = lua_gettop(L), l;
  matrix_t *m;
  
  CHECK_MATRIX(1);
  m = (matrix_t *) (float *) lua_touserdata(L, 1);

  if (n != 2) {
	printf("mat_co : bad arguments\n");
	return 0;
  }

  l = lua_tonumber(L,2);
  if (l<0 || l>3) {
	printf("mat_co : invalid index %d\n", l);
	return 0;
  }

  lua_settop(L,0);
  lua_newtable(L);
  for (n=0; n<4; ++n) {
	lua_pushnumber(L, n+1);
	lua_pushnumber(L, (*m)[n][l]);
	lua_rawset(L, 1);
  }
  return 1;
}

static int lua_mat_el(lua_State * L)
{
  int n = lua_gettop(L), l;
  matrix_t *m;
  
  CHECK_MATRIX(1);
  m = (matrix_t *) (float *) lua_touserdata(L, 1);

  if (n != 3) {
	printf("mat_el : bad arguments\n");
	return 0;
  }

  l = lua_tonumber(L,2);
  n = lua_tonumber(L,3);
  if (l<0 || l>3 || n<0 || n>3) {
	printf("mat_el : invalid index %d,%d\n", l, n);
	return 0;
  }

  lua_settop(L,0);
  lua_pushnumber(L, (*m)[l][n]);
  return 1;
}

/* display list LUA interface */
static int dl_list_tag;

static void lua_init_dl_type(lua_State * L)
{
  dl_list_tag = lua_newtag(L);

  /* Initialize an empty list of display list */
  lua_dostring(L, 
	       "\n dl_lists = { }"
	       "\n init_display_driver = nil"
	       );
}

static void lua_shutdown_dl_type(lua_State * L)
{
  /* Destroy all allocated display lists */
  lua_dostring(L, 
	       "\n if type(dl_lists)==[[table]] then "
	       "\n   while next(dl_lists) do"
	       "\n     dl_destroy_list(next(dl_lists))"
	       "\n   end"
	       "\n end"
	       "\n dl_lists = { }"
	       "\n init_display_driver = nil"
	       );

  dl_list_tag = -1;
}

static int lua_new_list(lua_State * L)
{
  dl_list_t * l;
  int heapsize = lua_tonumber(L, 1);
  int active = lua_tonumber(L, 2);

  if (dl_list_tag < 0) {
    /* try to initialize it */
    lua_dostring(L, DRIVER_NAME"_driver_init()");
    
    if (dl_list_tag < 0) {
      printf("display driver not initialized !");
      return 0;
    }
  }

  printf("Creating new list %d %d\n", heapsize, active);

  lua_settop(L, 0);
  l = dl_new_list(heapsize, active);
  if (l) {

    /* insert the new display list into list of all display lists (used in shutdown) */
    lua_getglobal(L, "dl_lists");
    lua_pushusertag(L, l, dl_list_tag);
    lua_pushnumber(L, 1);
    lua_settable(L, 1);
    
    /* return the display list to the happy user */
    lua_settop(L, 0);
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
  printf("destroying list %p\n", dl);

  lua_settop(L, 0);

  /* remove the display list from list of all display lists (used in shutdown) */
  lua_getglobal(L, "dl_lists");
  lua_pushusertag(L, dl, dl_list_tag);
  lua_pushnil(L);
  lua_settable(L, 1);

  dl_destroy_list(dl);
  return 0;
}
DL_FUNCTION_END()


DL_FUNCTION_START(heap_size)
{
  lua_settop(L, 0);
  lua_pushnumber(L, dl->heap_size);
  return 1;
}
DL_FUNCTION_END()


DL_FUNCTION_START(heap_used)
{
  lua_settop(L, 0);
  lua_pushnumber(L, dl->heap_pos);
  return 1;
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
  int active = lua_tonumber(L, 2);
  lua_settop(L, 0);
  lua_pushnumber(L, dl_set_active(dl, active));
  return 1;
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

  draw_poly_box(dl_trans[0][0] * c->x1 + dl_trans[3][0], 
		dl_trans[1][1] * c->y1 + dl_trans[3][1], 
		dl_trans[0][0] * c->x2 + dl_trans[3][0], 
		dl_trans[1][1] * c->y2 + dl_trans[3][1], 
		dl_trans[2][2] * c->z + dl_trans[3][2], 
		dl_color[0] * c->a1, dl_color[1] * c->r1, 
		dl_color[2] * c->g1, dl_color[3] * c->b1, 
		dl_color[0] * c->a2, dl_color[1] * c->r2, 
		dl_color[2] * c->g2, dl_color[3] * c->b2);
}

void dl_draw_box(dl_list_t * dl, 
		 float x1, float y1, float x2, float y2, float z,
		 float a1, float r1, float g1, float b1,
		 float a2, float r2, float g2, float b2)
{
  struct dl_draw_box_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x1 = x1;    c->y1 = y1;    c->x2 = x2;    c->y2 = y2;    c->z = z;
    c->a1 = a1;    c->r1 = r1;    c->g1 = g1;    c->b1 = b1;
    c->a2 = a2;    c->r2 = r2;    c->g2 = g2;    c->b2 = b2;
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

  draw_poly_text(dl_trans[0][0] * c->x + dl_trans[3][0], dl_trans[1][1] * c->y + dl_trans[3][1], dl_trans[2][2] * c->z + dl_trans[3][2], c->a * dl_color[0], c->r * dl_color[1], c->g * dl_color[2], c->b * dl_color[3], c->text);
}

void dl_draw_text(dl_list_t * dl, 
		  float x, float y, float z,
		  float a, float r, float g, float b,
		  const char * text)
{
  struct dl_draw_text_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x = x;    c->y = y;    c->z = z;
    c->a = a;    c->r = r;    c->g = g;    c->b = b;
    c->text = dl_alloc(dl, strlen(text)+1);
    if (c->text) {
      strcpy(c->text, text);
      dl_insert(dl, c, 0, dl_draw_text_render_transparent);
    }
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


DL_FUNCTION_START(measure_text)
{
  lua_pushnumber(L, measure_poly_text(lua_tostring(L, 2)));
  lua_pushnumber(L, 16); // this is constant for now !!!
  return 2;
}
DL_FUNCTION_END()

DL_FUNCTION_START(set_trans)
{
  CHECK_MATRIX(2);
  dl_set_trans(dl, lua_touserdata(L, 2));
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_trans)
{
  float * mat = NEW_MATRIX();
  memcpy(mat, dl_get_trans(dl), sizeof(matrix_t));
  lua_settop(L, 0);
  lua_pushusertag(L, mat, matrix_tag);
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_START(set_color)
{
  dl_color_t col = { 
    lua_tonumber(L, 2),
    lua_tonumber(L, 3),
    lua_tonumber(L, 4),
    lua_tonumber(L, 5),
  };
  dl_set_color(dl, col);
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_color)
{
  float * col;
  col = dl_get_color(dl);
  lua_settop(L, 0);
  lua_pushnumber(L, col[0]);
  lua_pushnumber(L, col[1]);
  lua_pushnumber(L, col[2]);
  lua_pushnumber(L, col[3]);
  return 4;
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


  lua_init_matrix_type(L);
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
  lua_shutdown_matrix_type(L);
  

  return 0;
}

static luashell_command_description_t display_commands[] = {
  {
    DRIVER_NAME"_driver_init", 0,            /* long and short names */
    "print [["
      DRIVER_NAME"_driver_init : INTERNAL ; initialize lua side display driver "
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_init            /* function */
  },
  {
    DRIVER_NAME"_driver_shutdown", 0,        /* long and short names */
    "print [["
      DRIVER_NAME"_driver_shutdown : INTERNAL ; shut down lua side display driver "
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
    "dl_heap_size", 0,               /* long and short names */
    "print [["
      "dl_heap_size(list) : get heap size in bytes"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_heap_size      /* function */
  },
  {
    "dl_heap_used", 0,              /* long and short names */
    "print [["
      "dl_heap_used(list) : get number of byte used in the heap"
    "]]",                               /* usage */
    SHELL_COMMAND_C, lua_heap_used      /* function */
  },
  {
    "dl_set_active", 0,                  /* long and short names */
    "print [["
      "dl_set_active(list,state) : set active state"
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
  {
    "dl_measure_text", 0,                /* long and short names */
    "print [["
      "dl_measure_text(list, string) : measure text"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_measure_text    /* function */
  },
  {
    "dl_set_trans", 0,                   /* long and short names */
    "print [["
      "dl_set_trans(list, matrix) : set the transformation"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_set_trans       /* function */
  },
  {
    "dl_get_trans", 0,                   /* long and short names */
    "print [["
      "dl_get_trans(list) : get the transformation, return a matrix"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_get_trans       /* function */
  },
  {
    "dl_set_color", 0,                   /* long and short names */
    "print [["
      "dl_set_color(list, color) : set the global color"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_set_color       /* function */
  },
  {
    "dl_get_color", 0,                   /* long and short names */
    "print [["
      "dl_get_color(list) : get the color"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_get_color       /* function */
  },
  {
    "mat_new", 0,                        /* long and short names */
    "print [["
      "mat_new() : make a new identity matrix"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_trans       /* function */
  },
  {
    "mat_trans", 0,                      /* long and short names */
    "print [["
      "mat_trans(x, y, z) : make a translation matrix"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_trans       /* function */
  },
  {
    "mat_rotx", 0,                       /* long and short names */
    "print [["
      "mat_rotx(angle) : make rotation matrix around X axis"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_rotx        /* function */
  },
  {
    "mat_roty", 0,                       /* long and short names */
    "print [["
      "mat_roty(angle) : make rotation matrix around Y axis"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_roty        /* function */
  },
  {
    "mat_rotz", 0,                       /* long and short names */
    "print [["
      "mat_rotz(angle) : make rotation matrix around Z axis"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_rotz        /* function */
  },
  {
    "mat_scale", 0,                      /* long and short names */
    "print [["
      "mat_scale(x, y, z) : make scaling matrix with given coeficient"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_scale       /* function */
  },
  {
    "mat_mult", 0,                       /* long and short names */
    "print [["
      "mat_mult(mat1, mat2) : make matrix by apply mat1xmat2"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_mult        /* function */
  },
  {
    "mat_li", 0,                         /* long and short names */
    "print [["
      "mat_li(mat, l) : get matrix line"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_li          /* function */
  },
  {
    "mat_co", 0,                         /* long and short names */
    "print [["
      "mat_co(mat, c) : get matrix column"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_co          /* function */
  },
  {
    "mat_el", 0,                         /* long and short names */
    "print [["
      "mat_el(mat, l, c) : get matrix element"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_el          /* function */
  },

  {0},                                   /* end of the command list */
};

any_driver_t display_driver =
{

  0,                     /**< Next driver                     */
  EXE_DRIVER,            /**< Driver type                     */      
  0x0100,                /**< Driver version                  */
  DRIVER_NAME,           /**< Driver name                     */
  "Vincent Penne",       /**< Driver authors                  */
  "Graphical LUA "       /**< Description                     */
  "extension ",
  0,                     /**< DLL handler                     */
  display_init,          /**< Driver init                     */
  display_shutdown,      /**< Driver shutdown                 */
  display_options,       /**< Driver options                  */
  display_commands,      /**< Lua shell commands              */
  
};

EXPORT_DRIVER(display_driver)
