
/**
 * @file      dreamcast68.c
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/02/08
 * @brief     sc68 for dreamcast - main for kos 1.1.x
 * @version   $Id: dreamcast68.c,v 1.1.1.1 2002-08-26 14:15:03 ben Exp $
 */

//#define RELEASE
//#define SKIP_INTRO



#define CLIP(a, min, max) ((a)<(min)? (min) : ((a)>(max)? (max) : (a)))


#define SCREEN_W 640
#define SCREEN_H 480



/* generated config include */
#include "config.h"

#include <dc/fmath.h>
#include <stdio.h>

#include "sndstream.h"
#include "songmenu.h"
#include "gp.h"

/* dreamcast68 includes */
#include "file_wrapper.h"
#include "matrix.h"
#include "obj3d.h"
#include "controler.h"
#include "info.h"
#include "option.h"
#include "remanens.h"

#include "playa.h"
#include "vupeek.h"
#include "driver_list.h"
#include "inp_driver.h"
#include "playa.h"
#include "plugin.h"
#include "plug_vlr.h"
#include "lef.h"
#include "fft.h"

float fade68;

uint32 frame_counter68 = 0;
int help_close_frames = 0;
controler_state_t controler68;

/* sc68 mutex */
spinlock_t app68mutex;
static int volume;

static obj_t * curlogo;

static volatile kthread_t * play_thd;

#define SET_PLAYER_STATUS(A,B) (play_status = ((A)&0xFFFF) | ((B)<<16)) 
#define PLAYER_STOPPED  0
#define PLAYER_EXITING  1
#define PLAYER_PLAYING  2
#define PLAYER_PAUSED   3
#define PLAYER_TRACKING 4
//volatile unsigned int play_status = 0;

extern void vmu_lcd_update(int *b, int n, int cnt);
extern int vmu68_init(void);
extern int vmu_lcd_title();

static int warning_splash(void);
extern void warning_render();


static vtx_t light_normal = { 
  0.8,
  0.4,
  0.5
};

static vtx_t tlight_normal;
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


#define ANIM_CONT   0
#define ANIM_CYCLE  1
#define ANIM_END    2

typedef struct
{
  float x,y,z;
  float ax,ay,az;
  float zoom;
  float flash;
  obj_t *o;
} anim_t;

static anim_t animdata;

typedef int (*anim_f)(anim_t *, unsigned int frames);


static float fade_step;

static void fade(unsigned int elapsed_frames)
{
  const float step = fade_step * (float)elapsed_frames;
  fade68 += step;
  if (fade68 < 0.0f) {
    fade68 = 0;
  } else if (fade68 > 1.0f) {
    fade68 = 1.0f;
  }
}

static void pipo_poly(int mode)
{
  poly_hdr_t poly;
  vertex_oc_t vert;
  int i;
	
  ta_poly_hdr_col(&poly, !mode ? TA_OPAQUE : TA_TRANSLUCENT);
  ta_commit_poly_hdr(&poly);

  vert.a = vert.r = vert.g = vert.b = 0;
	
  for (i=0;i<3;++i) {
    vert.flags = (i!=2) ? TA_VERTEX_NORMAL : TA_VERTEX_EOL;
    vert.x = (float)(!!i);
    vert.y = (float)(i>1);
    vert.z = 1.0f;
    ta_commit_vertex(&vert, sizeof(vert));
  }
}

void lockapp(void)
{
  spinlock_lock(&app68mutex);
}

void unlockapp(void)
{
  spinlock_unlock(&app68mutex);
}

#ifndef RELEASE
#define VCOLOR my_vid_border_color
static void my_vid_border_color(int r, int g, int b)
{
  vid_border_color(r,g,b);
}
#else
# define my_vid_border_color(R,G,B)
#endif

static void change_timeinfo(int lock)
{
}

static void change_diskinfo(int lock)
{
}


/* Sound first init : load arm code and setup streaming */
static int sound_init(void)
{
  playa_init();
  return 0;
}

int dreamcast68_isplaying(void)
{
  int res;

  res = playa_isplaying();
  return res;
}

int dreamcast68_changetrack(int new_track)
{
  return 0;
}

int dreamcast68_stop(int flush)
{
  playa_stop(flush);
  dbglog(DBG_DEBUG, "** " __FUNCTION__  " : STOPPED\n");
  return 0;
}

