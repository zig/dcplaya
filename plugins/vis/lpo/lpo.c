/**
 * $Id: lpo.c,v 1.22 2003-01-25 11:37:44 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dc/controller.h>
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
#include "controler.h"

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
#define RND_BORDER_MODE 4

/* Automatic object changes */
static int change_mode = RANDOM_MODE|FLASH_MODE|RND_BORDER_MODE;
//static int change_mode = 0;
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

static int lpo_controler = -1;
static int lpo_controler_binding = -1;

static int lpo_opaque = 0;
static int lpo_lighted = 1;
static int lpo_remanens = 1;
static unsigned int lpo_iframe = 0;

static void lpo_set_controler(int cont)
{
  cont = cont - 1;
  if (cont >= 0) {
    cont &= 31;
  } else {
    cont = -1;
  }

  printf("[lpo_set_controler] old:%d new:%d\n",lpo_controler,cont);

  if (cont != lpo_controler) {
    int clear = 0, set = 0;
    if (lpo_controler >=0) {
      /* Reset current lpo controller binding. */
      int m = 1 << lpo_controler;
      clear = m;
      set   = lpo_controler_binding & m;
    }

    if (cont >= 0) {
      int m = 1 << cont;
      clear |= m;
    }

    printf("[lpo_set_controler] clear:%08x set:%08x\n",clear,set);
    lpo_controler_binding = controler_binding(clear, set);
    lpo_controler = cont;
    printf("[lpo_set_controler] : binding old:%08x new:%08x\n",
	   lpo_controler_binding, controler_binding(0,0));
  }
}

static int same_sign(float a, float b)
{
  return (a>=0 && b>=0) || (a<0 && b<0);
}

static obj_driver_t * find_object(const char *name)
{
  if (!name) {
    return 0;
  }
  return (obj_driver_t *)driver_list_search(&obj_drivers, name);
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

  driver_list_lock(&obj_drivers);
  if (obj_drivers.n == 0) {
    driver_list_unlock(&obj_drivers);
    return 0;
  }
  o = num_object(rand() % (unsigned int)obj_drivers.n);
  if (o == not_this_one) {
    if (o->common.nxt) {
      o = (obj_driver_t *)o->common.nxt;
    } else {
      o = (obj_driver_t *)obj_drivers.drivers;
    }
  }
  if (o == not_this_one) {
    o = 0;
  } else if (o) {
    driver_reference(&o->common);
  }
  driver_list_unlock(&obj_drivers);
  return o;
}

