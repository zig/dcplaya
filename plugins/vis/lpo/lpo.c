/**
 * $Id: lpo.c,v 1.1 2002-09-03 15:06:07 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "matrix.h"
#include "driver_list.h"
#include "vis_driver.h"
#include "obj_driver.h"
#include "vupeek.h"

static obj_driver_t * curobj; /**< Current 3D-object */
static viewport_t viewport;   /**< Current viewport */
static matrix_t projmtx;      /**< Current projection matrix */

static vtx_t pos;     /**< Object position */
static vtx_t angle;   /**< Object rotation */
static float rspd;    /**< Current rotation speed factor */
static float flash;
static float ozoom;

/* Effects parms */
static int random_mode = 1;
static int change_cnt;
static int change_time = 10000;
static float min_rps = 0.1f;
static float max_rps = 5.0f;

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
  
  const float rspd_max = 2.7f;
  const float rspd_min = 0.2f;  
 
  float sax = 1.0f * sec;
  float say = 1.0f * sec;
  float saz = 1.0f * sec;
 
  float peek;
  float zoom;
  
  /* analysis parms */
  float
    peek_fact,    /* full range scale factor */
    peek_diff,    /* peek/oldpeek delta */
    peek_avgdiff, /* peek/avgpeek delta */
    peek_norm;    /* peek normalized to full range */

  int
    max=0,
    diff_max=0; /* True when respectively peek/peek_diff becomes max */
   
  zoom = 0.3f;

  /* scale factor */

  if (peek3.dyn) {
    peek_fact = 1.0f / (float)peek3.dyn;
  } else {
    peek_fact = 1.0f;
  }
  peek_norm = (float)peek1.dyn * peek_fact;

  peek = (float)peek1.dyn / 65536.0f;
  peek_diff = peek - opeek;
  max = peek > peek_max;
  if (max) {
    peek_max = peek;
    //    dbglog(DBG_DEBUG,"+%.2f+ ", peek);
  } else {
    peek_max *= 0.9999999f;
  }

  peek_avgdiff = ((float)peek1.dyn - (float)peek3.dyn) / 65536.0f;

  if (same_sign(peek_diff,opeek_diff)) {
    peek_diff += opeek_diff;
  } else {

    diff_max = opeek_diff > peek_diff_max;
    if (diff_max) {
      peek_diff_max = opeek_diff;
    } 
    //if (opeek_diff > 0)
    //      dbglog(DBG_DEBUG,"[%.2f %.2f] ", opeek_diff, peek_diff_max);

    if (diff_max) {
      flash = opeek_diff * 2.60f;
      //      dbglog(DBG_DEBUG," *");
    

      if (opeek_diff >= 0.30f) {
	zoom += 0.3f;
	//a->flash = -a->flash;
	//dbglog(DBG_DEBUG,"$");
      }
    }
  }
  peek_diff_max *= 0.995f;

  flash *= 0.9f;
  if (flash < 0 && flash > -1E-3) {
    flash = 0;
  }
   
  zoom += peek_norm * 0.5f + peek * 1.1f;
  //  if (zoom > 100.0f) zoom = 100.0f;
  
  if (peek > opeek) {
    float f = (peek-opeek) * 0.95f;
    rspd = f * rspd_max + (1.0f-f) * rspd;
  } else if (peek < opeek) {
    float f = opeek-peek;
    rspd = f * rspd_min + (1.0f-f) * rspd;
  } else {
    rspd = 0.05f * rspd_min + .95f * rspd;
  }

  opeek = peek;
  opeek_diff = peek_diff;  

  {
    float r;
    r = (zoom > ozoom) ? 0.75f : 0.90f;
    ozoom = ozoom * r + zoom * (1.0f-r);
  }
  angle.x += sax * rspd;
  angle.y += say * rspd;
  angle.z += saz * rspd;
  
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
  rspd = 0;

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
  return 0;
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
    INP_DRIVER,           /* Driver type */      
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
