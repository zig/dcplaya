/**
 * @ingroup  visual_plugin
 * @file     fftvlr.c
 * @author   Vincent Penne
 * @author   benjamin gerard <ben@sashipa.com>
 * @date     2002/08/17
 * @brief    dreammp3 visual plugin - fftvlr.
 * 
 * (C) COPYRIGHT 2002 Vincent Penne & Ben(jamin) Gerard
 *
 * $Id: fftvlr.c,v 1.25 2003-01-25 11:37:44 ben Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "draw/ta.h"
#include "draw/vertex.h"
#include "border.h"
#include "fft.h"
#include "vis_driver.h"
#include "obj3d.h"
//#include "draw_vertex.h"
#include "draw_object.h"
#include "sysdebug.h"

//#define BENSTYLE

extern short int_decibel[];

static int db_scaling;
static fftbands_t * bands;

/* From obj3d.c */
void FaceNormal(float *d, const vtx_t * v, const tri_t *t);

/* Here are the constant (for now) parameters */
#define VLR_X 1.0f
#define VLR_Z 1.0f
#define VLR_Y 0.2f
/*#define VLR_W 32
#define VLR_H 96*/
#define VLR_W 30
#define VLR_H 30


/* Resulting number of triangles and vertrices */
#define VLR_V (VLR_W*VLR_H)
#define VLR_TPL ((VLR_W-1)*2)
#define VLR_T   (VLR_TPL * (VLR_H-1))

/* VLR 3d-objects */
static vtx_t *v;   /* Vertex buffer */
static vtx_t *nrm; /* Normal buffer */
static tri_t *tri; /* Triangle buffer */
static tlk_t *tlk; /* Triangle link buffer */

volatile int ready;

static viewport_t viewport;   /* Graphic viewport */
static matrix_t fftvlr_proj;  /* Projection matrix */
static matrix_t fftvlr_mtx;   /* Local matrix */
static texid_t fftvlr_texid;  /* Custom texid. */
static int fftvlr_opacity = 0; /* Opaque/translucent rendering */

static vtx_t light_normal = { 
  0.8,
  0.4,
  0.5
};

static vtx_t tlight_normal;

#ifdef BENSTYLE

static vtx_t light_color = {
    1.5,
    1.1,
    1.5,
    -0.3
};

static vtx_t ambient_color = {
   0.0,
  -0.2,
  -1.0,
  0.8
};

#else

static vtx_t light_color = {
  0.9,
  0.90,
  0.9,
  0.5
};

static vtx_t ambient_color = {
  0.5,
  0.3,
  0.1,
  0.5
};

#endif

/*static vtx_t light_color = {
    1.5,
    1.0,
    0.5,
    -0.3
};

static vtx_t ambient_color = {
  0,
  0,
  0,
  0.8
};
*/

/* The 3D-object */
static obj_t fftvlr_obj =
{
  "fft-vlr",      /* Name */
  0,              /* Flags */
  VLR_W * VLR_H,  /* # of vertex */
  VLR_T,          /* # of triangle */

  VLR_W * VLR_H,  /* bis */
  VLR_T,          /* bis */
};

static void set_obj_pointer(void)
{
  fftvlr_obj.vtx = v;
  fftvlr_obj.nvx = nrm;
  fftvlr_obj.tri = tri;
  fftvlr_obj.tlk = tlk;
}

static void fftvlr_free(void)
{
  ready = 0;
  if (v) {
    free(v);
    v = 0;
  }
  if (nrm) {
    free(nrm);
    nrm = 0;
  }
  if (tri) {
    free(tri);
    tri = 0;
  }
  if(tlk) {
    free (tlk);
    tlk = 0;
  }
  set_obj_pointer();
}

static int fftvlr_alloc(void)
{
  fftvlr_free(); /* Safety net */
  v   = (vtx_t *)malloc(sizeof(vtx_t) * (VLR_H*VLR_W));
  nrm = (vtx_t *)malloc(sizeof(vtx_t) * (VLR_T+1));
  tri = (tri_t *)malloc(sizeof(tri_t) * (VLR_T+1)); /* +1 for invisible */
  tlk = (tlk_t *)malloc(sizeof(tlk_t) * (VLR_T+1));
  if (!v || !nrm || !tri || !tlk) {
    fftvlr_free();
    return -1;
  }
  memset(nrm, 0, sizeof(*nrm) * (VLR_T+1));
  set_obj_pointer();

  return 0;
}

