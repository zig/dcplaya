/** @ingroup dcplaya_vis_driver
 *  @file    hyperpipe.c
 *  @author  benjamin gerard 
 *  @date    2003/01/14
 *
 *  $Id: hyperpipe.c,v 1.1 2003-01-14 10:54:02 ben Exp $
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

#ifndef PI
# define PI 3.14159265359
#endif

#define MIN_RAY 0.05
#define MAX_RAY 0.23
#define W       64u
#define N       8u
#define V       (W*N)
#define T       ((N-2u)*2u + (W-1u) * (2u*N))
#define X       2.0

extern void FaceNormal(float *d, const vtx_t * v, const tri_t *t);

/* VLR 3d-objects */
static vtx_t *v;   /* Vertex buffer */
static vtx_t *nrm; /* Normal buffer */
static tri_t *tri; /* Triangle buffer */
static tlk_t *tlk; /* Triangle link buffer */

volatile int ready;

static viewport_t viewport;   /**< Current viewport */
static matrix_t mtx;          /**< Current local matrix */
static matrix_t proj;         /**< Current projection matrix */
static vtx_t color;           /**< Final Color */
static vtx_t base_color =     /**< Original color */
  {
    1.5f, 0.5f, -1.5f, -0.7f
  };
static vtx_t flash_color =     /**< Flashing color */
  {
    0.5f, 0.5f, 1.0f, 0.8f
  };

/* Ambient color */
static vtx_t ambient = {
  0.0,0.5,0.8,1.3
};



static vtx_t pos;     /**< Object position */
static vtx_t angle;   /**< Object rotation */
static float flash;
static float ozoom;
static texid_t texid;
static fftbands_t * bands;

/* Automatic object changes */
static int change_mode = 0;
static int change_cnt;
static int change_time = 5*1000;

static int use_remanens = 1;

extern short int_decibel[];

/* The 3D-object */
static obj_t hyperpipe_obj =
{
  "hyperpipe",    /* Name */
  0,              /* Flags */
  V,              /* # of vertex */
  T,              /* # of triangle */
  V,              /* bis */
  T,              /* bis */
};

static void set_obj_pointer(void)
{
  hyperpipe_obj.vtx = v;
  hyperpipe_obj.nvx = nrm;
  hyperpipe_obj.tri = tri;
  hyperpipe_obj.tlk = tlk;
}

static void hyperpipe_free(void)
{
  SDDEBUG("hyperpipe : free()\n");
  ready = 0;
  if (v) {
    free(v);
    v = 0;
  }
 SDDEBUG("hyperpipe : vtx freed\n");
 
  if (nrm) {
    free(nrm);
    nrm = 0;
  }
 SDDEBUG("hyperpipe : nrm freed\n");


  if (tri) {
    free(tri);
    tri = 0;
  }
 SDDEBUG("hyperpipe : tri freed\n");


  if(tlk) {
    free (tlk);
    tlk = 0;
  }
  SDDEBUG("hyperpipe : tlk freed\n");

  set_obj_pointer();
  SDDEBUG("hyperpipe : freed()\n");
}

static int hyperpipe_alloc(void)
{
  hyperpipe_free(); /* Safety net */

  SDDEBUG("hyperpipe : alloc vtx:%d tri:%d\n",
	  hyperpipe_obj.nbv,hyperpipe_obj.nbf);

  v   = (vtx_t *)malloc(sizeof(vtx_t) * hyperpipe_obj.nbv);
  nrm = (vtx_t *)malloc(sizeof(vtx_t) * hyperpipe_obj.nbf);
  tri = (tri_t *)malloc(sizeof(tri_t) * (hyperpipe_obj.nbf+1));
  tlk = (tlk_t *)malloc(sizeof(tlk_t) * hyperpipe_obj.nbf);
  if (!v || !nrm || !tri || !tlk) {
    hyperpipe_free();
    return -1;
  }
  set_obj_pointer();
  return 0;
}

static vtx_t ring[N];

static void build_ring(void)
{
  vtx_t *v,*ve;
  float a, stp;
  for(v=ring, ve=v+N, a=0, stp = 2*PI/N; v<ve; ++v, a+=stp) {
    v->x = cosf(a);
    v->y = sinf(a);
    v->z = 0;
    v->w = 1;
  }
}

