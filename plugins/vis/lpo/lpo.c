/**
 * $Id: lpo.c,v 1.15 2003-01-02 18:32:54 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
static texid_t lpo_texid;
static fftbands_t * bands;

#define RANDOM_MODE 1
#define FLASH_MODE 2

/* Automatic object changes */
static int change_mode = RANDOM_MODE|FLASH_MODE;
static int change_cnt;
static int change_time = 5*1000;

/* Rotation FX */
static float rps_min = 0.1f;   /**< Minimum rotation per second */
static float rps_max = 4.0f;   /**< Maximum rotation per second */
static float rps_cur;          /**< Current rotation speed factor */
static float rps_goal;         /**< Rotation to reach */

static float rps_ch = 0.3f;    /**< Smooth factor when rotation sign change */
static float rps_up = 0.3f;    /**< Smooth factor when rpsd_goal > rps_cur */
static float rps_dw = 0.95f;   /**< Smooth factor when rpsd_goal < rps_cur */

//static float rps_up = 0.25f;   /**< Smooth factor when rpsd_goal > rps_cur */
//static float rps_dw = 0.10f;   /**< Smooth factor when rpsd_goal < rps_cur */

static float rps_sign;         /**< Rotation sens */

static float zoom_min = 0.0f;
static float zoom_max = 7.0f;

static int lpo_remanens = 1;
static unsigned int lpo_iframe = 0;

static int same_sign(float a, float b)
{
  return (a>=0 && b>=0) || (a<0 && b<0);
}

