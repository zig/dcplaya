/** @ingroup dcplaya_vis_driver
 *  @file    hyperpipe.c
 *  @author  benjamin gerard 
 *  @date    2003/01/14
 *
 *  $Id: hyperpipe.c,v 1.13 2003-03-09 01:00:15 ben Exp $
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

#define MIN_RAY 0.05
#define MAX_RAY 0.28
#define W       100u
#define N       8u
#define V       (W*N)
#define T       ((N-2u)*2u + (W-1u) * (2u*N))
#define X       2.0

extern void FaceNormal(float *d, const vtx_t * v, const tri_t *t);

/* Audio data. */
static float spl[W]; /* Normalized sample buffer. */

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

static vtx_t flash_color =    /**< Original color */
  {
    1.5f, 1.5f, 1.5f, -0.4f
  };
static float flash;

/* Ambient color */
static vtx_t ambient = {
  0.0,0.5,0.8,1.3
};

static vtx_t pos;     /**< Object position */
static texid_t texid, texid2;
static fftbands_t * allbands;
static fftbands_t * bands3;

/* Automatic object changes */
static int mode;

#define CHANGE_MODE   1
#define CHANGE_FLASH  2
#define CHANGE_BORDER 4

#define DEFAULT_CHANGE (CHANGE_MODE|CHANGE_BORDER|CHANGE_FLASH)

static int hp_opaque = 0;
static int hp_lighted = 0;

static int change_mode;
static int change_cnt;
static int change_time = 5*1000;

extern short int_decibel[];

static obj_driver_t * light_object;

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
    do { a += 2*MF_PI; } while (a<0);
  } else if (a>=2*MF_PI) {
    do { a -= 2*MF_PI; } while (a>=2*MF_PI);
  }
  return a;
}

/* static void inc_angle_vector(vtx_t *a, const vtx_t * v) */
/* { */
/*   int i; */
/*   for (i=0; i<3; ++i) { */
/*     float * b = (float *)(&a->x) + i; */
/*     *b = inc_angle(*b, ((float *)(&v->x))[i]); */
/*   } */
/* } */



static void set_obj_pointer(void)
{
  hyperpipe_obj.vtx = v;
  hyperpipe_obj.nvx = nrm;
  hyperpipe_obj.tri = tri;
  hyperpipe_obj.tlk = tlk;
}

static void hyperpipe_free(void)
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

static int hyperpipe_alloc(void)
{
  hyperpipe_free(); /* Safety net */
  v   = (vtx_t *)malloc(sizeof(vtx_t) * hyperpipe_obj.nbv);
  nrm = (vtx_t *)malloc(sizeof(vtx_t) * (hyperpipe_obj.nbf+1));
  tri = (tri_t *)malloc(sizeof(tri_t) * (hyperpipe_obj.nbf+1));
  tlk = (tlk_t *)malloc(sizeof(tlk_t) * (hyperpipe_obj.nbf+1));
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
  for(v=ring, ve=v+N, a=0, stp = 2*MF_PI/N; v<ve; ++v, a+=stp) {
    v->x = Cos(a);
    v->y = Sin(a);
    v->z = 0;
    v->w = 1;
  }
}

static void build_normals(void)
{
  int i;
  const int n = hyperpipe_obj.nbf;
  for (i=0; i<n; ++i) {
    FaceNormal((float *)(nrm+i),v,tri+i);
  }
  vtx_identity(nrm+i);
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

      tlk[i].a = !i ? T : i;
      tlk[i].b = T;
      tlk[i].c = (i == n-3) ? T : i;
    }
  } else {
    int m = n-2;
    base = base - n;
    for (i=0; i<m; ++i) {
      tlk[i].flags = tri[i].flags = 0;

      tri[i].a = base;
      tri[i].c = base+i+1;
      tri[i].b = base+i+2;

      tlk[i].c = i;
      tlk[i].a = i;
      tlk[i].b = T;
    }
  }
  return i;
}

static int stop(void)
{
  ready = 0;
  hyperpipe_free();
  if (light_object) {
    driver_dereference(&light_object->common);
    light_object = 0;
  }
  return 0;
}