static int tesselate(tri_t * tri, tlk_t * tlk, int base, int n)
{
  int i;
  if (n<3) return 0;

  if (!base) {
    for (i=0; i<n-2; ++i) {
      tlk[i].flags = tri[i].flags = 0;

      tri[i].a = 0;
      tri[i].b = i+1;
      tri[i].c = i+2;

      tlk[i].a = !i ? T : i-1;
      tlk[i].b = T;
      tlk[i].c = (i == n-3) ? T : i+1;

    }
  } else {
    int m = n-2;
    base = base - n;
    for (i=0; i<m; ++i) {
      tlk[i].flags = tri[i].flags = 0;

      tri[i].a = base;
      tri[i].c = base+i+1;
      tri[i].b = base+i+2;

      tlk[i].c = T-1;
      tlk[i].a = tlk[i].b = T-1;
      tlk[i].b = T;
    }
  }
  return i;
}

static int stop(void)
{
  SDDEBUG("[hp] : stop()\n");
  ready = 0;
  hyperpipe_free();
  SDDEBUG("[hp] : stopped()\n");
  return 0;
}

static int start(void)
{
  int i, j, k, w;
  vtx_t *vy;

  SDDEBUG("[hp] : start()\n");

  ready = 0;

  SDDEBUG("[hp] : ring()\n");
  build_ring();

  SDDEBUG("[hp] : alloc()\n");

  /* Alloc buffers */
  if (hyperpipe_alloc()) {
    return -1;
  }

  SDDEBUG("[hp] : vertrices()\n");

  /* Build vertrices */
  for(j=0, vy=v; j<W; ++j) {
    for (i=0; i<N; ++i, ++vy){
      vy->x = ((float)j-(float)W*0.5) * X / W;
      vy->y = ring[i].x * MAX_RAY;
      vy->z = ring[i].y * MAX_RAY;
      vy->w = 1.0f;
    }
  }
  if (vy-v != hyperpipe_obj.nbv) {
    SDERROR("hyperpipe : wrong number of vtx [%d != %d]\n",
	    vy-v,hyperpipe_obj.nbv);
    stop();
    return -1;
  }

  SDDEBUG("[hp] : close 1()\n");
  /* First close faces */
  k = tesselate(tri,tlk,0,N);

  SDDEBUG("[hp] : faces() k:=%d\n",k);

  /* Build faces and link-faces */
  for(w=j=0; j<W-1; ++j) {
    int k1 = k;
    for (i=0; i<N; ++i, k += 2, ++w) {
      const int tpl = N*2;

      tlk[k+0].flags = tri[k+0].flags = 0;
      tlk[k+1].flags = tri[k+1].flags = 0;

      tri[k+0].a = w;
      tri[k+1].a = tri[k+0].b = w + N;
      tri[k+1].c = tri[k+0].c = w - ((i==N-1) ? i : -1);
      tri[k+1].b = tri[k+0].c+N;

      tlk[k+0].a = k1 + ((i+N-1)%N)*2 + 1;
      tlk[k+0].b = k+1;
      tlk[k+0].c = k-tpl+1;
      if (tlk[k+0].c < (int)N-2) {
	tlk[k+0].c = T; /* invisible */
      }

      tlk[k+1].a = k+tpl;
      if (tlk[k+1].a >= T-(N-2)) {
	tlk[k+1].a = T; /* invisible */
      }
      tlk[k+1].b = k1 + ((i+1)%N)*2;
      tlk[k+1].c = k;

      // $$$
/*       tlk[k+0].a = tlk[k+0].b = tlk[k+0].c =  */
/* 	tlk[k+1].a = tlk[k+1].b = tlk[k+1].c = T; */

    }
  }

  SDDEBUG("[hp] : close end() k:=%d\n",k);

  k += tesselate(tri+k,tlk+k,V,N);
/*   for (;k<T;++k) { */
/*     tri[k].a = tri[k].b = tri[k].c = T-1; */
/*     tlk[k].a = tlk[k].b = tlk[k].c = T; */
/*     tri[k].flags = tlk[k].flags = 0; */
/*   } */

  if (k != T) {
    SDERROR("hyperpipe: Init error, wromg number of face %d != %d\n",k,T);
    stop();
    return -1;
  }

  for (i=0;i<T; ++i) {
    for (j=0; j<3; ++j) {
      unsigned int * ptr;
      ptr = ((unsigned int*)(&tri[i].a))+j;
      if (*ptr >= (unsigned int)V) {
	printf("Face #%d %c : out of range [%d >= %d]\n",i,'A'+j, *ptr, V);
	*ptr = 0;
      }
      ptr = ((unsigned int*)(&tlk[i].a))+j;
      if (*ptr > (unsigned int)T) {
	printf("Link #%d %c : out of range [%d > %d]\n", i,'A'+j, *ptr, T);
	*ptr = T;
      }
    }
  }

  SDDEBUG("[hp] : finish close()\n");

  tri[T].a = tri[T].b = tri[T].c = 0;
  tri[T].flags = 1; /* Setup invisible face */

  for (i=0; i<hyperpipe_obj.nbf; ++i) {
    FaceNormal((float *)(nrm+i),v,tri+i);
  }

/*   hyperpipe_obj.nbf = T - (N-2) - N; */

  /* Setup matrix*/
  MtxIdentity(proj);
  MtxIdentity(mtx);

  ready = 1;
  return 0;
}