int dreamcast68_loaddisk(char *fn, int immediat)
{
  /* No filename : stop, no error */
  if (!fn) {
    dreamcast68_stop(immediat);
    return 0;
  }
  return playa_start(fn,immediat);
}


static void sature(float *a, const float min, const float max)
{
  float f = *a;

  if (f<min) f = min;
  else if (f>max) f=max;
  *a = f;
}

typedef struct {
  float u;
  float v;
} uv_t;

static uv_t uvlinks[8][4] =
{
  /* 0 cba */  { {0.0f,0.5f},   {0.0f,0.0f},    {0.5f,0.0f},   {0,0} },
  /* 1 cbA */  { {0.5f,0.0f},   {0.5f,0.5f},    {1.0f,0.0f},   {0,0} },
  /* 2 cBa */  { {1.0f,0.1f},   {0.5f,0.5f},    {0.5f,0.1f},   {0,0} },
  /* 3 cBA */  { {0.0f,1.0f},   {0.0f,0.5f},    {0.5f,0.5f},   {0,0} },
  /* 4 Cba */  { {0.5f,0.1f},   {1.0f,0.1f},    {0.5f,0.5f},   {0,0} },
  /* 5 CbA */  { {0.0f,0.5f},   {0.5f,0.5f},    {0.0f,1.0f},   {0,0} },
  /* 6 CBa */  { {0.5f,0.5f},   {0.0f,1.0f},    {0.0f,0.5f},   {0,0} },
  /* 7 CBA */  { {0.5f,1.0f},   {0.5f,0.5f},    {1.0f,0.5f},   {0,0} },
};

