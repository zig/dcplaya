/**
 * $Id: lpo.c,v 1.4 2002-09-13 00:27:11 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"
#include "driver_list.h"
#include "vis_driver.h"
#include "obj_driver.h"
#include "vupeek.h"
#include "draw_object.h"

/* $$$ Defined in libdcutils */
extern int rand();

static obj_driver_t * curobj; /**< Current 3D-object */
static viewport_t viewport;   /**< Current viewport */
static matrix_t mtx;          /**< Current local matrix */
static matrix_t projmtx;      /**< Current projection matrix */
static vtx_t color;           /**< Final Color */
static vtx_t base_color =     /**< Original color */
  {
    0.8f, 0.8f, 0.2f, 0.7f
  };
static vtx_t flash_color =     /**< Flashing color */
  {
    0.5f, 0.5f, 1.0f, 0.8f
  };

static vtx_t pos;     /**< Object position */
static vtx_t angle;   /**< Object rotation */
static float flash;
static float ozoom;

/* Automatic object changes */
static int random_mode = 1;
static int change_cnt;
static int change_time = 10000;

/* Rotation FX */
static float rps_min = 0.1f;   /**< Minimum rotation per second */
static float rps_max = 5.0f;   /**< Maximum rotation per second */
static float rps_cur;          /**< Current rotation speed factor */
static float rps_goal;         /**< Rotation to reach */
static float rps_up = 0.25f;   /**< Smooth factor when rpsd_goal > rps_cur */
static float rps_dw = 0.10f;   /**< Smooth factor when rpsd_goal < rps_cur */

static float zoom_min = 0.0f;
static float zoom_max = 7.0f;

/* Previous analysis parms */
static float opeek_diff;
static float opeek;
static float peek_max;
static float peek_diff_max;

static int same_sign(float a, float b)
{
  return (a>=0 && b>=0) || (a<0 && b<0);
}

static int anim(unsigned int ms)
{
  const float sec = 0.001f * (float)ms;

  float sax = 1.0f * sec;
  float say = 1.0f * sec;
  float saz = 1.0f * sec;
 
  float peek;
  float zoom;
  
  /* analysis parms */
  float
    peek_diff,    /* peek/oldpeek delta */
    peek_avgdiff; /* peek/avgpeek delta */

  int max, diff_max;

  /* Read peek values */
  peek = (float)peek1.dyn / 65536.0f;
  peek_avgdiff = ((float)peek1.dyn - (float)peek3.dyn) / 65536.0f;
  peek_diff = peek - opeek;

  max = peek > peek_max;
  if (max) {
    peek_max = peek;
  }

  if (same_sign(peek_diff,opeek_diff)) {
    peek_diff += opeek_diff;
  } else {
    diff_max = opeek_diff > peek_diff_max;
    if (diff_max) {
      peek_diff_max = opeek_diff;
      flash = (opeek_diff * 2.60f) + 1.0f;
    }
  }

  /* Calculate zoom factor */
  {
    float r;

    zoom = zoom_min;
    if (peek_max > 1E-4) {
      zoom += (zoom_max - zoom_min) * peek / peek_max;
    }
    r = (zoom > ozoom) ? 0.75f : 0.90f;
    ozoom = ozoom * r + zoom * (1.0f-r);
  }

  /* Calculate rotation speed */
  {
    float r;

    if (peek_avgdiff >= 0) {
      rps_goal = rps_min;
      if (peek_max > 1E-4) {
	rps_goal += (rps_max - rps_min) * peek / peek_max;
      }
    }
    r = rps_cur > rps_goal ? rps_up : rps_dw;
    rps_cur = rps_cur * r + rps_goal * (1.0f - r);
  }

  {
    const float r = 0.99f; 
    rps_goal = rps_goal * r + rps_min * (1.0-r);
  }
    
  peek_max *= 0.9999f;
  peek_diff_max *= 0.99f;
  flash *= 0.9f;

  /* Save for next call */
  opeek = peek;
  opeek_diff = peek_diff;  
  
  /* Move angle */
  angle.x += sax * rps_cur;
  angle.y += say * rps_cur;
  angle.z += saz * rps_cur;

  // $$$
  pos.z = zoom_max + 1.7;

  /* Build local matrix */
  MtxIdentity(mtx);
  MtxRotateX(mtx, angle.x);
  MtxRotateY(mtx, angle.y);
  MtxRotateZ(mtx, angle.z);
  //  MtxScale(mtx, ozoom);
  mtx[3][0] = pos.x;
  mtx[3][1] = pos.y;
  mtx[3][2] = pos.z - ozoom;
  
  /* Build render color : blend base and flash color */
  color.x = base_color.x * (1.0f-flash) + flash_color.x * flash;
  color.y = base_color.y * (1.0f-flash) + flash_color.y * flash;
  color.z = base_color.z * (1.0f-flash) + flash_color.z * flash;
  color.w = base_color.w * (1.0f-flash) + flash_color.w * flash;

  return 0;
}