static int change_object(obj_driver_t *o)
{
  if (!o) {
    return -1;
  }
  if (curobj) {
    driver_dereference(&curobj->common);
  }
  curobj = o;

  // $$$
  if (0 && o && o->obj.nvx) {
    int i, err=0;
/*     SDDEBUG("Build normals\n"); */
    for (i=0; i<o->obj.nbf; ++i) {
      err += (1 - vtx_sqnorm(o->obj.nvx+i)) > MF_EPSYLON;
    }
    if (err) {
      SDDEBUG("[%s] : %d errors in normals !!\n", o->obj.name, err);
    }
      
/*       o->obj.nvx[i].x = 1; */
/*       o->obj.nvx[i].y = 0; */
/*       o->obj.nvx[i].z = 0; */
/*       o->obj.nvx[i].w = 1; */
//      FaceNormal(&o->obj.nvx[i].x, o->obj.vtx, o->obj.tri+i);
    
  }

  if (0 && o && o->obj.tri) {
    int i;
    SDDEBUG("Inverting face def\n");
    for (i=0; i<o->obj.nbf; ++i) {
      int a = o->obj.tri[i].a;
      int b = o->obj.tri[i].b;
      int c = o->obj.tri[i].c;

      o->obj.tri[i].a = b;
      o->obj.tri[i].b = a;
      o->obj.tri[i].c = c;
    }
    obj3d_build_normals(&o->obj);
  }

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
  static float border_latch;

  const float sec = 0.001f * (float)ms;

  vtx_t sa;
  float zoom;


/*   const int zoom1_analyser = 3 * 2 + 1; */
/*   const int zoom2_analyser = 3 * 1 + 1; */
  const int zoom1_analyser = 3 * 3 + 1;

  const int border_analyser = 3 * 0 + 1;

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

  vtx_set(&sa,sec,sec * 0.91,sec * 0.93);

  /* Flash */
  flash_latch -= sec;
  if (flash_latch < 0) {
    flash_latch = 0;
  }
  if (flash_latch == 0) {
    a = analysers.analyser + flash_analyser;
    if (!a->state.max && a->ostate.max) {
      if (change_mode & FLASH_MODE) {
	flash = 2.0f;
      }
      if (change_mode & RANDOM_MODE) {
	change_object(random_object(curobj));
	change_cnt = 0;
      }
      flash_latch = 0.3;
    }
  }

  /* Border change */
  border_latch -= sec;
  if (border_latch < 0) {
    border_latch = 0;
  }
  if (border_latch == 0) {
    a = analysers.analyser + border_analyser;
    if (!a->state.max && a->ostate.max) {
      if (change_mode & RND_BORDER_MODE) {
	border_def_t borderdef;
	border_get_def(borderdef,(unsigned short)rand());
	border_customize(lpo_texid, borderdef);
      }
      border_latch = 0.3;
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
  r = (zoom > ozoom) ? 0.6 : 0.90f;
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


  if (lpo_controler >= 0) {
    controler_state_t state;
    int err = controler_read(&state, lpo_controler);
    if (!err) {
      vtx_set(&sa,
	      sec * (float)state.joyy * (1.0/32.0),
	      sec * (float)state.joyx * (1.0/32.0),
	      0);
      if (controler_pressed(&state, CONT_A)) {
	change_object(random_object(curobj));
      }
      if (controler_pressed(&state, CONT_START|CONT_B)) {
	lpo_set_controler(-1);
      }
    } else {
      lpo_set_controler(-1);
    }
    pos.z = 2.0;
  }

  if (lpo_controler < 0) {
    vtx_scale(&sa, rps_cur);
    pos.z = zoom_max + 1.7 - ozoom;
  }

  vtx_inc_angle(&angle, &sa);

  // $$$

  /* Build local matrix */
  MtxIdentity(mtx);
  MtxRotateX(mtx, angle.x);
  MtxRotateY(mtx, angle.y);
  MtxRotateZ(mtx, angle.z);
  //MtxScale(mtx, ozoom);
  mtx[3][0] = pos.x;
  mtx[3][1] = pos.y;
  // $$$ TEST REMOVE NEXT COMMENT
  mtx[3][2] = pos.z;
  
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
  change_object(random_object(0));
  return 0;
}

static int stop(void)
{
  lpo_set_controler(-1);
  if (curobj) {
    driver_dereference(&curobj->common);
    curobj = 0;
  }
  return 0;
}

static int process(viewport_t * vp, matrix_t projection, int elapsed_ms)
{
  if (!curobj) {
    return -1;
  }

  /* Copy viewport and projection matrix for further use (render) */
  viewport = *vp;
  MtxCopy(projmtx, projection);

  /* Check for object change */
  if ((change_cnt += elapsed_ms) >= change_time) {
    change_cnt -= change_time;
    if (change_mode&RANDOM_MODE) {
      change_object(random_object(curobj));
      if (!curobj) {
	return -1;
      }
    }
  }
  anim(elapsed_ms);
  if (lpo_remanens && !lpo_opaque) {
    remanens_push(&curobj->obj, mtx, ++lpo_iframe);
  }

  return 0;
}

/* Ambient color */
static vtx_t ambient = {
  0,0,0,0
};

int render(void)
{
  vtx_t flat_color;

  if (!curobj || lpo_texid <= 0) {
    return -1;
  }

  curobj->obj.flags = 0
    | DRAW_NO_FILTER
    | (lpo_opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (lpo_texid << DRAW_TEXTURE_BIT);

  if (!lpo_lighted) {
    draw_color_add((draw_color_t *)&flat_color,
		   (draw_color_t *)&ambient, (draw_color_t *)&color);
  }

  if (lpo_remanens && !lpo_opaque) {
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

      if (lpo_lighted) {
	color.w *= 0.75f;
	DrawObjectFrontLighted(&viewport, r->mtx, projmtx,
			       r->o,
			       &ambient, &color);
      } else {
	flat_color.w *= 0.75f;
	DrawObjectSingleColor(&viewport, r->mtx, projmtx,
			      r->o, &flat_color);
      }
    }
  } else {
    if (lpo_lighted) {
      DrawObjectFrontLighted(&viewport, mtx, projmtx,
			     &curobj->obj,
			     &ambient, &color);
    } else {
      DrawObjectSingleColor(&viewport, mtx, projmtx,
			    &curobj->obj, &flat_color);
    }
  }
  return 0;
}

static int opaque_render(void)
{
  if (!lpo_opaque) {
    return 0;
  }
  return render();
}


static int transparent_render(void)
{
  if (lpo_opaque) {
    return 0;
  }
  return render();
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

  lpo_controler_binding = controler_binding(0, 0);
  lpo_controler = -1;

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

static int lua_setboolean(lua_State * L, int * v)
{
  int old = *v;

  if (lua_gettop(L) >= 1) {
    *v = lua_tonumber(L,1) != 0;
  }
  lua_settop(L,0);
  if (old) {
    lua_pushnumber(L,1);
  }
  return lua_gettop(L);
}

static int lua_setinteger(lua_State * L, int * v)
{
  int old = *v;

  if (lua_gettop(L) >= 1) {
    *v = lua_tonumber(L,1);
  }
  lua_settop(L,0);
  lua_pushnumber(L,old);
  return lua_gettop(L);
}

static int lua_setremanens(lua_State * L)
{
  return lua_setboolean(L, &lpo_remanens);
}

static int lua_setlighting(lua_State * L)
{
  return lua_setboolean(L, &lpo_lighted);
}

static int lua_setopaque(lua_State * L)
{
  return lua_setboolean(L, &lpo_opaque);
}

static int lua_setcontroller(lua_State * L)
{
  int c = lpo_controler;
  int r = lua_setinteger(L, &c);
  lpo_set_controler(c);
  return r;
}

static int lua_setchange(lua_State * L)
{
  int mode = change_mode;
  float time = (float)change_time/1000.0f, new_time;
  int n = lua_gettop(L);
  
  if (n>=1 && lua_type(L,1) != LUA_TNIL) {
    change_mode = lua_tonumber(L,1);
  }
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
    "lpo_setremanens([boolean]) : get/set remanens FX. "
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setremanens    /* function */
  },
  {
    "lpo_setopacity", 0,            /* long and short names */
    "print [["
    "lpo_setopacity([boolean]) : get/set opacity mode. "
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setopaque    /* function */
  },
  {
    "lpo_setcontroller", 0,            /* long and short names */
    "print [["
    "lpo_setcontroller([cont_id]) : set/get controller control."
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setcontroller    /* function */
  },

  {
    "lpo_setchange", 0,            /* long and short names */
    "print [["
    "lpo_setchange(type [, time]) : Set object change properties. "
    "type bit0:random-object bit1:active-flash bit2:auto-border."
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
  {
    "lpo_setlighting", 0,            /* long and short names */
    "print [["
    "lpo_setlighting([boolean]) : Set/Get lighting process."
    "Return old values."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setlighting    /* function */
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