static int fftvlr_start(void)
{
  int i, j, k;
  vtx_t *vy;

  ready = 0;

  /* Alloc buffers */
  if (fftvlr_alloc()) {
    return -1;
  }

  /* Build vertrices double buffer (ring buffer)  */
  for(j=0, vy=v; j<VLR_H; ++j) {
    for (i=0; i<VLR_W; ++i, ++vy){
      vy->x = (i-(VLR_W>>1)) * VLR_X / VLR_W;
      vy->y = 0;
      vy->z = (j-(VLR_H>>1)) * VLR_Z / VLR_H;
      vy->w = 1.0f;
    }
  }

  /* Build faces and link-faces */
  for(k=j=0; j<VLR_H-1; ++j) {
    for (i=0; i<VLR_W-1; ++i, k += 2) {
      const int tpl = VLR_TPL;

      tlk[k+0].flags = tri[k+0].flags = 0;

      tri[k+0].a = i + j*VLR_W;
      tri[k+0].b = tri[k+0].a + 1;
      tri[k+0].c = tri[k+0].a + VLR_W;

      tlk[k+0].a = (!j) ? VLR_T : k - tpl + 1;  // (k-2*(VLR_W-1)+1);
      tlk[k+0].b = k+1;
      tlk[k+0].c = (!i) ? VLR_T : k-1;

      tlk[k+1].flags = tri[k+1].flags = 0;

      tri[k+1].a = tri[k+0].b;
      tri[k+1].b = tri[k+0].a + VLR_W+1;
      tri[k+1].c = tri[k+0].c;

      tlk[k+1].a = (i<VLR_W-2) ? k+2 : VLR_T;
      tlk[k+1].b = (j<VLR_H-2) ? k+tpl : VLR_T;
      tlk[k+1].c = k;
    }
  }
  /* Verify */
  if (k != VLR_T) {
    SDERROR("Bad number of face [%d != %d]\n", k,VLR_T);
    return -1;
  }

  tri[k].a = tri[k].b = tri[k].c = 0;
  tri[k].flags = 1; /* Setup invisible face */
  tlk[k].a = tlk[k].b = tlk[k].c = i;
  tlk[k].flags = 0;

  /* More verify */
  k = 0;
  for (i=0; i<VLR_TPL; ++i) {
    int * t = &tri[i].a, * l = &tlk[i].a;
    for (j=0; j<3; ++j) {
      if (t[j] >= VLR_V) {
	SDDEBUG("tri %03d, %c out of range : [%d >= %d]\n",i,'A'+j,t[j],VLR_V);
	++k;
      }
      if (l[j] > VLR_T) {
	SDDEBUG("tlk %03d, %c out of range : [%d > %d]\n",i,'A'+j,l[j], VLR_T);
	++k;
      }
    }
  }
  if (k) {
    SDERROR("%d error in mesh generation.\n",k);
    return -1;
  }

  /* Setup matrix*/
  MtxIdentity(fftvlr_proj);
  MtxIdentity(fftvlr_mtx);

  ready = 1;
  return 0;
}

static int sature(const float a)
{
  int v;

  v = (int)a;
  v &= ~(v>>31);
  v |= ((255-v)>>31);
  v &= 255;
  return v;
}

static unsigned int argb4(const float a, const float r,
			  const float g, const float b)
{
  return
    (sature(a) << 24) |
    (sature(r) << 16) |
    (sature(g) << 8)  |
    (sature(b) << 0);
}