/* $$$ Defined in libdcutils */
extern int rand();

static int anim(unsigned int ms)
{
  /* Build local matrix */
  MtxIdentity(mtx);
/*   MtxRotateX(mtx, angle.x); */
/*   MtxRotateY(mtx, angle.y); */
/*   MtxRotateZ(mtx, angle.z); */
  //MtxScale(mtx, ozoom);
/*   mtx[3][0] = pos.x; */
/*   mtx[3][1] = pos.y; */
/*   mtx[3][2] = pos.z - ozoom; */

  return 0;
}

static int process(viewport_t * vp, matrix_t projection, int elapsed_ms)
{
  static float ay, az, paz = 0.01f;

  matrix_t tmp;

  if (!ready) {
    return -1;
  }

  {
    static short spl[W];
    short spl2[W];
    int i;
    vtx_t * vy;
    //    const float s = (MAX_RAY-MIN_RAY) / 32768.0f;
    const float s = (MAX_RAY-MIN_RAY) / sqrtf(32768.0f);

    fft_fill_pcm(spl2,W);
    for (i=0, vy=v; i<W; ++i) {
      int j,v = spl[i];
      float s1,s2;

      spl[i] = v = (v+v+v+spl2[i]) >> 2;
      if (v<0) {
	s1 = 0;
	s2 = sqrtf(-(v+1)) * s;
      } else {
	s1 = sqrtf(v) * s;
	s2 = 0;
      }
	
      const float f1 = MIN_RAY + s1;
      const float f2 = MIN_RAY + s2;

      for (j=0; j<(N>>1); ++j, ++vy) {
	vy->y = ring[j].x * f1;
	vy->z = ring[j].y * f1;
      }
      for (; j<N; ++j, ++vy) {
	vy->y = ring[j].x * f2;
	vy->z = ring[j].y * f2;
      }

    }
    {
      const int n = hyperpipe_obj.nbf;
      for (i=0; i<n; ++i) {
	FaceNormal((float *)(nrm+i),v,tri+i);
      }
    }
  }
    

  viewport = *vp;
  MtxCopy(proj, projection);

  MtxIdentity(mtx);

/*   MtxRotateZ(mtx, az); */
  az = sinf(paz += 0.0233f) * 0.22f;

  //  MtxRotateX(mtx, 4.3f * ay);
  MtxRotateY(mtx, ay += 0.014*0.52);
  MtxRotateX(mtx, 0.3f + az);
  mtx[3][2] = X * 0.5 + 1.00;
  if (ay > 2*PI) {
    ay -= 2*PI;
  }

#if 0
  MtxIdentity(tmp);
  MtxRotateZ(tmp, 3.14159);
/*   MtxRotateY(tmp, -0.33468713*ay); */
  MtxRotateX(tmp, 0.4);
  MtxTranspose(tmp);
  MtxVectMult(&tlight_normal, &light_normal, tmp);
#endif

/*     vlr_update(); */

  hyperpipe_obj.flags = 0
    | DRAW_NO_FILTER
    | DRAW_TRANSLUCENT
    | (texid << DRAW_TEXTURE_BIT);
  return 0;
}