void DrawObject(obj_t * o, matrix_t local, matrix_t proj,
		const float z, const float a, const float light)
{
  static vtx_t transform[4096];
  matrix_t m;
  //  int i;
  //  int cnt = 0;

  poly_hdr_t poly;
  //  vertex_ot_t vert[3];

  typedef struct s_v_t {
    int flags;
    float x,y,z,u,v;
    unsigned int col, addcol;
  } v_t;

  volatile v_t * const hw = (volatile v_t *)(0xe0<<24);

  float ca, cb , cr, cg;

  float aa, ar, ag, ab;
  float la, lr, lg, lb;

  if (!o) {
    static int i=0;
    if (!i) {
      dbglog(DBG_ERROR,"$$ " __FUNCTION__ 
	     " : <null> object\n");
      i=1;
    }
    return;
  }

  if (o->nbv > sizeof(transform)/sizeof(*transform)) {
    dbglog(DBG_ERROR,"$$ " __FUNCTION__ 
	   " : Too many vertrices (%d) in object [%s]\n",
	   o->nbv, o->name);
    *(volatile int *)1 = 0xdeaddead;
  }

  ca = a;
  cb = 1.0f /*+ light * light * .5f*/;
  cg = 1.0f + light;
  cr = 1.0f;
/*
  ca = 1.0f;
  cb = cg = cr =  1.0f;
*/

  ca *= 255.0f;
  cr *= 255.0f;
  cg *= 255.0f;
  cb *= 255.0f;


  ar = cr * ambient_color.x;
  ag = cg * ambient_color.y;
  ab = cb * ambient_color.z;
  aa = ca * ambient_color.w;

  lr = cr * light_color.x;
  lg = cg * light_color.y;
  lb = cb * light_color.z;
  la = ca * light_color.w;




#if 0
  for (i=0; i<3; ++i) {
    vert[i].flags = TA_VERTEX_NORMAL;
    vert[i].dummy1 = vert[i].dummy2 = 0;
    // Yellow cool
    /*
      vert[i].b = 0.0f;
      vert[i].g = 0.89f * 0.8f;
      vert[i].r = 1.0f  * 0.8f;
    */
    if (light >= 0) {
      vert[i].a = a;
      vert[i].b = 0.0f /*+ light * light * .5f*/;
      vert[i].g = 0.5f + light;
      vert[i].r = 1.0f;
    } else {
      float l = -light;
      sature(&l,0.3f,1.0f);
      vert[i].a = a;
      vert[i].b = 1.0f * l;
      vert[i].g = 0.5f;
      vert[i].r = 1.0f;
    }
    sature(&vert[i].a, 0.3f, 1.0f);
    sature(&vert[i].b, 0.0f, 1.0f);
    sature(&vert[i].g, 0.0f, 1.0f);
    sature(&vert[i].r, 0.0f, 1.0f);

    
    /* // Red cool
       vert[i].b = 0.0f;
       vert[i].g = 0.39f * 0.8f;
       vert[i].r = 1.0f  * 0.8f;
    */

    /* // Red cool
       vert[i].b = 0.0f;
       vert[i].g = 0.39f * 0.8f;
       vert[i].r = 1.0f  * 0.8f;
    */
    
    vert[i].z = z;
    
    
    vert[i].oa = vert[i].or = vert[i].og = vert[i].ob = 0.0f;
  }
  vert[2].flags = TA_VERTEX_EOL;
#endif

  memcpy(m,local,sizeof(m));


  VCOLOR(255,0,0);

  /* Rotate vtx */
  {
/*     const float xscale = 160.0f; */
/*     const float yscale = xscale; */
/*     const float zscale = 500.0f; */
    vtx_t * v = o->vtx;
    int     n = o->nbv;
    vtx_t * d = transform;

/*    const float m00 = m[0][0] * xscale; 
    const float m01 = m[0][1] * xscale; 
    const float m02 = m[0][2] * xscale; 
    const float m03 = m[0][3] * xscale + 320.0f;; 

    const float m10 = m[1][0] * yscale; 
    const float m11 = m[1][1] * yscale; 
    const float m12 = m[1][2] * yscale; 
    const float m13 = m[1][3] * yscale + 240.0f; 

    const float m20 = m[2][0] * zscale; 
    const float m21 = m[2][1] * zscale; 
    const float m22 = m[2][2] * zscale; 
    const float m23 = m[2][3] * zscale + zscale*2.0f;*/

    const float m00 = m[0][0]; 
    const float m01 = m[0][1]; 
    const float m02 = m[0][2]; 
    const float m03 = m[0][3]; 

    const float m10 = m[1][0]; 
    const float m11 = m[1][1]; 
    const float m12 = m[1][2]; 
    const float m13 = m[1][3]; 

    const float m20 = m[2][0]; 
    const float m21 = m[2][1]; 
    const float m22 = m[2][2]; 
    const float m23 = m[2][3];

    const float m30 = m[3][0]; 
    const float m31 = m[3][1]; 
    const float m32 = m[3][2]; 
    const float m33 = m[3][3];


/*    mat_load(&m[0][0]);
    td_reset();
    mat_apply(m);

    td_transform_vectors(v, d, n, 4*4);
*/

    do { 
      const float x = v->x, y = v->y, z = v->z; 
      const float w = (x * m03 + y * m13 + z * m23) + m33; 
      d->z = 1/((x * m02 + y * m12 + z * m22) + m32); 
      d->x = SCREEN_W * 0.5f *
	(1.0f + ((x * m00 + y * m10 + z * m20) + m30) / w); 
      d->y = SCREEN_H * 0.5f *
	(1.0f + ((x * m01 + y * m11 + z * m21) + m31) / w); 
      ++d; 
      ++v; 
    } while (--n);
  }

  VCOLOR(0,255,0);

  /* Flag visible face */
  {
    tri_t * f = o->tri;
    int     n = o->nbf;
    do {
      const vtx_t *t0 = transform + f->a;
      const vtx_t *t1 = transform + f->b;
      const vtx_t *t2 = transform + f->c;

      const float a = t1->x - t0->x;
      const float b = t1->y - t0->y;
      const float c = t2->x - t0->x;
      const float d = t2->y - t0->y;

      const float sens =  a * d - b * c;

      f->flags = (sens < 0);

      //      f->flags = (rand()&3)==3;

      //      cnt += f->flags;
      ++f;
    } while(--n);
  }

  VCOLOR(0,255,255);

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;
    int     a, r, g, b;
    
    ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 64, 64, bordertex,
		  TA_NO_FILTER);

    poly.flags1 &= ~(3<<4);
    ta_commit_poly_hdr(&poly);

    hw->addcol = 0;
    /*    hw->addcol = -1;
	  hw->col = -1;*/
 
    if (!l) {
      // $$$ !
      return;
    }
    for(n=o->nbf ;n--; ++f, ++l, ++nrm)  {
      int lflags = 0;
      uv_t * uvl;
      float coef;
      int   * col;

      if (f->flags) {
	continue;
      }

      col = (int *) &nrm->w;

      if (n < 2*64) {
	coef = nrm->x * tlight_normal.x + 
	  nrm->y * tlight_normal.y + 
	  nrm->z * tlight_normal.z;
	
	if (coef < 0)
	  coef = 0;  //-0.3f * coef;
	
	r = ar + coef * lr;
	g = ag + coef * lg;
	b = ab + coef * lb;
	a = aa + coef * la;
	
	//b= g = r = 255 * coef;
	
	//ca = 0xFF;
	
#define MIN255(a) ( (a | ((255-a)>>31)) & 255 )
	
	*col =
	  (MIN255(a)  << 24) +
	  (MIN255(r)  << 16) +
	  (MIN255(g)  << 8) +
	  (MIN255(b)  << 0);
      }
      
      hw->col = *col;
      

      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      // Strong link :
      //lflags |= l->flags & 7;
	
      // $$$
/*       if (lflags != test && test != 8) { */
/* 	lflags = 0; */
/*       } */

      uvl = uvlinks[lflags];

      hw->flags = TA_VERTEX_NORMAL;
      hw->x = transform[f->a].x;
      hw->y = transform[f->a].y;
      hw->z = transform[f->a].z;
      hw->u = uvl[0].u;
      hw->v = uvl[0].v;
      ta_commit32_nocopy();

      //	hw->flags = TA_VERTEX;
      hw->x = transform[f->b].x;
      hw->y = transform[f->b].y;
      hw->z = transform[f->b].z;
      hw->u = uvl[1].u;
      hw->v = uvl[1].v;
      ta_commit32_nocopy();

      hw->flags = TA_VERTEX_EOL;
      hw->x = transform[f->c].x;
      hw->y = transform[f->c].y;
      hw->z = transform[f->c].z;
      hw->u = uvl[2].u;
      hw->v = uvl[2].v;
      ta_commit32_nocopy();
      
    }
  }
  
  // end:
  VCOLOR(0,0,0);
}


