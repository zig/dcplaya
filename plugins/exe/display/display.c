/**
 * @ingroup  exe_plugin
 * @file     display.c
 * @author   Vincent Penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin
 * 
 * $Id: display.c,v 1.19 2002-11-29 08:29:42 ben Exp $
 */

#include <stdlib.h>
#include <string.h>

#include "luashell.h"
#include "lef.h"
#include "driver_list.h"
#include "obj3d.h"
#include "draw/gc.h"
#include "draw_object.h"

#include "display_driver.h"
#include "display_matrix.h"

/* display_matrix.c */
DL_FUNCTION_DECLARE(init_matrix_type);
DL_FUNCTION_DECLARE(shutdown_matrix_type);
DL_FUNCTION_DECLARE(set_trans);
DL_FUNCTION_DECLARE(get_trans);
DL_FUNCTION_DECLARE(mat_new);
DL_FUNCTION_DECLARE(mat_mult);
DL_FUNCTION_DECLARE(mat_mult_self);
DL_FUNCTION_DECLARE(mat_rotx);
DL_FUNCTION_DECLARE(mat_roty);
DL_FUNCTION_DECLARE(mat_rotz);
DL_FUNCTION_DECLARE(mat_scale);
DL_FUNCTION_DECLARE(mat_trans);
DL_FUNCTION_DECLARE(mat_li);
DL_FUNCTION_DECLARE(mat_co);
DL_FUNCTION_DECLARE(mat_el);
DL_FUNCTION_DECLARE(mat_dim);
DL_FUNCTION_DECLARE(mat_stat);
DL_FUNCTION_DECLARE(mat_dump);

/* display_text.c */
DL_FUNCTION_DECLARE(draw_text);
DL_FUNCTION_DECLARE(measure_text);
DL_FUNCTION_DECLARE(text_prop);

/* display_box.c */
DL_FUNCTION_DECLARE(draw_box1);
DL_FUNCTION_DECLARE(draw_box2);
DL_FUNCTION_DECLARE(draw_box4);

/* display_clipping.c */
DL_FUNCTION_DECLARE(set_clipping);
DL_FUNCTION_DECLARE(get_clipping);

/* display_triangle.c */
DL_FUNCTION_DECLARE(draw_triangle);

/* display_strip.c */
DL_FUNCTION_DECLARE(draw_strip);

/* display_texture.c */
DL_FUNCTION_DECLARE(tex_new);
DL_FUNCTION_DECLARE(tex_destroy);
DL_FUNCTION_DECLARE(tex_get);
DL_FUNCTION_DECLARE(tex_info);

/* display_commands.c */
DL_FUNCTION_DECLARE(nop);
DL_FUNCTION_DECLARE(sublist);

/* display_color.c */
DL_FUNCTION_DECLARE(draw_colors);