static int opaque_render(void)
{
  return 0;
}

static int transparent_render(void)
{
  if (!ready) {
    return -1;
  }

  color = base_color;
  DrawObjectFrontLighted(&viewport, mtx, proj,
			 &hyperpipe_obj,
			 &ambient, &color);
  return 0;
}


static int init(any_driver_t *d)
{
  const char * tname = "pipe_bordertile";
  border_def_t borderdef;

  SDDEBUG("init [%s]\n",d->name);
/*   static fftband_limit_t limits[] = { */
/*     {0, 150}, */
/*     {150, 1500}, */
/*     {1500, 44100}, */
/*   };  */

  ready = 0;
  bands = 0;
  v = 0;
  nrm = 0;
  tri = 0;
  tlk = 0;

  texid = texture_get(tname);
  if (texid < 0) {
    texid = texture_dup(texture_get("bordertile"), tname);
  }
  border_get_def(borderdef, 1);
  border_customize(texid, borderdef);
/*   bands = fft_create_bands(3,limits); */
  SDDEBUG("init [%s] := [%d]\n",d->name, texid);

  return -(texid < 0); // || !bands);;
}

static int shutdown(any_driver_t *d)
{
  SDDEBUG("[hp] shutdown\n");
  stop();
  if (bands) {
    free(bands);
    bands = 0;
  }
  SDDEBUG("[hp] shutdowned\n");
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

  border_get_def(borderdef, lua_tonumber(L, 1));
  border_customize(texid, borderdef);
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
  border_customize(texid,borderdef);
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
  int remanens = use_remanens;

  use_remanens = lua_type(L,1) != LUA_TNIL;
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

static luashell_command_description_t commands[] = {
  {
    "hyperpipe_setambient", 0,              /* long and short names */
    "print [["
    "hyperpipe_setambient(a, r, g, b) : set ambient color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setambient      /* function */
  },
  {
    "hyperpipe_setbasecolor", 0,         /* long and short names */
    "print [["
    "hyperpipe_setbasecolor(a, r, g, b) : set object base color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbasecolor /* function */
  },
  {
    "hyperpipe_setflashcolor", 0,         /* long and short names */
    "print [["
    "hyperpipe_setflashcolor(a, r, g, b) : set object flash color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setflashcolor /* function */
  },
  {
    "hyperpipe_setbordertex", 0,            /* long and short names */
    "print [["
    "hyperpipe_setbordertex(num) : set border texture type."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbordertex    /* function */
  },
  {
    "hyperpipe_custombordertex", 0,            /* long and short names */
    "print [["
    "hyperpipe_custombordertex(a1,r1,g1,b1, a2,r2,g2,b2, a3,r3,g3,b3) : "
    "set custom border texture. Each color componant could be set to nil to "
    "keep the current value.\n"
    " a1,r1,g1,b1 : border color\n"
    " a2,r2,g2,b2 : fill color\n"
    " a3,r3,g3,b3 : link color\n"
    "]]",                                   /* usage */
    SHELL_COMMAND_C, lua_custombordertex    /* function */
  },

  {
    "hyperpipe_setremanens", 0,            /* long and short names */
    "print [["
    "hyperpipe_setremanens(boolean) : active/desacitive remanens FX. "
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setremanens    /* function */
  },
  {
    "hyperpipe_setchange", 0,            /* long and short names */
    "print [["
    "hyperpipe_setchange(type [, time]) : Set object change properties. "
    "type bit0:random-object bit1:active-flash bit2:auto-border."
    "Return old values."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setchange    /* function */
  },

  {0},                                   /* end of the command list */
};

static vis_driver_t driver = {

  /* Any driver. */
  {
    NEXT_DRIVER,          /* Next driver (see any_driver.h) */
    VIS_DRIVER,           /* Driver type */      
    0x0100,               /* Driver version */
    "hyperpipe",          /* Driver name */
    "Benjamin Gerard\0",  /* Driver authors */
    "Hyper Groovy Pipe "  /* Description */
    "Object",
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