static void render_fx(int age_per_trace, int trace_age)
{
  const float amax = 0.5f, amin = 0.1f;
  const unsigned int this_frame = frame_counter68;
  
  matrix_t proj;
  unsigned int tframe;
  int nb, wanted_age;
  float sa =  (amax - amin) / trace_age;
  float z = 80.0f;

  tframe = this_frame - trace_age + 1;
  if (tframe > this_frame) {
    tframe = 0;
  }
  remanens_remove_old(tframe);
  
  wanted_age = 0; 
  nb = remanens_nb();
  while (--nb >= 0) {
    remanens_t *r;
    int age;
     
    r = remanens_get(nb);
    if (!r) continue; /* safety net */
     
    age = this_frame - r->frame;
    while (age > wanted_age) {
      wanted_age += age_per_trace;
    }
    
    if (age == wanted_age) {
      float a = amax - (float)age * sa;
      DrawObject(r->o, r->mtx, proj, z, a * fade68, 0);
    }
  }
}


static int render_anim_object(uint32 elapsed_frames, anim_f anim, anim_t *data)
{
  /* Trace constants */
  const int age_per_trace = 5;
  const int max_trace = 5;
  int trace_age;

  /* Frame adapt system */
  static uint32 pass_acu = 0;
  static uint32 elapsed_acu = 0;
  static int ntrace = max_trace;
  
  int err;

  /* Adapt number of trace acording to frame rate */    
  if (pass_acu == 128) {
    if (elapsed_acu > 130) {
      if (ntrace > 0) {
        --ntrace;
      }
    } else if (elapsed_acu == pass_acu && ntrace < max_trace) {
      ++ntrace;
    }
    pass_acu = elapsed_acu = 0;
  }
  elapsed_acu += elapsed_frames;
  pass_acu++;

  /* Anim & push to renanens */
  err = anim(data, elapsed_frames);
  
  if (data->o) {
    matrix_t m;
    MtxIdentity(m);
    MtxRotateZ(m, data->az);
    MtxRotateX(m, data->ax);
    MtxRotateY(m, data->ay);
    MtxScale(m,data->zoom);
    m[3][0] = data->x;
    m[3][1] = data->y;
    m[3][2] = data->z;
    remanens_push(data->o, m, frame_counter68);
  }

  if (ntrace > 0) {
    trace_age = ntrace * age_per_trace;
    render_fx(age_per_trace, trace_age);
  }
  return err;
}