static int start(void)
{
  int i, j, k, w;
  vtx_t *vy;

  memset(spl,0,sizeof(spl));

  ready = 0;
  build_ring();

  /* Alloc buffers */
  if (hyperpipe_alloc()) {
    return -1;
  }

  /* Build vertrices */
  for(j=0, vy=v; j<W; ++j) {
    for (i=0; i<N; ++i, ++vy){
      vy->x = ((float)j-(float)W*0.5) * X / W;
      vy->y = ring[i].x * MAX_RAY;
      vy->z = ring[i].y * MAX_RAY;
      vy->w = 1.0f;
    }
  }

  /* First close faces */
  k = tesselate(tri,tlk,0,N);

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
    }
  }

  k += tesselate(tri+k,tlk+k,V,N);

  if (k != T) {
    printf("[hyperpipe] : bad number of generated face [%d != %d]\n",k,T);
    hyperpipe_free();
    return -1;
  }

  tri[T] = tri[0];
  tri[T].flags = 1;
  tlk[T].a = tlk[T].b = tlk[T].c = T;
  tlk[T].flags = 0;

  build_normals();

  if (obj3d_verify(&hyperpipe_obj)) {
    printf("[hyperpipe] : verify failed\n");
    hyperpipe_free();
    return -1;
  }

  /* Setup matrix*/
  MtxIdentity(proj);
  MtxIdentity(mtx);

  ready = 1;
  return 0;
}


static vtx_t light_orgvtx = { 1,0,0,1 };
static vtx_t light_vtx;
static vtx_t light_angle;
static vtx_t light_speed = { 0.5, 0.431, 0.1237, 0 };
static matrix_t light_mtx;

static void anim_light(const float seconds)
{
  vtx_t speed;

  speed.x = light_speed.x * seconds;
  speed.y = light_speed.y * seconds;
  speed.z = light_speed.z * seconds;
  speed.w = 0;

  vtx_inc_angle(&light_angle, &speed);

  MtxIdentity(light_mtx);
  MtxRotateX(light_mtx, light_angle.x);
  MtxRotateY(light_mtx, light_angle.y);
  MtxRotateZ(light_mtx, light_angle.z);
  MtxVectMult(&light_vtx.x, &light_orgvtx.x, light_mtx);
}

static void get_pcm(void)
{
  int i;
  short spl2[W];
  const float scale = 1.0f/32768.0f;
  const float s0 = 0.75;
  const float s1 = 1.0 - s0;

  /* PCM */
  fft_fill_pcm(spl2,W);
  for (i=0; i<W; ++i) {
    const float a = (float)spl2[i] * scale;
    spl[i] = (spl[i] * s0) + (a * s1);
  }
}

static void anim_occilo(const float seconds)
{
  int i;
  vtx_t * vy;
  const float s = (MAX_RAY-MIN_RAY);

  get_pcm();

  for (i=0, vy=v; i<W; ++i) {
    int j;
    const float v = spl[i];
    float s1,s2;
    s1 = s2 = MIN_RAY;
    if (v<0) {
      s2 += sqrtf(-v) * s;
    } else {
      s1 += sqrtf(v) * s;
    }

    for (j=0; j<(N>>1); ++j, ++vy) {
      vy->y = ring[j].x * s1;
      vy->z = ring[j].y * s1;
    }
    for (; j<N; ++j, ++vy) {
      vy->y = ring[j].x * s2;
      vy->z = ring[j].y * s2;
    }
  }
}

static void anim_occilo2(const float seconds)
{
  int i;
  vtx_t * vy;
  const float s = (MAX_RAY-MIN_RAY);
  static float az=0;
  float az2;
  const float saz  = 7 * seconds;
  const float saz2 = 5 * seconds;

  az2 = az;
  az = inc_angle(az,saz);
  
  get_pcm();

  for (i=0, vy=v; i<W; ++i) {
    int j;
    const float v = spl[i], w = (v < 0 ? -sqrtf(-v) : sqrtf(v));
    const float 
      w2 = fabs(w) * s,
      s1 = MIN_RAY + w2,
      s3 = MIN_RAY + w2 * 0.5,
      s2 = s1 * ((v<0) ? -0.5 : 0.5);

    const float sy = Cos(az2) * s * 0.3;
    az2 = inc_angle(az2,saz2);

    for (j=0; j<N; ++j, ++vy) {
      vy->y = ring[j].x * s3 + sy;
      vy->z = ring[j].y * s1 + s2;
    }
  }
}