static obj_driver_t * find_object(const char *name)
{
  any_driver_t * d;

  for (d=obj_drivers.drivers; d; d=d->nxt) {
    if (!strcmp(name, d->name)) {
      return (obj_driver_t *)d;
    }
  }
  return 0;
}

static obj_driver_t * num_object(int n)
{
  any_driver_t * d;
  int i;

  for (i=0, d=obj_drivers.drivers; d; d=d->nxt, ++i) {
    if (i == n) {
      return (obj_driver_t *)d;
    }
  }
  return 0;
}

static obj_driver_t * random_object(obj_driver_t * not_this_one)
{
  obj_driver_t * o;
  if (obj_drivers.n == 0) {
    return 0;
  }
  if (obj_drivers.n == 1) {
    o = (obj_driver_t *)obj_drivers.drivers;
  } else {
    o = num_object(rand() % obj_drivers.n);
    if (o == not_this_one) {
      if (o->common.nxt) {
	o = (obj_driver_t *)o->common.nxt;
      } else {
	o = (obj_driver_t *)obj_drivers.drivers;
      }
    }
  }

  return (o == not_this_one) ? 0 : o;
}

static int change_object(obj_driver_t *o)
{
  if (!o) {
    return -1;
  }
  curobj = o;
  return 0;
}

static int start(void)
{
  obj_driver_t *o;

  change_cnt = 0;

  angle.x = angle.y = angle.z = angle.z = 0;
  pos.x = pos.y = pos.z = pos.w = 0;
  rps_cur = 0;
  rps_goal = rps_min;

  opeek_diff = 0;
  opeek = 0;
  peek_max = 0;
  peek_diff_max = 0;
  flash = 0;
  ozoom = 0;

  curobj = 0;

  /* Select mine_3 as 1st object */
  o = find_object("mine_3");
  if (!o) {
    /* If it does not exist, try another */
    o = random_object(curobj);
  }
  /* No object : failed */
  if (!o) {
    return -1;
  }
  /* Start this object */
  if (change_object(o)) {
    return -1;
  }

  return 0;
}

static int stop(void)
{
  curobj = 0;
  return 0;
}

static int process(viewport_t * vp, matrix_t projection, int elapsed_ms)
{
  if (!curobj) {
    return -1;
  }

  /* Copy viewport and projection matrix for further use (render) */
  viewport = *vp;
  MtxCopy(projmtx,projection);

  /* Check for object change */
  if (random_mode && (change_cnt += elapsed_ms) >= change_time) {
    change_cnt -= change_time;
    change_object(random_object(curobj));
  }

  anim(elapsed_ms);

  return 0;
}


static int opaque_render(void)
{
  return 0;
}

static int transparent_render(void)
{
  if (!curobj) {
    return -1;
  }
  return DrawObjectSingleColor(&viewport, mtx, projmtx,
			       &curobj->obj, &color);
}

static int init(any_driver_t *d)
{
  curobj = 0;
  return 0;
}

static int shutdown(any_driver_t *d)
{
  stop();
  return 0;
}

static driver_option_t * options(struct _any_driver_s *driver,
				 int idx, driver_option_t * opt)
{
  return opt;
}

static vis_driver_t driver = {

  /* Any driver. */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h) */
    VIS_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "LPO",                /* Driver name */
    "Benjamin Gerard\0",  /* Driver authors */
    "Little "             /**< Description */
    "Pumping "
    "Object",
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },

  start,                  /**< Driver start */
  stop,                   /**< Driver stop */  
  process,                /**< Driver post render calculation */
  opaque_render,          /**< Driver opaque render */
  transparent_render      /**< Driver transparent render */

};

EXPORT_DRIVER(driver)