static int render_simple_anim_object(uint32 elapsed_frames, anim_f anim,
				     anim_t *data)
{
  int err;
  matrix_t m;

  /* Anim & push to renanens */
  err = anim(data, elapsed_frames);
  
  if (data->o) {
    MtxIdentity(m);
    MtxRotateZ(m, data->az);
    MtxRotateX(m, data->ax);
    MtxRotateY(m, data->ay);
    MtxVectMult(&tlight_normal.x, &light_normal.x, m);
    MtxScale(m,data->zoom*8*4);
    m[3][0] = data->x;
    m[3][1] = data->y;
    m[3][2] = data->z;

    matrix_t tmp;
    MtxProjection(tmp, 70*2.0*3.14159/360,
		  0.01, (float)SCREEN_W/SCREEN_H,
		  1000);
    MtxMult(m, tmp);
  }


  if(0) {
    DrawObject(data->o, m, 0, 80.0f, 1.0f, data->flash);
  } else {
    static int init = 0;

    if (!init) {
      vlr_init();
      init = 1;
    }
    MtxIdentity(m);
    MtxRotateZ(m, 3.14159);
    MtxRotateY(m, 0.1*data->ay);
    MtxRotateX(m, 0.4);


    matrix_t tmp;

/*    MtxCopy(tmp, m);*/
    MtxIdentity(tmp);
    MtxRotateZ(tmp, 3.14159);
    MtxRotateY(tmp, -0.33468713*data->ay);
    MtxRotateX(tmp, 0.4);
    MtxTranspose(tmp);
    MtxVectMult(&tlight_normal.x, &light_normal.x, tmp);


    data->zoom = 1;
    MtxScale(m,data->zoom*32*4);
    m[3][0] = data->x;
    m[3][1] = data->y;
    m[3][2] = data->z;

    MtxProjection(tmp, 70*2.0*3.14159/360,
		  0.01, (float)SCREEN_W/SCREEN_H,
		  1000);
    MtxMult(m, tmp);

    vlr_update();
    vlr_render(m);
  }


  return err;
}



static int anim_intro(anim_t *a, unsigned int elapsed_frames)
{
#if 0
  static obj_t * objects[] = { &trois, &deux, &un, &zero, 0 };
  static int cur_obj = 0;
  //  const int recount = 200;
  //  static int frame_cnt = recount;

  const float sax =  0.0201f * 0.7f;
  const float say =  0.0212f * 3.07f;
  //  const float pi = 3.1415926f;
  const float deux_pi = 6.283185f;
  
  int err = ANIM_CONT;

  a->x = a->y = 0.0f;
  a->z = 80.0f;
  a->zoom = 1.7f;
  a->flash = 0;

  a->ay += say * elapsed_frames;
  a->ax += sax * elapsed_frames;
  if (a->ay > deux_pi) {
    a->ay -= deux_pi;
    cur_obj++;
    err = ANIM_CYCLE;
  }
  
  if (a->o = objects[cur_obj], !a->o) {
    err = ANIM_END;
  }
  /* else {
     dbglog( DBG_DEBUG, "-- " __FUNCTION__ " : Change object vtx:%d/%d tri:%d/%d\n",
     a->o->nbv, a->o->static_nbv, a->o->nbf, a->o->static_nbf);
     }*/
  

  /*  
      frame_cnt -= elapsed_frames;
      if (frame_cnt<=0) {
      frame_cnt += recount;
      cur_obj++;
      }
  */

  return err;
#else
  return ANIM_END;
#endif
}

static int render_intro(uint32 elapsed_frames)
{
  int code;
  
  code = render_anim_object(elapsed_frames, anim_intro, &animdata);
  
  return code == ANIM_END;
}

//extern unsigned int vmu_get_peek();

static int same_sign(float a, float b) {
  return (a>=0 && b>=0) || (a<0 && b<0);
}