static void anim_fft(const float seconds)
{
  int i;
  vtx_t * vy;
  const float s = (MAX_RAY-MIN_RAY);

  if (!allbands) {
    for (i=0, vy=v; i<W; ++i) {
      int j;
      for (j=0; j<N; ++j, ++vy) {
	vy->y = ring[j].x * MIN_RAY;
	vy->z = ring[j].y * MIN_RAY;
      }
    }
  } else {
    fft_fill_bands(allbands);
    for (i=0, vy=v; i<W; ++i) {
      int j;
      int d = allbands->band[i].v - 150;
      float w;
      d &= ~(d>>31);
      w = int_decibel[d>>3] * (1.0/32768.0);
      {
	const float v = MIN_RAY + w * s;
	for (j=0; j<N; ++j, ++vy) {
	  vy->y = ring[j].x * v;
	  vy->z = ring[j].y * v;
	}
      }
    }
  }
}

static struct {
  float v,w;
  float avg,avg2;
  float max, max2;
  float thre;
  float ts;
  int tacu;
  float tap[W];
} b[2];

// return bit:0-2 beat on band 0-2 bit:8 beat on all band.
static int analyse()
{
  int j,r;

  if (bands3) {
    float q;

    fft_fill_bands(bands3);
   
    for (j=0; j<2; ++j) {
      const float avgf  = 0.988514020;
      const float avgf2 = 0.93303299;
      const float v = int_decibel[bands3->band[j].v>>3] * (1.0/32768.0);
      b[j].v = b[j].v * 0.7 + v * 0.3;

      b[j].avg = b[j].avg * avgf + b[j].v * (1.0-avgf);
      b[j].avg2 = b[j].avg2 * avgf2 + b[j].v * (1.0-avgf2);
      b[j].max = max(b[j].max,0.1);
      b[j].max = max(b[j].max,b[j].avg);

      b[j].ts = bound(b[j].ts, 0.1, 0.98);
      b[j].thre = (b[j].max * b[j].ts) + (b[j].avg * (1.0-b[j].ts));

      if (b[j].v > b[j].thre) {
	if (b[j].tacu <= 0) {
	  b[j].tacu = 1;
	  b[j].max2 = b[j].v;
	} else {
	  ++b[j].tacu;
	  if (b[j].tacu > 10) {
	    if (b[j].max2 > b[j].max) {
	      b[j].max = b[j].max2;
	    } else {
	      b[j].ts *= 1.01;
	    }
	  }
	  b[j].max2 = max(b[j].max2,b[j].v);
	}
      } else {
	if (b[j].tacu > 0) {
	  b[j].max = b[j].max2;
	  b[j].tacu = 0;
	  b[j].max2 = b[j].v;
	}
	b[j].max2 = max(b[j].max2,b[j].v);

	--b[j].tacu;
	if (b[j].tacu < -10) {
	  b[j].max = b[j].max * avgf + b[j].max2 * (1.0f-avgf);
	  b[j].ts *= 0.99f;
	}
      }
      q = 0;
      if (b[j].tacu == 3) {
	q = 1;
      }
      b[j].w = q;
    }
  } else {
    for (j=0; j<2; ++j) {
      b[j].w = 0;
    }
  }

  r = (b[0].w>0) | ((b[1].w>0)<<1);
  if (r) {
    r |= 0x100 
      & -(b[0].tacu>=3 && b[0].tacu<20)
      & -(b[1].tacu>=3 && b[1].tacu<20);
  }

  return r;
}

static void anim_band(const float seconds)
{
  int j;
  vtx_t * vy;

  if (!change_mode) {
    /* Process analyse if no change mode has been selected. Else it
       has already be done by process().
    */
    analyse();
  }

  /* Filter */
  for (j=0; j<2; ++j) {
    float * dtap=b[j].tap;
    float s0,s1/*,s2*/;
    float toto = b[j].w;
    int i;

    if (toto > 0) {
      dtap[0] = toto;
    } else {
      dtap[0] *= 0.4;
    }

    s1 = dtap[0];
    for (i=1; i<W-1; ++i) {
      s0 = s1;
      s1 = dtap[i];
/*       dtap[i] = s0 * 0.91 + s1 * 0.09; */
      dtap[i] = s0 * 0.81 + s1 * 0.192;
    }
  }

  /* Convert */
  {
    int i;
    float * t0 = b[0].tap;
    float * t1 = b[1].tap;
    
    for (i=0, vy=v; i<W; ++i) {
      int j;
      const float s0 = MIN_RAY + t0[i] * (MAX_RAY-MIN_RAY);
      const float s1 = MIN_RAY + t1[W-1-i] * (MAX_RAY-MIN_RAY);
      float s,a,b;
      if (s0 < s1) {
	a = 0.3f;
	b = 1.0;
      } else {
	b = 0.3f;
	a = 1.0;
      }
      s = s0 * a + s1 * b;
      for (j=0; j<N; ++j, ++vy) {
	vy->y = ring[j].x * s;
	vy->z = ring[j].y * s;
      }
    }
  }

}