static void vlr_update(void)
{
  vtx_t *vy;
  int i;

  /* Scroll FFT towards Z axis */
  for (i=0,vy=v; i<VLR_W*(VLR_H-1); ++i, ++vy) {
      vy->y = vy[VLR_W].y;
  }
  /* Scroll Normals and colors (norm.w) */
  memmove(nrm, nrm + VLR_TPL, sizeof(nrm[0]) * (VLR_T - VLR_TPL));

  fft_fill_bands(bands);

  /* Update first VLR row with FFT data */
  vy = v + (VLR_H-1) * VLR_W;

  if (!db_scaling) {
    const unsigned int mr = 0xFF0;
    static unsigned int max = 16000, min = 0;
    float localmax = max;
    
    /* Compute smoothed min/max */
    min = ((min * mr) + (bands->band[bands->imin].v * (0x1000-mr))) >> 12;

    {
      float f0 = 0.3 * 0.14f / ((float)max), f1;
      for (i=0,f1=f0*0.5; i<VLR_W; ++i, f1 += f0) {
	const float r = 0.3f;
	float w = f1 * (float)bands->band[i].v;
	if (w > localmax) {
	  localmax = w;
	}
	vy[i].y = (float)(w * (1.0f-r)) + (vy[i-VLR_W].y * r);
      }
      if (localmax < 100) localmax = 100;
      max = localmax;
    }
  } else {
    const int db_min = 10;
    const unsigned int mr = 0xfc0;
    static unsigned int min = 0;
    int minv;
    /* Compute smoothed min/max */
    minv = bands->band[bands->imin].v;
    min = ((min * mr) + (minv * (0x1000-mr))) >> 12;
    if (min < db_min) min = db_min;
    minv = int_decibel[min>>3];

    {
      const unsigned int db_sub = min;
      const float f0 = 0.7 * VLR_Y / (float)32768.0;

      for (i=0; i<VLR_W; ++i) {
	const float r = 0.3f;
	int w0 = bands->band[i].v - min;
	float w;
	w0 &= ~(w0>>31);
	w0 = int_decibel[w0>>3];
	w = (float)w0 * f0;
	vy[i].y = (float)(w * (1.0f-r)) + (vy[i-VLR_W].y * r);
      }
    }
  }

  {
    const float la = light_color.w * 255.0f, aa = ambient_color.w * 255.0f;
    const float lr = light_color.x * 255.0f, ar = ambient_color.x * 255.0f;
    const float lg = light_color.y * 255.0f, ag = ambient_color.y * 255.0f;
    const float lb = light_color.z * 255.0f, ab = ambient_color.z * 255.0f;

    /* Face normal calculation  and lightning */
    for (i=0; i<VLR_TPL; i++) {
      const int idx = (VLR_H-2)*VLR_TPL+i;
      vtx_t *n = &nrm[idx];
      float coef;
      FaceNormal(&n->x, v, tri+idx);
      coef = vtx_dot_product(n, &tlight_normal);
      if (coef < 0) {
	coef *= -0.3 * coef;
      }
      *(int*)&n->w = argb4(aa + coef * la, ar + coef * lr,
			   ag + coef * lg, ab + coef * lb);
    }
    *(int*)&nrm[i].w = -1;
  }
}

static int fftvlr_stop(void)
{
  ready = 0;
  fftvlr_free();
  return 0;
}

static int fftvlr_init(any_driver_t *d)
{
  const char * tname = "fftvlr_bordertile";
  border_def_t borderdef;
  ready = 0;
  v = 0;
  tri = 0;
  nrm = 0;
  tlk = 0;

  bands = fft_create_bands(VLR_W, 0);

  fftvlr_texid = texture_get(tname);
  if (fftvlr_texid < 0) {
	fftvlr_texid = texture_dup(texture_get("bordertile"), tname);
  }
  border_get_def(borderdef, 2);
  border_customize(fftvlr_texid, borderdef);

  db_scaling = 0;
  fftvlr_opacity = 0;

  return -(fftvlr_texid < 0);
}

static int fftvlr_shutdown(any_driver_t * d)
{
  ready = 0;
  fftvlr_free();
  if (bands) {
    free(bands);
    bands = 0;
  }
  return 0;
}

static int fftvlr_process(viewport_t * vp, matrix_t projection, int elapsed_ms)
{
  if (ready) {
    matrix_t tmp;
    static float ay;
    
    viewport = *vp;
    MtxCopy(fftvlr_proj, projection);

    MtxIdentity(fftvlr_mtx);
    MtxRotateZ(fftvlr_mtx, 3.14159);
    MtxRotateY(fftvlr_mtx, ay += 0.014*0.15);
    MtxRotateX(fftvlr_mtx, 0.3f);
    fftvlr_mtx[3][2] = 1.0;

    MtxIdentity(tmp);
    MtxRotateZ(tmp, 3.14159);
    MtxRotateY(tmp, -0.33468713*ay);
    MtxRotateX(tmp, 0.4);
    MtxTranspose(tmp);
    MtxVectMult(&tlight_normal, &light_normal, tmp);

    vlr_update();

    return 0;
  }
  return -1;
}