static obj_driver_t * find_object(const char *name)
{
  any_driver_t * d;

  if (!name) {
    return 0;
  }
  //  SDDEBUG("find '%s'\n",name);

  for (d=obj_drivers.drivers; d; d=d->nxt) {
    //    SDDEBUG("-- '%s'\n",d->name);
    
    if (!strcmp(name, d->name)) {
      //      SDDEBUG("OK\n");
      return (obj_driver_t *)d;
    }
  }
  SDERROR("%s(%s) : failed\n", __FUNCTION__, name);
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

extern short int_decibel[];

typedef struct {
  unsigned int max:1;
  unsigned int min:1;
} analyser_state_t;

typedef struct {
  float val;
  float oval;
  float sec_factor;   /* reduct factor per second. */
  float frame_factor; /* reduct factor per frame. */
  float sec_len;      /* len of analyser in second */
  float min;    /* current min value. */
  float max;    /* current max value. */
  float avg;    /* current average value */
  analyser_state_t state;
  analyser_state_t ostate;
} analyser_t;

typedef struct {
  analyser_t analyser[4*3];
} analysers_t;

static analysers_t analysers;

static void init_analyser(analyser_t *a, float sec_len, float reduction)
{
  memset(a,0,sizeof(*a));
  a->sec_factor = pow(reduction, 1.0f/sec_len);
  a->frame_factor = pow(reduction, 1.0f/(60.0f*sec_len)); 
  a->sec_len = sec_len;
  a->max = 0.1;
  a->avg = (a->max+a->min) * 0.5;
}

static void init_analysers(analysers_t *a)
{
  int i,j,k;
  static float times[] = { 4, 1, 0.2 };

  for (i=k=0; i<4; ++i) {
    for (j=0; j<3; ++j, ++k) {
      init_analyser(a->analyser+k, times[j], 0.5);
    }
  }

}

static float reduc(const float va, const float vb, const float f)
{
  float v;
  v = va * f + vb * (1.0f-f);
  return v;
}

static void update_analyser(analyser_t *a, float v, float elapsed_sec)
{
  const float f = a->frame_factor;
  float sec, min,max;

  a->ostate = a->state;

  /* Update value. */
  v = reduc(v, a->oval, 0.5);
  a->oval = a->val;
  a->val = v;

  /* Average */
  sec = a->sec_len - elapsed_sec;
  if (sec < 0) {
    a->avg = v;
  } else {
    const float f = sec / a->sec_len;
    a->avg = reduc(a->avg, v, f);
  }

  /* Min / Max */
  max = reduc(a->max, a->avg, f);
  min = reduc(a->min, a->avg, f);
  if (max < a->avg) max = a->avg;
  if (min > a->avg) min = a->avg;
  a->max = max;
  a->min = min;

  a->state.min = v < min;
  if (a->state.min) {
    a->min = v;
  }

  a->state.max = v > max;
  if (a->state.max) {
    a->max = v;
  }

}

static void update_analysers(analysers_t * a, fftbands_t * bands,
			     float elapsed_sec)
{
  int i,j,k;
  float v;
  static int cnt;

  cnt = (cnt + 1) & 15;

  for (i=k=0; i<4; ++i) {
    int vi;
    if (i<3) {
      vi = bands->band[i].v;
    } else {
      vi = bands->loudness;
    }
    if (vi < 0) {
      vi = 0;
    } else if (vi > 32767) {
      vi = 32767;
    }
    vi = int_decibel[vi];
    v = vi / 32767.0f;

    for (j=0; j<3; ++j, ++k) {
      analyser_t * b = a->analyser+k; 
      update_analyser(b, v, elapsed_sec);
/*       if (!cnt) { */
/* 	printf("#%02d %.02f %.02f %.02f %.02f\n", */
/* 	       k, b->val,b->min,b->avg,b->max); */
/*       } */
    }
  }
}

static int anim(unsigned int ms)
{
  static float swing_latch;
  static float flash_latch;

  const float sec = 0.001f * (float)ms;

  float sax = 1.0f * sec;
  float say = 1.0f * sec;
  float saz = 1.0f * sec;

  float zoom;

/*   const int zoom1_analyser = 3 * 2 + 1; */
/*   const int zoom2_analyser = 3 * 1 + 1; */
  const int zoom1_analyser = 3 * 3 + 1;

  const int flash_analyser = 3 * 2 + 1;
  const int swing_analyser = 3 * 2 + 2;
  const int rotate_analyser = 3 * 1 + 1;
  analyser_t * a;
  float r;

  if (!bands) {
    return -1;
  }
  fft_fill_bands(bands);
  update_analysers(&analysers, bands , sec);

  /* Flash */
  flash_latch -= sec;
  if (flash_latch < 0) {
    flash_latch = 0;
  }
  if (flash_latch == 0) {
    a = analysers.analyser + flash_analyser;
    if (!a->state.max && a->ostate.max) {
      flash = 2.0f;
      if (change_mode & FLASH_MODE) {
	change_object(random_object(curobj));
	change_cnt = 0;
      }
      flash_latch = 0.3;
    }
  }
	
  /* Calculate zoom factor */
  { 
    const float zoom_delta = (zoom_max - zoom_min) / 1;
    zoom = zoom_min;

    a = analysers.analyser + zoom1_analyser;
    if (a->max - a->min > 1E-4) {
      r = (a->val - a->min) / (a->max - a->min);
      zoom += zoom_delta * r;
    }

/*     a = analysers.analyser + zoom2_analyser; */
/*     if (a->max - a->min > 1E-4) { */
/*       r = (a->val - a->min) / (a->max - a->min); */
/*       zoom += zoom_delta * r; */
/*     } */
  }
  r = (zoom > ozoom) ? 0.75f : 0.90f;
  ozoom = ozoom * r + zoom * (1.0f-r);

  /* Swing ! */
  swing_latch -= sec;
  if (swing_latch < 0) {
    swing_latch = 0;
  }

  if (swing_latch == 0) {
    a = analysers.analyser + swing_analyser;
    if (!a->state.max && a->ostate.max) {
      rps_sign = - rps_sign;
      swing_latch = 0.3;
    }
  }

  /* Calculate rotation speed */
  a = analysers.analyser + rotate_analyser;
  if (!a->state.max && a->ostate.max) {
    rps_goal = rps_min;
    if (a->max - a->min > 1E-4) {
      r = (a->val - a->min) / (a->max - a->min);
      rps_goal += (rps_max - rps_min) * r;
    }
    rps_goal *= rps_sign;
  }
  if (!same_sign(rps_cur,rps_goal)) {
    r = rps_ch;
  } else {
    r = fabs(rps_goal) > fabs(rps_cur) ? rps_up : rps_dw;
  }
  rps_cur = rps_cur * r + rps_goal * (1.0f - r);

  flash *= 0.95f;
  
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
  //MtxScale(mtx, ozoom);
  mtx[3][0] = pos.x;
  mtx[3][1] = pos.y;
  mtx[3][2] = pos.z - ozoom;
  
  /* Build render color : blend base and flash color */
  {
    const float f = flash > 1.0f ? 1.0f : flash;
    color.x = base_color.x * (1.0f-f) + flash_color.x * f;
    color.y = base_color.y * (1.0f-f) + flash_color.y * f;
    color.z = base_color.z * (1.0f-f) + flash_color.z * f;
    color.w = base_color.w * (1.0f-f) + flash_color.w * f;
  }

  return 0;
}


static int start(void)
{
  obj_driver_t *o;

  init_analysers(&analysers);

  change_cnt = 0;

  angle.x = 3.14f;
  angle.y = angle.z = angle.z = 0;
  pos.x = pos.y = pos.z = pos.w = 0;
  rps_cur = 0;
  rps_goal = rps_min;
  rps_sign = 1;

  flash = 0;
  ozoom = 0;

  curobj = 0;

  /* $$$ */
#if DEBUG
  {
    any_driver_t *d;
    for (d=obj_drivers.drivers; d; d=d->nxt) {
      SDDEBUG("OBJECT: %s\n", d->name);
    }
  }
#endif


  /* Select mine_3 as 1st object */
  o = find_object("bebop");
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

  curobj->obj.flags = 0
    | DRAW_NO_FILTER
    | DRAW_TRANSLUCENT
    | (lpo_texid << DRAW_TEXTURE_BIT);

  /* Copy viewport and projection matrix for further use (render) */
  viewport = *vp;
  MtxCopy(projmtx, projection);

  /* Check for object change */
  if ((change_mode&RANDOM_MODE) && (change_cnt += elapsed_ms) >= change_time) {
    change_cnt -= change_time;
    change_object(random_object(curobj));
  }

  anim(elapsed_ms);


  if (lpo_remanens) {
    remanens_push(&curobj->obj, mtx, ++lpo_iframe);
  }

  return 0;
}


static int opaque_render(void)
{
  return 0;
}

/* Light vector */ 
static vtx_t light = {
  0, 0, 1.0, 1.0f
};

/* Ambient color */
static vtx_t ambient = {
  0,0,0,0
};

static int transparent_render(void)
{
  if (!curobj) {
    return -1;
  }

  if (lpo_remanens) {
    int age, f, maxf = 2000; /* Max number of face */

    if (curobj->obj.nbf >= maxf) {
      color.w *= 1.5f; 
    }

    for (age=f=0; f<=maxf && color.w > 0.05f; age += 3) {
      remanens_t *r = remanens_get(age);

      if (!r) {
	break;
      }
      if (!r->o) {
	SDWARNING("No stacked object\n");
	break;
      }

      f += r->o->nbf;
      color.w *= 0.75f;

      DrawObjectFrontLighted(&viewport, r->mtx, projmtx,
			     r->o,
			     &ambient, &color);

      /*       DrawObjectSingleColor(&viewport, r->mtx, projmtx, */
      /* 			    r->o, &color); */
    }
    return 0;
  } else {
    return DrawObjectSingleColor(&viewport, mtx, projmtx,
				 &curobj->obj, &color);
  }
}

static int init(any_driver_t *d)
{
  const char * tname = "lpo_bordertile";
  border_def_t borderdef;
  static fftband_limit_t limits[] = {
    {0, 150},
    {150, 1500},
    {1500, 44100},
  }; 

  curobj = 0;
  lpo_texid = texture_get(tname);
  if (lpo_texid < 0) {
    lpo_texid = texture_dup(texture_get("bordertile"), tname);
  }
  border_get_def(borderdef, 1);
  border_customize(lpo_texid, borderdef);

  bands = fft_create_bands(3,limits);

  return -(lpo_texid < 0 || !bands);;
}

static int shutdown(any_driver_t *d)
{
  stop();
  if (bands) {
    free(bands);
    bands = 0;
  }
  return 0;
}

static driver_option_t * options(struct _any_driver_s *driver,
				 int idx, driver_option_t * opt)
{
  return opt;
}

#include "luashell.h"

static int lua_setbordertex(lua_State * L)
{
  border_def_t borderdef;

  border_get_def(borderdef,lua_tonumber(L, 1));
  border_customize(lpo_texid, borderdef);
  return 0;
}

static int lua_custombordertex(lua_State * L)
{
  int i;
  int max = lua_gettop(L);
  static border_def_t borderdef = {
    {1,1,1,1}, {0.5, 0.5, 0.5, 0.5}, {0.7, 0.2, 0.2, 0.2}
  };

  if (max > 12) max = 12;
  for (i=0; i<max; ++i) {
    if (lua_type(L,i+1) != LUA_TNIL) {
      ((float *)&borderdef[i>>2])[i&3] = lua_tonumber(L,i+1);
    }
  }
  border_customize(lpo_texid,borderdef);
  return 0;
}  

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

static int lua_setbasecolor(lua_State * L)
{
  lua_setcolor(L,(float *) &base_color);
  return 0;
}

static int lua_setflashcolor(lua_State * L)
{
  lua_setcolor(L,(float *) &flash_color);
  return 0;
}

static int lua_setremanens(lua_State * L)
{
  int remanens = lpo_remanens;

  lpo_remanens = lua_type(L,1) != LUA_TNIL;
  lua_settop(L,0);
  if (remanens) {
    lua_pushnumber(L,1);
  }
  return lua_gettop(L);
}

static int lua_setchange(lua_State * L)
{
  int mode = change_mode;
  float time = (float)change_time/1000.0f, new_time;
  
  change_mode = lua_tonumber(L,1);
  new_time = lua_tonumber(L,2) * 1000.0f;
  if (new_time > 0) {
    change_time = new_time;
  }
  lua_settop(L,0);
  lua_pushnumber(L, mode);
  lua_pushnumber(L, time);
  return 2;
}

static int lua_setobject(lua_State * L)
{
  const char * name = curobj ? curobj->obj.name : 0;

  change_object(find_object(lua_tostring(L,1)));
  change_cnt = 0;
  lua_settop(L,0);
  if (name) lua_pushstring(L,name);
  return lua_gettop(L);
}

static luashell_command_description_t commands[] = {
  {
    "lpo_setambient", 0,              /* long and short names */
    "print [["
    "lpo_setambient(a, r, g, b) : set ambient color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setambient      /* function */
  },
  {
    "lpo_setbasecolor", 0,         /* long and short names */
    "print [["
    "lpo_setbasecolor(a, r, g, b) : set object base color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbasecolor /* function */
  },
  {
    "lpo_setflashcolor", 0,         /* long and short names */
    "print [["
    "lpo_setflashcolor(a, r, g, b) : set object flash color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setflashcolor /* function */
  },
  {
    "lpo_setbordertex", 0,            /* long and short names */
    "print [["
    "lpo_setbordertex(num) : set border texture type."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbordertex    /* function */
  },
  {
    "lpo_custombordertex", 0,            /* long and short names */
    "print [["
    "lpo_custombordertex(a1,r1,g1,b1, a2,r2,g2,b2, a3,r3,g3,b3) : "
    "set custom border texture. Each color componant could be set to nil to "
    "keep the current value.\n"
    " a1,r1,g1,b1 : border color\n"
    " a2,r2,g2,b2 : fill color\n"
    " a3,r3,g3,b3 : link color\n"
    "]]",                                   /* usage */
    SHELL_COMMAND_C, lua_custombordertex    /* function */
  },


  {
    "lpo_setremanens", 0,            /* long and short names */
    "print [["
    "lpo_setremanens(boolean) : active/desacitive remanens FX. "
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setremanens    /* function */
  },
  {
    "lpo_setchange", 0,            /* long and short names */
    "print [["
    "lpo_setchange(type [, time]) : Set object change properties. "
    "type [0,nil:no-change 1:random 2:flash 3 random and flash."
    "Return old values."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setchange    /* function */
  },
  {
    "lpo_setobject", 0,            /* long and short names */
    "print [["
    "lpo_setobject(name) : Set and get current object."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setobject    /* function */
  },

  {0},                                   /* end of the command list */
};

static vis_driver_t driver = {

  /* Any driver. */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h) */
    VIS_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "lpo",                /* Driver name */
    "Benjamin Gerard\0",  /* Driver authors */
    "Little "             /**< Description */
    "Pumping "
    "Object",
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
    commands,             /**< Lua shell commands  */
  },

  start,                  /**< Driver start */
  stop,                   /**< Driver stop */  
  process,                /**< Driver post render calculation */
  opaque_render,          /**< Driver opaque render */
  transparent_render      /**< Driver transparent render */

};

EXPORT_DRIVER(driver)