static void (*fct[])(const float) = {
  anim_occilo, anim_occilo2, anim_fft, anim_band
};

static const unsigned int nfct = sizeof(fct) / sizeof(*fct);

static void anim_object(const float seconds)
{
  static float ay, az, paz = 0.01f;

  fct[mode % nfct](seconds);

  /* Build local matrix */
  MtxIdentity(mtx);
  az = Sin(paz += 0.0433f) * 0.22f;
  MtxRotateX(mtx, MF_PI/2);
  MtxRotateY(mtx, ay += 0.014*0.52);
  MtxRotateX(mtx, 0.4f+ az);
  if (ay > 2*MF_PI) {ay -= 2*MF_PI;}

  mtx[3][0] = pos.x;
  mtx[3][1] = pos.y;
  mtx[3][2] = pos.z;

  build_normals();
}



static int anim(const float seconds)
{
  anim_object(seconds);
  anim_light(seconds);

  return 0;
}

static int process(viewport_t * vp, matrix_t projection, int elapsed_ms)
{
  const float seconds = elapsed_ms * (1/1000.0f);
  static int mode_latch = 0, mode_acu;

  if (!ready) {
    return -1;
  }

  if (change_mode) {
    int result = analyse();

    if (!(result & 3)) {
      if ((change_cnt += elapsed_ms) >= change_time) {
	change_cnt -= change_time;
	result |= 3;
      }
    } else {
      change_cnt = 0;
    }

    if (mode_latch) --mode_latch;

    if (result & 1) {
      mode_acu = (mode_acu+1) & 7;
    }
    if ( !mode_latch && !mode_acu && (change_mode & CHANGE_MODE)) {
      mode_latch = 30;
      mode = rand() % nfct;
    }

    if ((result & 2) && (change_mode & CHANGE_BORDER)) {
      border_def_t borderdef;
      border_get_def(borderdef,(unsigned short)rand());
      border_customize(texid, borderdef);
    }

    if ((result & 0x100)  && (change_mode & CHANGE_FLASH) ) {
      flash = 1.0f;
    }

    if (flash > MF_EPSYLON) {
      const float o = 1.0f - flash;
      color.x = base_color.x * o + flash_color.x * flash;
      color.y = base_color.y * o + flash_color.y * flash;
      color.z = base_color.z * o + flash_color.z * flash;
      color.w = base_color.w * o + flash_color.w * flash;
      flash *= 0.94f;
    } else {
      color = base_color;
      flash = 0;
    }

  }

  if (!light_object) {
    light_object = (obj_driver_t *)driver_list_search(&obj_drivers,"spaceship1");
  }

  pos.z = X * 0.5 + 1.00;
  viewport = *vp;
  MtxCopy(proj, projection);
  anim(seconds);

  return 0;
}

static int render(int opaque)
{
  if (!ready) {
    return -1;
  }
  
  if (light_object && hp_lighted && texid2>0) {
    vtx_t di;

    float lx,ly,lz;
    lx = light_vtx.x + pos.x;
    ly = light_vtx.y + pos.y;
    lz = light_vtx.z + pos.z;

    di.x = color.x; di.y = color.y; di.z = color.z; di.w = 0.9;
    MtxLookAt(light_mtx, pos.x-lx, pos.y-ly, pos.z-lz);
    MtxTranspose3x3(light_mtx);
    MtxScale(light_mtx,0.43);
    MtxTranslate(light_mtx,lx,ly,lz);
    
    light_object->obj.flags = 0
      | DRAW_NO_FILTER
      | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
      | (texid2 << DRAW_TEXTURE_BIT);

    DrawObjectSingleColor(&viewport, light_mtx, proj,
			   &light_object->obj, &di);
  }

  if (texid>0) {
    hyperpipe_obj.flags = 0
      | DRAW_NO_FILTER
      | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
      | (texid << DRAW_TEXTURE_BIT);

    if (hp_lighted) {
      vtx_t light_dir;
      vtx_neg2(&light_dir, &light_vtx);
      DrawObjectLighted(&viewport, mtx, proj,
			&hyperpipe_obj,
			&ambient, &light_dir, &color);
    } else {
      DrawObjectFrontLighted(&viewport, mtx, proj,
			     &hyperpipe_obj,
			     &ambient, &color);
    }
  }
  return 0;
}