static int anim_logo(anim_t *a, unsigned int iframes)
{
  const float fframes = (const float)iframes;
  
  const float rspd_max = 2.7f;
  const float rspd_min = 0.2f;  
 
  static float rspd   = 0.0f;
  //  static float oflash = 0.0f;
  /*  static float rspd2  = 0.0f; */
  
  float sax = 0.0321f * fframes;
  float say = 0.0212f * fframes;
  float saz = 0.0313f * fframes;
 
  float peek;
  float zoom;
  
  /*    peek = (float)peek1.dyn / 65536.0f; */
  /*    zoom = 0.3f + peek*2.2f; */
  
  /*    if (peek > opeek) { */
  /*      float f = (peek-opeek) * 0.95f; */
  /*      rspd = f * rspd_max + (1.0f-f) * rspd; */
  /*    } else if (peek < opeek) { */
  /*      float f = opeek-peek; */
  /*      rspd = f * rspd_min + (1.0f-f) * rspd; */
  /*    } else { */
  /*      rspd = 0.05f * rspd_min + .95f * rspd; */
  /*    } */
  /*    opeek = peek; */
  
  /*    a->x = a->y = 0.0f; */
  /*    a->z = 80.0f; */
  
  
  /*    { */
  /*      float r; */
  /*      r = (zoom > a->zoom) ? 0.70f : 0.90f; */
  /*      a->zoom = a->zoom * r + zoom * (1.0f-r); */
  /*    } */
  /*    a->ax += sax * rspd * 2.0f; */
  /*    a->ay += say * rspd * 2.0f; */
  /*    a->az += saz * rspd * 2.0f; */

  /* analysis parms */
  float
    peek_fact,    /* full range scale factor */
    peek_diff,    /* peek/oldpeek delta */
    peek_avgdiff, /* peek/avgpeek delta */
    peek_norm;    /* peek normalized to full range */

  static float
    opeek_diff    = 0.0f,
    opeek         = 0.0f,
    peek_max      = 0.0f,
    peek_diff_max = 0.0f;

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
      a->flash = opeek_diff * 2.60f;
      //      dbglog(DBG_DEBUG," *");
    

      if (opeek_diff >= 0.30f) {
	zoom += 0.3f;
	//a->flash = -a->flash;
	//dbglog(DBG_DEBUG,"$");
      }
    }
  }
  peek_diff_max *= 0.995f;

  a->flash *= 0.9f;
  if (a->flash < 0 && a->flash > -1E-3) {
    a->flash = 0;
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

  a->x = a->y = 0.0f;
  a->z = 80.0f;
  
  
  {
    float r;
    r = (zoom > a->zoom) ? 0.75f : 0.90f;
    a->zoom = a->zoom * r + zoom * (1.0f-r);
  }
  a->ax += sax * rspd * 2.0f;
  a->ay += say * rspd * 2.0f;
  a->az += saz * rspd * 2.0f;
  
  a->o = curlogo;
  
  return ANIM_CONT;
}

static int render_logo(uint32 elapsed_frames)
{
  int code;
  
  code = render_simple_anim_object(elapsed_frames, anim_logo, &animdata);
  
  return code == ANIM_END;
}


// $$$ Test built-in drivers 
//extern driver_list_t inp_driver;
//extern inp_driver_t xing_driver;


static int driver_init(void)
{
  int err=0;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "\n");

  /* Init driver list */
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Init decoder driver list\n");
  err = driver_list_init_all();
  if (err < 0) {
    goto error;
  }

  /* Load the default drivers from romdisk */
  {
    const char *path1 = "/pc" DREAMMP3_HOME "plugins/inp/ogg";
    const char *path2 = "/pc" DREAMMP3_HOME "plugins/inp/xing";
    dbglog(DBG_DEBUG,"** " __FUNCTION__
	   " : Load default drivers\n[%s]\n[%s]\n",
	   path1, path2);
    err  = plugin_path_load(path1, 1);
    err = plugin_path_load(path2, 1);
  }

  /* Load built-in plugins */
  //driver_list_register(&inp_drivers, (any_driver_t *)&xing_driver);
  //xing_driver.init();

 error:
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ " := %d\n", err);
  return err;
}