static int render(int render_opaque)
{
  if (!ready) {
    return -1;
  }
  if (render_opaque != fftvlr_opacity) {
    return 0;
  }

  fftvlr_obj.flags = 0
    | DRAW_NO_FILTER
    | (render_opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (fftvlr_texid << DRAW_TEXTURE_BIT);

  return DrawObjectPrelighted(&viewport, fftvlr_mtx, fftvlr_proj,
			      &fftvlr_obj);
}

static int fftvlr_opaque(void)
{
  return render(1);
}

static int fftvlr_transparent(void)
{
  return render(0);
}

static driver_option_t * fftvlr_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

#include "luashell.h"

static int lua_setambient(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i;
  float * ac = (float *) &ambient_color;

  for (i=0; i<4 && i<nparam; i++) {
    ac[i] = lua_tonumber(L, i+1);
    //printf("%d %g\n", i, ac[i]);
  }

  return 0;
}

static int lua_setdirectionnal(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i;
  float * ac = (float *) &light_color;

  for (i=0; i<4 && i<nparam; i++) {
    ac[i] = lua_tonumber(L, i+1);
    //printf("%d %g\n", i, ac[i]);
  }

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

static int lua_setdb(lua_State * L)
{
  return lua_setboolean(L,&db_scaling);
}

static int lua_setopaque(lua_State * L)
{
  return lua_setboolean(L, &fftvlr_opacity);
}

static int lua_setbordertex(lua_State * L)
{
  border_def_t borderdef;

  border_get_def(borderdef,lua_tonumber(L, 1));
  border_customize(fftvlr_texid,borderdef);
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
  border_customize(fftvlr_texid,borderdef);
  return 0;
}  

static luashell_command_description_t fftvlr_commands[] = {
  {
    "fftvlr_setambient", 0,              /* long and short names */
    "print [["
      "fftvlr_setambient(r, g, b, a) : set ambient color to given (r,g,b,a) "
      "values (ranging 0..1)"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setambient      /* function */
  },
  {
    "fftvlr_setdirectionnal", 0,         /* long and short names */
    "print [["
      "fftvlr_setdirectionnal(r, g, b, a) : set directionnal light color to given (r,g,b,a) "
      "values (ranging 0..1)"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setdirectionnal /* function */
  },
  {
    "fftvlr_setbordertex", 0,            /* long and short names */
    "print [["
	"fftvlr_setbordertex([number]) : set border texture type."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbordertex    /* function */
  },

  {
    "fftvlr_custombordertex", 0,            /* long and short names */
    "print [["
	"fftvlr_custombordertex(a1,r1,g1,b1, a2,r2,g2,b2, a3,r3,g3,b3) : "
	"set custom border texture. Each color componant could be set to nil to "
	"keep the current value.\n"
	" a1,r1,g1,b1 : border color\n"
	" a2,r2,g2,b2 : fill color\n"
	" a3,r3,g3,b3 : link color\n"
    "]]",                                   /* usage */
    SHELL_COMMAND_C, lua_custombordertex    /* function */
  },

  {
    "fftvlr_setdb", 0,            /* long and short names */
    "print [["
      "fftvlr_db(bool) : set Db scaling on/off"
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setdb    /* function */
  },
  {
    "fftvlr_setopacity", 0,
    "print [["
    "fftvlr_setopacity([boolean]) : get/set opacity mode. "
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setopaque    /* function */
  },

  {0},                                   /* end of the command list */
};

static vis_driver_t fftvlr_driver =
{

  /* Any driver */
  {
    0,                    /**< Next driver (see any_driver.h)  */
    VIS_DRIVER,           /**< Driver type                     */      
    0x0100,               /**< Driver version                  */
    "fftvlr",             /**< Driver name                     */
    "Vincent Penne, "     /**< Driver authors                  */
    "Benjamin Gerard",
    "Virtual Landscape "  /**< Description                     */
    "Reality based on "
    "Fast Fourier "
    "Transform",
    0,                    /**< DLL handler                     */
    fftvlr_init,          /**< Driver init                     */
    fftvlr_shutdown,      /**< Driver shutdown                 */
    fftvlr_options,       /**< Driver options                  */
    fftvlr_commands,      /**< Lua shell commands              */
  },

  fftvlr_start,           /**< Start                           */
  fftvlr_stop,            /**< Stop                            */
  fftvlr_process,         /**< Process                         */
  fftvlr_opaque,          /**< Opaque render list              */
  fftvlr_transparent,     /**< Transparent render list         */
  
};

EXPORT_DRIVER(fftvlr_driver)