static int opaque_render(void)
{
  if (!ready) {
    return -1;
  }

  if (!hp_opaque) {
    return 0;
  }
  return render(1);
}

static int transparent_render(void)
{

  if (!ready) {
    return -1;
  }

  if (hp_opaque) {
    return 0;
  }
  return render(0);
}


static int init(any_driver_t *d)
{
  const char * tname = "hp_bordertile";
  const char * tname2 = "hp_light";
  border_def_t borderdef;

  static fftband_limit_t limits[] = {
    {0, 250},
    {1000, 3500},
  };

  ready = 0;
  allbands = 0;
  bands3 = 0;
  change_mode = DEFAULT_CHANGE;
  change_cnt = 0;
  mode = 1 % nfct;
  flash = 0;

  memset(b,sizeof(b),0);

  v = 0;
  nrm = 0;
  tri = 0;
  tlk = 0;

  texid = texture_get(tname);
  if (texid < 0) {
    texid  = texture_dup(texture_get("bordertile"), tname);
  }
  texid2 = texture_get(tname2);
  if (texid2 < 0) {
    texid2 = texture_dup(texture_get("bordertile"), tname2);
  }

  border_get_def(borderdef, 1);
  border_customize(texid, borderdef);
  border_customize(texid2, borderdef);
  allbands = fft_create_bands(W,0);
  bands3 = fft_create_bands(2,limits);

  return -(texid < 0 || texid2 < 0);
}

static int shutdown(any_driver_t *d)
{
  stop();
  if (allbands) {
    free(allbands);
    allbands = 0;
  }
  if (bands3) {
    free(bands3);
    bands3 = 0;
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

static int lua_setflashcolor(lua_State * L)
{
  lua_setcolor(L,(float *) &flash_color);
  return 0;
}

static int lua_setbasecolor(lua_State * L)
{
  lua_setcolor(L,(float *) &base_color);
  return 0;
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
  return lua_gettop(L);
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

static int lua_setopaque(lua_State * L)
{
  return lua_setboolean(L, &hp_opaque);
}

static int lua_setlighting(lua_State * L)
{
  return lua_setboolean(L, &hp_lighted);
}


static int lua_setmode(lua_State * L)
{
  int omode = mode;
  int n = lua_gettop(L);
  
  if (n>=1 && lua_type(L,1) != LUA_TNIL) {
    if (nfct>0) {
      mode = (unsigned int)lua_tonumber(L,1) % nfct;
    } else {
      mode = 0;
    }
  }
  lua_settop(L,0);
  lua_pushnumber(L, omode);
  return 1;
}


static luashell_command_description_t commands[] = {
  {
    "hyperpipe_setambient", "hp_ambient",
    "print [["
    "hyperpipe_setambient(a, r, g, b) : set ambient color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setambient      /* function */
  },
  {
    "hyperpipe_setdiffuse", "hp_diffuse",
    "print [["
    "hyperpipe_setdiffuse(a, r, g, b) : set object base color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbasecolor /* function */
  },
  {
    "hyperpipe_setflashcolor", "hp_flashcolor",
    "print [["
    "hyperpipe_setflashcolor(a, r, g, b) : set flash color."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setflashcolor /* function */
  },

  {
    "hyperpipe_setbordertex", "hp_border",
    "print [["
    "hyperpipe_setbordertex(num) : set border texture type."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setbordertex    /* function */
  },
  {
    "hyperpipe_custombordertex", "hp_customborder",
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
    "hyperpipe_setchange", "hp_change",            /* long and short names */
    "print [["
    "hyperpipe_setchange(type [, time]) : Set object change properties. "
    "type bit0:random-mode bit1:active-flash bit2:auto-border."
    "Return old values."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setchange    /* function */
  },
  {
    "hyperpipe_setmode", "hp_mode",            /* long and short names */
    "print [["
    "hyperpipe_setmode([mode]) : Set display mode. "
    "Return old values."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setmode    /* function */
  },
  {
    "hyperpipe_setopacity", "hp_opacity", /* long and short names */
    "print [["
    "hyperpipe_setopacity([boolean]) : get/set opacity mode. "
    "Return old state."
    "]]",                                /* usage */
    SHELL_COMMAND_C, lua_setopaque    /* function */
  },
  {
    "hyperpipe_setlighting", "hp_light",  /* long and short names */
    "print [["
    "hp_setlighting([boolean]) : Set/Get lighting process."
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