/* display list LUA interface */
int dl_list_tag;

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
  int active   = lua_tonumber(L, 2);
  int sub      = lua_tonumber(L, 3);

  if (dl_list_tag < 0) {
    /* try to initialize it */
    lua_dostring(L, DRIVER_NAME"_driver_init()");
    
    if (dl_list_tag < 0) {
      printf("display driver not initialized !");
      return 0;
    }
  }

  printf("Creating new %s-list %d %d\n", sub?"sub":"main",heapsize, active);

  lua_settop(L, 0);
  l = dl_create(heapsize, active, sub);
  if (l) {

    /* insert the new display list into list of all display lists
	   (used in shutdown) */
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


DL_FUNCTION_START(destroy_list)
{
  printf("destroying list %p\n", dl);

  lua_settop(L, 0);

  /* remove the display list from list of all display lists (used in shutdown) */
  lua_getglobal(L, "dl_lists");
  lua_pushusertag(L, dl, dl_list_tag);
  lua_pushnil(L);
  lua_settable(L, 1);

  dl_destroy(dl);
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

DL_FUNCTION_START(set_active2)
{
  int active = lua_tonumber(L, 3);
  dl_list_t * dl2;

  if (lua_tag(L, 2) != dl_list_tag) {
	printf("dl_set_active2 : 2nd parameter is not a list\n");
	return 0;
  }
  dl2 = lua_touserdata(L, 2);
  lua_settop(L, 0);
  lua_pushnumber(L, dl_set_active2(dl, dl2, active));
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_START(clear)
{
  dl_clear(dl);
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(set_color)
{
  int i;
  float col[4];
  const float * dcol = (const float *) dl_get_color(dl);

  for (i=0; i<4; ++i) {
	col[i] = (lua_type(L,i+2) == LUA_TNUMBER)
	  ? lua_tonumber(L, i+2)
	  : dcol[i];
  }
  dl_set_color(dl,(const dl_color_t *) col);
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_color)
{
  dl_color_t * col;
  col = dl_get_color(dl);
  lua_settop(L, 0);
  lua_pushnumber(L, col->a);
  lua_pushnumber(L, col->r);
  lua_pushnumber(L, col->g);
  lua_pushnumber(L, col->b);
  return 4;
}
DL_FUNCTION_END()


static int display_init(any_driver_t *d)
{
  return display_matrix_init();
}

static int display_shutdown(any_driver_t * d)
{
  return display_matrix_shutdown();
}

static driver_option_t * display_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}


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

  /* general commands */

  {
    DRIVER_NAME"_driver_init", 0,            /* long and short names */
    "print [["
	DRIVER_NAME"_driver_init : "
	"INTERNAL ; initialize lua side display driver."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_init            /* function */
  },
  {
    DRIVER_NAME"_driver_shutdown", 0,        /* long and short names */
    "print [["
	DRIVER_NAME"_driver_shutdown : "
	"INTERNAL ; shut down lua side display driver."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_shutdown        /* function */
  },
  {
    "dl_new_list", "dl_new",             /* long and short names */
    "print [["
	"dl_new_list([heapsize, [active, [sub-list] ] ]) : "
	"Create a new display list, return handle on display list."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_new_list        /* function */
  },
  {
    "dl_destroy_list", "dl_destroy",     /* long and short names */
    "print [["
	"dl_destroy_list(list) : "
	"Destroy the given list."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_destroy_list    /* function */
  },
  {
    "dl_heap_size", 0,               /* long and short names */
    "print [["
	"dl_heap_size(list) : "
	"Get heap size in bytes."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_heap_size      /* function */
  },
  {
    "dl_heap_used", 0,              /* long and short names */
    "print [["
	"dl_heap_used(list) : "
	"Get number of byte used in the heap."
    "]]",                               /* usage */
    SHELL_COMMAND_C, lua_heap_used      /* function */
  },
  {
    "dl_set_active", 0,                  /* long and short names */
    "print [["
	"dl_set_active(list,state) : "
	"Set active state and return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_set_active      /* function */
  },
  {
    "dl_set_active2", 0,                  /* long and short names */
    "print [["
    "dl_set_active(list1,list2,state) : "
	"Set active state of 2 lists. Bit:0/1 respectively state of list1/list2.\n"
	"Return old states."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_set_active2     /* function */
  },
  {
    "dl_get_active", 0,                  /* long and short names */
    "print [["
      "dl_get_active(list) : get active state"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_get_active      /* function */
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
    "dl_clear", 0,                       /* long and short names */
    "print [["
      "dl_clear(list) : get clear"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_clear           /* function */
  },

  /* color interface */

  {
    "dl_draw_colors", 0,                    /* long and short names */
    "print [["
	"dl_draw_box(list, a, r, g, b [,a, r, g, b ...]) : set up to 4 "
	"draw colors."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_colors     /* function */
  },

  /* box interface */

  {
    "dl_draw_box1", 0,                    /* long and short names */
    "print [["
	"dl_draw_box(list, x1, y1, x2, y2, z, a, r, g, b) : draw a flat box"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_box1       /* function */
  },
  {
    "dl_draw_box", 0,                    /* long and short names */
    "print [["
	"dl_draw_box(list, x1, y1, x2, y2, z,"
	" a1, r1, g1, b1, a2, r2, g2, b2, type) : draw gradiant box\n"
	" - type is gradiant orientation (0:diagonal, 1:horizontal 2:vertical"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_box2       /* function */
  },
  {
    "dl_draw_box4", 0,                   /* long and short names */
    "print [["
	"dl_draw_box(list, x1, y1, x2, y2, z, "
	"a1, r1, g1, b1, "
	"a2, r2, g2, b2, "
	"a3, r3, g3, b3, "
	"a4, r4, g4, b4) : draw a colored box"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_box4       /* function */
  },

  /* text interface */

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
    "dl_text_prop", 0,                   /* long and short names */
    "print [["
      "dl_text_prop(list, [fontid, [size, [aspect] ] ]) : set text properties"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_text_prop       /* function */
  },

  /* triangle interface */
  {
    "dl_draw_triangle", 0,               /* long and short names */
    "print [["
	"dl_draw_triangle(list, matrix | (vector[,vector,vector]) "
	"[, [ texture-id, ] [ opacity-mode ] ] ) : draw a triangle"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_triangle   /* function */
  },

  /* strip interface */

  {
    "dl_draw_strip", 0,               /* long and short names */
    "print [["
	"dl_draw_strip(list, matrix | (vector,n) "
	"[, [ texture-id, ] [ opacity-mode ] ] ) : draw a strip"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_draw_strip   /* function */
  },

  /* built-in command interface */

  {
	"dl_nop", 0,                /* long and short names */
	"print [["
	"dl_nop() : No operation."
	"]]",                       /* usage */
	SHELL_COMMAND_C, lua_nop    /* function */
  },
  {
	"dl_sublist", 0,            /* long and short names */
	"print [["
	"dl_sublist(list, sublist [, color-op [, trans-op [, restore] ] ]) "
	" : draw a sub-list.\n"
	" <color-op> and <trans-op> are strings that respectively set the color"
	" and transformation heritage.\n"
    " default := MODULATE\n"
	" values := { PARENT, LOCAL, ADD, MODULATE }\n"
	"\n"
	" <restore> is a string containing characters that define which graphic"
	" context properties will be restored at the end of the sub-list"
	" execution. The string is parsed from left to right.\n"
	" Control characters may be uppercase to activate and lowercase"
	" to desactivate a graphic property restore stat.\n"
    " default := A (All)\n"
	" General-Properties\n"
	" - <A/a> : set/clear all properties.\n"
	" - <~>   : invert all\n"
	" Text-Properties\n"
	" - <T/t> : activate/desactivate all text properties\n"
	" - <S/s> : activate/desactivate text size\n"
	" - <F/s> : activate/desactivate text font face\n"
	" - <K/k> : activate/desactivate text color\n"
	" Color-Properties\n"
	" - <C/c> : activate/desactivate color table properties\n"
    " - <0/1/2/3> : toggle color #\n"  
	" Clipping-Property\n"
	" -<B/b>  : activate/desactivate clipping box restoration.\n"
	"]]",                           /* usage */
	SHELL_COMMAND_C, lua_sublist    /* function */
  },

  /* clipping interface */

  {
	"dl_set_clipping", 0,                /* long and short names */
	"print [["
	"dl_set_clipping(list, x1, y1, x2, y2) : set clipping box"
	"]]",                                /* usage */
	SHELL_COMMAND_C, lua_set_clipping    /* function */
  },
  
  {
	"dl_get_clipping", 0,                /* long and short names */
	"print [["
	"dl_get_clipping(list) : get clipping box {x1,y1,x2,y2}"
	"]]",                                /* usage */
	SHELL_COMMAND_C, lua_get_clipping    /* function */
  },

  /* texture interface */

  {
	"tex_new", 0,                        /* long and short names */
	"print [["
	"tex_new(filename) or tex_new(name,width,heigth,format) :\n"
	"Create a new texure. Returns texture identifier"
	"]]",                                /* usage */
	SHELL_COMMAND_C, lua_tex_new         /* function */
  },

  {
	"tex_destroy", 0,                    /* long and short names */
	"print [["
	"tex_destroy(texture-name|texture-id [,force] ) : "
	"Destroy a texture."
	"]]",                                /* usage */
	SHELL_COMMAND_C, lua_tex_destroy     /* function */
  },

  {
	"tex_get", 0,                        /* long and short names */
	"print [["
	"tex_get(texture-name|texture-id) : "
	"Get texture identifier."
	"]]",                                /* usage */
	SHELL_COMMAND_C, lua_tex_get         /* function */
  },

  {
	"tex_info", 0,                        /* long and short names */
	"print [["
	"tex_info(texture-name|texture-id) : "
	"Get texture info structure."
	"]]",                                /* usage */
	SHELL_COMMAND_C, lua_tex_info        /* function */
  },

  /* matrix interface */
  {
    "mat_new", 0,                        /* long and short names */
    "print [["
	"mat_new( [ mat | lines, [ columns ] ] ) : make a new matrix. "
	"Default lines and columns is 4. "
	"If mat is given the result is a copy of mat. "
	"If matrix dimension is 4x4 the result is an identity matrix "
	"else the result matrix is zeroed."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_new         /* function */
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
	"mat_mult(mat1, mat2) : make matrix by apply mat1xmat2\n"
	"Where mat2 is 4x4 matrix."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_mult        /* function */
  },
  {
    "mat_mult_self", 0,                  /* long and short names */
    "print [["
	"mat_mult([mat0,] mat1, mat2) : returns mat0=mat1xmat2 or mat1=mat1xmat2\n"
	"Where mat2 is 4x4 matrix."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_mult_self   /* function */
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
  {
    "mat_dim", 0,                         /* long and short names */
    "print [["
      "mat_dim(mat) : get matrix dimension (line,columns)"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_dim         /* function */
  },

  {
    "mat_stat", 0,                         /* long and short names */
    "print [["
	"mat_stat([level]) : Display matrices debug info."	
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_stat        /* function */
  },

  {
    "mat_dump", 0,                         /* long and short names */
    "print [["
	"mat_dump(mat,[level]) : Display matrix mat debug info."	
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_mat_dump        /* function */
  },


  {0},                                   /* end of the command list */
};

any_driver_t display_driver =
{

  0,                     /**< Next driver                     */
  EXE_DRIVER,            /**< Driver type                     */      
  0x0100,                /**< Driver version                  */
  DRIVER_NAME,           /**< Driver name                     */
  "Vincent Penne, "      /**< Driver authors                  */
  "Benjamin Gerard",
  "Graphical LUA "       /**< Description                     */
  "extension ",
  0,                     /**< DLL handler                     */
  display_init,          /**< Driver init                     */
  display_shutdown,      /**< Driver shutdown                 */
  display_options,       /**< Driver options                  */
  display_commands,      /**< Lua shell commands              */
  
};

EXPORT_DRIVER(display_driver)