static int no_mt_init(void)
{
  int err = 0;
  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "\n");

  /* Driver list */
  if (driver_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Start sound */
  if (sound_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Setup background display */
  if (bkg_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Setup border poly */
  if (border_setup() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Find a mouse if there is one */
  //mmouse = maple_first_mouse();

  /* Init VMU stuff */
  if (vmu68_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Init font 16x16 */
  if (text_setup() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Init song menu */
  if (songmenu_init() < 0) {
    err = __LINE__;
    goto error;
  }
  
  /* Init info */
  if (info_setup() < 0) {
    err = __LINE__;
    goto error;
  }
  
  /* Init option */
  if (option_setup() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Prepare 3D objects */
  /*
  if (object_setup() < 0) {
    err = __LINE__;
    goto error;
  }
  */
  
  /* Remanens FX */
  if (remanens_setup() < 0) {
    err = __LINE__;
    goto error;
  }
  
  /* Start controller */
  if (controler_init(frame_counter68) < 0) {
    err = __LINE__;
    goto error;
  }


 error:
  dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : error line [%d]\n", err);
  return err;
}

static void update_lcd(void)
{
  int *buf, nb, cnt, frq;

  playa_get_buffer(&buf, &nb, &cnt, &frq);
  vupeek_adddata(buf, nb, cnt, frq);
  vmu_lcd_update(buf, nb, cnt);
}

static void update_fft(void)
{
  int *buf, nb, cnt, frq;
  static int scnt = -1;

  playa_get_buffer(&buf, &nb, &cnt, &frq);
  if (cnt == scnt) return;
  scnt = cnt;
  fft(buf, nb, cnt, frq);

}

void main_thread(void *cookie)
{
  const int exit_buttons = CONT_START|CONT_A|CONT_Y;
  int err = 0;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "\n");

  vid_border_color(0,0,0);

  if (songmenu_start() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Load default disk from ROM */
  /*  if (dreamcast68_loaddisk("/rd/test.mp3", 1) < 0) {
      err = __LINE__;
      goto error;
      }*/
  dreamcast68_loaddisk("/rd/test.mp3", 1);
  thd_pass(); // $$$ Don't ask me why !!! It removes a bug in intro sound !!!

  fade68    = 0.0f;
  fade_step = 0.02f;
#ifdef SKIP_INTRO
  if (0)
#endif  
    {
      int end = 0;
      while (!end) {
	uint32 elapsed_frames;

	ta_begin_render();
	pipo_poly(0);

	/* Update FFT */
	update_fft();

	/* Update the VMU LCD */
	update_lcd();

	elapsed_frames = frame_counter68;
	frame_counter68 = ta_state.frame_counter;
	elapsed_frames = frame_counter68 - elapsed_frames;
	fade(elapsed_frames);
	controler_read(&controler68, frame_counter68);
      
	ta_commit_eol();

	end = render_intro(elapsed_frames);
	ta_commit_eol();

	/* Finish the frame *******************************/
/*	extern kthread_t * playa_thread;
	if (playa_thread)
	  thd_set_prio(playa_thread, PRIO_DEFAULT);
	ta_finish_frame();
	if (playa_thread)
	  thd_set_prio(playa_thread, PRIO_DEFAULT-1);*/

	ta_finish_frame();

	// playa_decoderupdate();

      }
    }
  
  fade68    = 0.0f;
  fade_step = 0.005f;  
  while ( (controler68.buttons & exit_buttons) != exit_buttons) {
    uint32 elapsed_frames;
    int is_playing = playa_isplaying();

    my_vid_border_color(0,0,0);
    ta_begin_render();
    //my_vid_border_color(255,255,255);
    pipo_poly(0);
    
    elapsed_frames = frame_counter68;
    frame_counter68 = ta_state.frame_counter;
    elapsed_frames = frame_counter68 - elapsed_frames;
    fade(elapsed_frames);
    controler_read(&controler68, frame_counter68);

    /* Update FFT */
    update_fft();

    /* Update the VMU LCD */
    update_lcd();

    /* Opaque list *************************************/
    //my_vid_border_color(255,0,0);
    bkg_render(fade68, info_is_help() || !is_playing);
    
    /* End of opaque list */
    ta_commit_eol();
    my_vid_border_color(0,255,0);

    if (option_visual()) {
      float save = fade68;
      fade68 = 1.0f;
      render_logo(elapsed_frames);
      fade68 = save;
    }
    my_vid_border_color(0,0,255);    
    /* Translucent list ********************************/
    info_render(elapsed_frames, is_playing);
    my_vid_border_color(255,255,255);
    songmenu_render(elapsed_frames);
    option_render(elapsed_frames);
    my_vid_border_color(255,0,255);    
    /* End of translucent list */
    ta_commit_eol();

    //my_vid_border_color(255,0,0);

    /* Finish the frame *******************************/
/*    extern kthread_t * playa_thread;
    if (playa_thread)
      thd_set_prio(playa_thread, PRIO_DEFAULT);
    ta_finish_frame();
    if (playa_thread)
      thd_set_prio(playa_thread, PRIO_DEFAULT-1);*/
    
    ta_finish_frame();
    
    my_vid_border_color(0,0,0);    
  }

  dbglog(DBG_DEBUG, "** "  __FUNCTION__ " : Start exit procedure\n");

  /* Stop sc68 from playing */
  dreamcast68_loaddisk(0,0);
  vmu_lcd_title();

  /* */
  stream_shutdown();

  /* Stop the sound */
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : disable SPU\n");
  spu_disable();
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : SPU disabled\n");

  /* Stop songmenu */
  songmenu_kill();
  err = 0;

 error:
  if (cookie) {
    *(int *)cookie = err;
  }
  dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : error line [%d]\n", err);
}

extern uint8 romdisk[];


/* Program entry :
 *
 * - Init some static variables
 * - KOS init
 * - Init all no thread safe
 * - Launch main thread and wait ...
 */
int dreammp3_main(int argc, char **argv)
{
  int err = 0;

  curlogo = 0; //& mine_3;
  volume = 255;
  fade68 = 0.0f;
  fade_step = 0.01f;
  memset(&animdata,0,sizeof(animdata));
  play_thd = 0;

#ifdef RELEASE
  dbglog_set_level(0);
#else
  dbglog_set_level(DBG_DEBUG);
#endif

  /* Do basic setup */

  kos_init_all(IRQ_ENABLE | THD_ENABLE, romdisk);
  vid_border_color(0,0,0);

  ta_init(TA_LIST_OPAQUE_POLYS | TA_LIST_TRANS_POLYS, TA_POLYBUF_32, 1024*1024);
/*  ta_set_buffer_config(TA_LIST_OPAQUE_POLYS | TA_LIST_TRANS_POLYS, TA_POLYBUF_32, 1024*1024);
  ta_hw_init();*/
  

  frame_counter68 = ta_state.frame_counter;
  spinlock_init(&app68mutex);

  /* Run no multi-thread setup (malloc/free rules !) */
  if (no_mt_init() < 0) {
    err = __LINE__;
    goto error;
  }
  //return 0;
  
  /* WARNING MESSAGE */
  dbglog( DBG_DEBUG, "** " __FUNCTION__ " : starting WARNING screen\n");
  warning_splash();
  dbglog( DBG_DEBUG, "** " __FUNCTION__ " : End of WARNING screen\n");
  

  /* MAIN */
  main_thread(0);

  //  return 0;


error:
  dbglog_set_level(
DBG_DEBUG);
  dbglog( DBG_DEBUG, ">> " __FUNCTION__ " : error line [%d]\n", err);

  return 0;
}

void toto()
{
  printf("callback !!\n");
}

static int warning_splash(void)
{
  int end = 0;
  unsigned int end_frame = 0;

  fade68 = 0.0f;
  fade_step = 0.01f;

  vmu_lcd_title();

#ifdef SKIP_INTRO
  end = 1;
#endif
	
  while (!end) {
    uint32 elapsed_frames;

    ta_begin_render();
    pipo_poly(0);
    elapsed_frames = frame_counter68;
    frame_counter68 = ta_state.frame_counter;
    elapsed_frames = frame_counter68 - elapsed_frames;
    fade(elapsed_frames);
    controler_read(&controler68, frame_counter68);

    if (controler_released(&controler68, CONT_START)) {
      end_frame = frame_counter68;
    }

    if (fade68 == 1.0f) {
      if (!end_frame) {
	end_frame = frame_counter68 + 60 * 30;
      } else if (frame_counter68 > end_frame) {
	fade_step = -0.01f;
      }
    }

    end = end_frame && fade68 == 0.0f;

    /* Opaque list ************************************ */
    /* End of opaque list */
    ta_commit_eol();

    /* Translucent list ******************************* */
    warning_render();

    /* End of translucent list */
    ta_commit_eol();

    /* Finish the frame ****************************** */
    ta_finish_frame();

  }
  return 0;
}
