/** @ingroup dcplaya_vis_driver
 *  @file    fime.c
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   Fly Into a Musical Environment
 *  $Id: fime.c,v 1.8 2003-02-04 15:07:34 ben Exp $
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math_float.h"

#include "matrix.h"
#include "driver_list.h"
#include "vis_driver.h"
#include "obj_driver.h"
#include "fft.h"
#include "draw_object.h"
#include "remanens.h"
#include "sysdebug.h"
#include "draw/vertex.h"
#include "border.h"

/* fime includes */
#include "fime_pcm.h"
#include "fime_analysis.h"
#include "fime_beatdetect.h"
#include "fime_bordertex.h"
#include "fime_ship.h"
#include "fime_bees.h"
#include "fime_ground.h"
#include "fime_star.h"

#ifndef PI
# define PI 3.14159265359
#endif

extern void FaceNormal(float *d, const vtx_t * v, const tri_t *t);

/* Ambient color */
static vtx_t ambient = {
  0,0,0,0
};

volatile int ready;

static viewport_t viewport;   /**< Current viewport */
static matrix_t proj;         /**< Current projection matrix */
static matrix_t camera;       /**< Current camera matrix */
static vtx_t camera_pos;

static float max(const float a, const float b) {
  return a > b ? a : b;
}
static float min(const float a, const float b) {
  return a < b ? a : b;
}
static float bound(const float a, const float b, const float c) {
  return a < b ? b : (a > c ? c : a);
}

/* $$$ Defined in libdcutils */
extern int rand();

static float inc_angle(float a, const float v)
{
  a = a + v;
  if (a<0) {
    do { a += 2*PI; } while (a<0);
  } else if (a>=2*PI) {
    do { a -= 2*PI; } while (a>=2*PI);
  }
  return a;
}


static fime_star_field_t * starfield;

static int stop(void)
{
  ready = 0;

  fime_star_shutdown(starfield);
  starfield = 0;
  fime_ground_shutdown();
  fime_bees_shutdown();
  fime_ship_shutdown();
  fime_beatdetect_shutdown();
  fime_analysis_shutdown();
  fime_pcm_shutdown();

  SDDEBUG("[fime] stopped\n");

  return 0;
}

static int start(void)
{
  int err = 0;
  ready = 0;

  SDDEBUG("[fime] starting\n");
  SDINDENT;

  camera_pos.x = 1.31;
  camera_pos.y = 1.2;
  camera_pos.z = -1.5;

  err = err || fime_pcm_init();
  err = err || fime_analysis_init();
  err = err || fime_beatdetect_init();
  err = err || fime_ship_init();
  err = err || fime_bees_init();
  err = err || fime_ground_init(4,20,6,20);

  {
    static const char * names[] = { "star", "asteroid_low", 0 }; 
    fime_star_box_t box = { -6, 6, -4, -1, 0, 20 };

    starfield = fime_star_init(32, names, &box, 0);
    err = err || (!starfield);
  }

  SDUNINDENT;

  ready = !err;

  SDDEBUG("[fime] start := [%s]\n", ready ? "OK" : "FAILED");

  return -!ready;
}

void fime_bees_set_target(const vtx_t * pos);

static int process(viewport_t * vp, matrix_t projection, int elapsed_ms)
{
  const float seconds = elapsed_ms * (1/1000.0f);

  matrix_t * shipmtx;

  if (!ready) {
    return -1;
  }
  if (seconds == 0) {
    return 0;
  }

/*   vid_border_color(0,255,0); */
  viewport = *vp;
  MtxCopy(proj, projection);

  fime_pcm_update();
  fime_analysis_update();
  fime_beatdetect_update();
  shipmtx = fime_ship_update(seconds);
  fime_bees_set_target((const vtx_t *)shipmtx[3]);
  fime_bees_update();
  fime_ground_update(seconds);
  fime_star_update(starfield, seconds);

  {
    const float s = 0.7, o = 1.0 - s;
    float x = (*shipmtx)[3][0];
    float y = (*shipmtx)[3][1];
    float z = (*shipmtx)[3][2];

    float cam_x = x; 
    float cam_y = y - 1.1/2;
    float cam_z = z - 1.5;
    z += 3;
    x = 0;

    camera_pos.x = camera_pos.x * s + cam_x * o;
    camera_pos.y = camera_pos.y * s + cam_y * o;
    camera_pos.z = camera_pos.z * s + cam_z * o;

    MtxLookAt2(camera, camera_pos.x, camera_pos.y, camera_pos.z, x, y, z);
  }

  /* $$$ For test : */
/*   MtxIdentity(camera); */
/*   camera[3][2] = -2; */

/*   SDDEBUG("[%f %f %f] [%f %f %f]\n", */
/* 	  camera_pos.x, camera_pos.y, camera_pos.z, */
/* 	  camera[3][0], */
/* 	  camera[3][1], */
/* 	  camera[3][2] */
/* 	  ); */

/*   vid_border_color(0,0,0); */

  return 0;
}

static int opaque_render(void)
{
  int err = 0;
  if (!ready) {
    return -1;
  }

  static vtx_t color = { 1,1,0,1};

  err = err || fime_bees_render(&viewport, camera, proj);
  err = err || fime_ship_render(&viewport, camera, proj,
				0, 0, 0, 1);
  err = err || fime_ground_render(&viewport, camera, proj, 1);

  err = err || fime_star_render(starfield,
				&viewport, camera, proj,
				&color,
				&color,
				0,1);
  return -(!!err);
}

static int transparent_render(void)
{
  int err = 0;

  if (!ready) {
    return -1;
  }
  return -(!!err);
}

static int init(any_driver_t *d)
{
  int err = 0;

  ready = 0;
  err = fime_bordertex_init();

  SDDEBUG("[fime] init := [%s]\n", !err ? "OK" : "FAILED" );
  return -(!!err);
}

static int shutdown(any_driver_t *d)
{
  stop();
  fime_bordertex_shutdown();
  SDDEBUG("[fime] studowned\n");
  return 0;
}

static driver_option_t * options(struct _any_driver_s *driver,
				 int idx, driver_option_t * opt)
{
  return opt;
}

#include "luashell.h"

static void lua_setcolor(lua_State * L, float * c)
{
  int i, max;
  max = lua_gettop(L);
  if (max > 4) max = 4;
  for (i=0; i<max; ++i) {
    if (lua_type(L,i+1) != LUA_TNIL) {
      c[(i-1)&3] = lua_tonumber(L, i+1);
    }
  }
}
 
static int lua_setambient(lua_State * L)
{
  lua_setcolor(L,(float *) &ambient);
  return 0;
}

static luashell_command_description_t commands[] = {
  {
    "fime_setambient", 0,              /* long and short names */
    "print [["
    "fime_setambient(a, r, g, b) : set ambient color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setambient      /* function */
  },
  {0},                                   /* end of the command list */
};

static vis_driver_t driver = {

  /* Any driver. */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h) */
    VIS_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "fime",               /* Driver name */
    "Benjamin Gerard\0",  /* Driver authors */
    "FLy Into a Musical " /* Description */
    "Environment",
    0,                    /* DLL handler */
    init,                 /* Driver init */
    shutdown,             /* Driver shutdown */
    options,              /* Driver options */
    commands,             /* Lua shell commands  */
  },

  start,                  /* Driver start */
  stop,                   /* Driver stop */  
  process,                /* Driver post render calculation */
  opaque_render,          /* Driver opaque render */
  transparent_render      /* Driver transparent render */

};

EXPORT_DRIVER(driver)
