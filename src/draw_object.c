/**
 * $Id: draw_object.c,v 1.18 2003-01-28 22:58:18 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <dc/ta.h>

#include "math_float.h"
#include "draw/ta.h"
#include "sysdebug.h"
#include "draw_object.h"

typedef struct {
  float u;
  float v;
} uv_t;

static matrix_t mtx;
static ta_hw_tex_vtx_t * const hw = HW_TEX_VTX;

typedef struct {
  float x,y,z;
  int flags;
} pvtx_t;

typedef struct {
  vtx_t v;
  pvtx_t p;
} tvtx_t;

static tvtx_t * transform = 0;
static int transform_sz = 0;
static float oozNear, zNear;
static const int strong_mask = 7; // 0 for no strong link

#define U1 (02.0f/64.0f)
#define U2 (30.0f/64.0f)
#define U3 (34.0f/64.0f)
#define U4 (62.0f/64.0f)

static uv_t uvlinks[8][4] =
  {
    /* 0 cba */  { {U1,U2},   {U1,U1},    {U2,U1},   {0,0} },
    /* 1 cbA */  { {U3,U1},   {U3,U2},    {U4,U1},   {0,0} },
    /* 2 cBa */  { {U4,U1},   {U3,U2},    {U3,U1},   {0,0} },
    /* 3 cBA */  { {U1,U4},   {U1,U3},    {U2,U3},   {0,0} },
    /* 4 Cba */  { {U3,U1},   {U4,U1},    {U3,U2},   {0,0} },
    /* 5 CbA */  { {U1,U3},   {U2,U3},    {U1,U4},   {0,0} },
    /* 6 CBa */  { {U2,U3},   {U1,U4},    {U1,U3},   {0,0} },
    /* 7 CBA */  { {U3,U4},   {U3,U3},    {U4,U3},   {0,0} },
  };

inline static int sature(const float a)
{
  int v;

  v = (int)a;
  v &= ~(v>>31);
  v |= (255-v) >> 31;
  v &= 255;
  return v;
}

inline static unsigned int argb255(const vtx_t *color)
{
  return
    (sature(color->w*255.0f) << 24) |
    (sature(color->x*255.0f) << 16) |
    (sature(color->y*255.0f) << 8)  |
    (sature(color->z*255.0f) << 0);
}

inline static unsigned int argb4(const float a, const float r,
			  const float g, const float b)
{
  return
    (sature(a) << 24) |
    (sature(r) << 16) |
    (sature(g) << 8)  |
    (sature(b) << 0);
}

static void dump_clipflags(int f)
{
  int i;
  static char t[7], t2[] = "xXyYzZ";
  for (i=0; i<6; ++i) {
    t[i] = (f&(1<<i)) ? t2[i] : '.';
  }
  t[i] = 0;
  printf(t);
}  

static void dump_vtxflags(const pvtx_t *p)
{
  printf("[");
  dump_clipflags(p->flags>>6);
  printf(" ");
  dump_clipflags(p->flags);
  printf("]");
}

static void pvtx_dump(const pvtx_t * v)
{
  printf("[x:%f y:%f z:%f ", v->x, v->y, v->z);
  dump_vtxflags(v);
  printf("]");
}

static void dump_triflags(const tri_t *f)
{
  printf("[%c ", (f->flags&1) ? 'I' : '.');
  dump_clipflags(f->flags>>(8+0));
  printf(" ");
  dump_clipflags(f->flags>>(8+6));
  printf(" ");
  dump_clipflags(f->flags>>(8+12));
  printf("]");
}

static void tvtx_dump(const tvtx_t * v, const char * tabs)
{
  printf(tabs);
  vtx_dump(&v->v);
  printf(tabs);
  pvtx_dump(&v->p);
}

static void dump_face(const tri_t * f, const tlk_t * l, const tri_t * t,
		      const tvtx_t * transfo, const char * label )
{
  int i;
  printf("%s : [A:%d%s B:%d%s C:%d%s] ",
	 label,
	 f->a, (t[l->a].flags&1)?"*":"",
	 f->b, (t[l->b].flags&1)?"*":"",
	 f->c, (t[l->c].flags&1)?"*":"");
  dump_triflags(f);
  printf("\n[");
  for (i=0; i<3; ++i) {
    tvtx_dump(transfo + ((int*)&f->a)[i],"\n  ");
  }
  printf("\n]\n");
}


static int init_transform(int nb)
{
  tvtx_t *t;

  if (nb <= transform_sz) {
    return 0;
  }

  t = (tvtx_t *)realloc(transform, nb * sizeof(*t));
  if (!t) {
    return -1;
  }
  transform = t;
  transform_sz = nb;
  return 0;
}

static void TransformVtx(tvtx_t * d, const vtx_t *v, int n,
			 const viewport_t *vp, matrix_t m)
{
  const float mx = vp->mx;
  const float my = vp->my;
  const float tx = vp->tx;
  const float ty = vp->ty;

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

  // $$$ Optimize with special opcode
  do { 
    const float x = v->x, y = v->y, z = v->z; 
    const float oow = Inv((x * m03 + y * m13 + z * m23) + m33);
    //    const float z2 = (x * m02 + y * m12 + z * m22) + m32;

    d->p.x = ((x * m00 + y * m10 + z * m20) + m30) * oow * mx + tx;
    d->p.y = ((x * m01 + y * m11 + z * m21) + m31) * oow * my + ty;
    d->p.z = Inv((x * m02 + y * m12 + z * m22) + m32);
    ++d;
    ++v; 
  } while (--n);
}

inline static void proj_vtx(tvtx_t * d,
			    const float mx, const float my,
			    const float tx, const float ty,
			    const float oow)
{
  d->p.x = d->v.x * oow * mx + tx;
  d->p.y = d->v.y * oow * my + ty;
  d->p.z = Inv(d->v.z + 1);
}

static void ProjectVtx(tvtx_t * d, int n,
		       const viewport_t *vp)
{
  const float mx = vp->mx;
  const float my = vp->my;
  const float tx = vp->tx;
  const float ty = vp->ty;

  // $$$ Optimize with special opcode
  do { 
    proj_vtx(d, mx, my, tx, ty, Inv(d->v.w));
    ++d;
  } while (--n);
}

static void ProjectVtxNotClipped(tvtx_t * d, int n,
				 const viewport_t *vp, const int mask)
{
  const float mx = vp->mx;
  const float my = vp->my;
  const float tx = vp->tx;
  const float ty = vp->ty;

  // $$$ Optimize with special opcode
  do {
    if (! (d->p.flags & mask) ) {
      proj_vtx(d, mx, my, tx, ty, Inv(d->v.w));
    }
    ++d;
  } while (--n);
}

inline static int test_visible(const pvtx_t *t0,
			       const pvtx_t *t1,
			       const pvtx_t *t2)
{
    const float a = t1->x - t0->x;
    const float b = t1->y - t0->y;
    const float c = t2->x - t0->x;
    const float d = t2->y - t0->y;
    return (a * d - b * c) <= 0;
}

static void TestVisibleFace(const obj_t * o)
{
  tri_t * f = o->tri;
  int     n = o->nbf;
  do {
    f->flags = test_visible(&transform[f->a].p,
			    &transform[f->b].p,
			    &transform[f->c].p);
    ++f;
  } while(--n);
}

static void TestVisibleFaceNotClipped(const obj_t * o, const int omask)
{
  const int mask = (omask | (omask<<6) | (omask<<12)) << 8;
  tri_t * f = o->tri;
  int     n = o->nbf;
  do {
    const pvtx_t *t0 = &transform[f->a].p;
    const pvtx_t *t1 = &transform[f->b].p;
    const pvtx_t *t2 = &transform[f->c].p;
    int flags;

    flags = 0
      | (t0->flags << 8)
      | (t1->flags << (8+6))
      | (t2->flags << (8+12));

    if (!(flags&mask)) {
      flags |= test_visible(t0,t1,t2);
    }
    f->flags = flags;
    ++f;
  } while(--n);
}

typedef int cl_t;
/* Set TA clipping */
static void set_clipping(cl_t xmin, cl_t ymin, cl_t xmax, cl_t ymax)
{
  struct {
    uint32 com;
    uint32 r0,r1,r2;
    cl_t xmin;
    cl_t ymin;
    cl_t xmax;
    cl_t ymax;
  } c;
  c.com = 0x1<<29; // USER CLIP
  c.xmin = xmin;
  c.ymin = ymin;
  c.xmax = xmax;
  c.ymax = ymax;
  c.r0 = c.r1 = c.r2 = 0;
  ta_commit32_inline(&c);
}

#include "controler.h"

extern controler_state_t controler68;

int DrawObjectPostProcess(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o)
{
  if (!vp || !o) {
    return -1;
  }

  if (init_transform(o->nbv) < 0) {
    return -1;
  }

  MtxMult3(mtx, local, proj);

  TransformVtx(transform, o->vtx, o->nbv, vp, mtx);
  TestVisibleFace(o);

  return 0;
}

static int SingleColor(obj_t *o, const vtx_t * color)
{
  unsigned int col;
  col = argb255(color);

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;

    DRAW_SET_FLAGS(o->flags);

    hw->addcol = 0;
    hw->col = col;
 
    if (!l) {
      return -1;
    }
    for(n=o->nbf ;n--; ++f, ++l, ++nrm)  {
      int lflags;
      uv_t * uvl;

      if (f->flags) {
	continue;
      }

      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      // Strong link :
      lflags |= strong_mask & l->flags;
	
      uvl = uvlinks[lflags];

      hw->flags = TA_VERTEX_NORMAL;
      hw->x = transform[f->a].p.x;
      hw->y = transform[f->a].p.y;
      hw->z = transform[f->a].p.z;
      hw->u = uvl[0].u;
      hw->v = uvl[0].v;
      ta_commit32_nocopy();

      //	hw->flags = TA_VERTEX;
      hw->x = transform[f->b].p.x;
      hw->y = transform[f->b].p.y;
      hw->z = transform[f->b].p.z;
      hw->u = uvl[1].u;
      hw->v = uvl[1].v;
      ta_commit32_nocopy();

      hw->flags = TA_VERTEX_EOL;
      hw->x = transform[f->c].p.x;
      hw->y = transform[f->c].p.y;
      hw->z = transform[f->c].p.z;
      hw->u = uvl[2].u;
      hw->v = uvl[2].v;
      ta_commit32_nocopy();
    }
  }
  return 0;
}

int DrawObjectSingleColor(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o, const vtx_t *color)
{

  if (DrawObjectPostProcess(vp, local, proj, o) < 0) {
    return -1;
  }
  return SingleColor(o, color);
}

int DrawObjectLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
		      vtx_t *ambient, vtx_t *light, vtx_t *diffuse)
{
  float aa, ar, ag, ab;
  float la, lr, lg, lb;
  vtx_t tlight;
  matrix_t lightmtx;

  if (DrawObjectPostProcess(vp, local, proj, o) < 0) {
    return -1;
  }

  MtxCopy(lightmtx, local);
  MtxTranspose(lightmtx);
  MtxVectMult(&tlight.x, &light->x, lightmtx);

  ar = 255.0f * ambient->x;
  ag = 255.0f * ambient->y;
  ab = 255.0f * ambient->z;
  aa = 255.0f * ambient->w;

  lr = 255.0f * diffuse->x;
  lg = 255.0f * diffuse->y;
  lb = 255.0f * diffuse->z;
  la = 255.0f * diffuse->w;

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;
    
    DRAW_SET_FLAGS(o->flags);

    hw->addcol = 0;
 
    if (!l) {
      // $$$ !
      return -1;
    }
    for(n=o->nbf ;n--; ++f, ++l, ++nrm)  {
      int lflags;
      uv_t * uvl;
      float coef;

      if (f->flags) {
	continue;
      }

      coef = nrm->x * tlight.x 
	+ nrm->y * tlight.y 
	+ nrm->z * tlight.z;

      coef *= coef;
/*       if (coef < 0) { */
/* 	coef = 0; */
/*       } */
      hw->col = argb4(aa + coef * la, ar + coef * lr,
		      ag + coef * lg, ab + coef * lb);
	
      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      lflags |= strong_mask & l->flags;
	
      uvl = uvlinks[lflags];

      hw->flags = TA_VERTEX_NORMAL;
      hw->x = transform[f->a].p.x;
      hw->y = transform[f->a].p.y;
      hw->z = transform[f->a].p.z;
      hw->u = uvl[0].u;
      hw->v = uvl[0].v;
      ta_commit32_nocopy();

      //	hw->flags = TA_VERTEX;
      hw->x = transform[f->b].p.x;
      hw->y = transform[f->b].p.y;
      hw->z = transform[f->b].p.z;
      hw->u = uvl[1].u;
      hw->v = uvl[1].v;
      ta_commit32_nocopy();

      hw->flags = TA_VERTEX_EOL;
      hw->x = transform[f->c].p.x;
      hw->y = transform[f->c].p.y;
      hw->z = transform[f->c].p.z;
      hw->u = uvl[2].u;
      hw->v = uvl[2].v;
      ta_commit32_nocopy();
      
    }
  }
  return 0;
}

int DrawObjectFrontLighted(viewport_t * vp, matrix_t local, matrix_t proj,
			   obj_t * o,
			   const vtx_t * ambient, const vtx_t * diffuse)
{
  float aa, ar, ag, ab;
  float la, lr, lg, lb;

  const float m02 = local[0][2];
  const float m12 = local[1][2];
  const float m22 = local[2][2];

  if (DrawObjectPostProcess(vp, local, proj, o) < 0) {
    return -1;
  }

  ar = 255.0f * ambient->x;
  ag = 255.0f * ambient->y;
  ab = 255.0f * ambient->z;
  aa = 255.0f * ambient->w;

  lr = 255.0f * diffuse->x;
  lg = 255.0f * diffuse->y;
  lb = 255.0f * diffuse->z;
  la = 255.0f * diffuse->w;

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;

    DRAW_SET_FLAGS(o->flags);

    hw->addcol = 0;
 
    if (!l) {
      // $$$ !
      return -1;
    }
    for(n=o->nbf ;n--; ++f, ++l, ++nrm)  {
      int lflags;
      uv_t * uvl;
      float coef;

      if (f->flags) {
	continue;
      }

      coef = m02 * nrm->x + m12 * nrm->y + m22 * nrm->z;
      coef *= coef;

/*       if (coef < 0) { */
/* 	coef = -1 * coef; */
/* 	hw->col = 0xFF7f0000 + (sature(coef * 127.0f)<<16); */
/*       } else { */
/* 	hw->col = argb4(aa + coef * la, ar + coef * lr, */
/* 			ag + coef * lg, ab + coef * lb); */
/*       } */

      hw->col = argb4(aa + coef * la, ar + coef * lr,
		      ag + coef * lg, ab + coef * lb);

	
      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      lflags |= strong_mask & l->flags;
	
      uvl = uvlinks[lflags];

      hw->flags = TA_VERTEX_NORMAL;
      hw->x = transform[f->a].p.x;
      hw->y = transform[f->a].p.y;
      hw->z = transform[f->a].p.z;
      hw->u = uvl[0].u;
      hw->v = uvl[0].v;
      ta_commit32_nocopy();

      //	hw->flags = TA_VERTEX;
      hw->x = transform[f->b].p.x;
      hw->y = transform[f->b].p.y;
      hw->z = transform[f->b].p.z;
      hw->u = uvl[1].u;
      hw->v = uvl[1].v;
      ta_commit32_nocopy();

      hw->flags = TA_VERTEX_EOL;
      hw->x = transform[f->c].p.x;
      hw->y = transform[f->c].p.y;
      hw->z = transform[f->c].p.z;
      hw->u = uvl[2].u;
      hw->v = uvl[2].v;
      ta_commit32_nocopy();
      
    }
  }
  return 0;
}


int DrawObjectPrelighted(viewport_t * vp, matrix_t local, matrix_t proj,
			 obj_t *o)
{
  if (DrawObjectPostProcess(vp, local, proj, o) < 0) {
    return -1;
  }

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;
    
 
    if (!l) {
      // $$$ !
      return -1;
    }

    DRAW_SET_FLAGS(o->flags);
    hw->addcol = 0;

/* #define SMOOTH_IT */

    for(n=o->nbf ;n--; ++f, ++l, ++nrm)  {
      int lflags;
      unsigned int col;
#ifdef SMOOTH_IT
      unsigned int cola, colb, colc,
#endif
      uv_t * uvl;

      if (f->flags) {
	continue;
      }

      col = *(int *)&nrm->w;

#ifdef SMOOTH_IT
      cola = *(int *)&o->nvx[l->a].w;
      colb = *(int *)&o->nvx[l->b].w;
      colc = *(int *)&o->nvx[l->c].w;

      // $$$ ben : hack to remove invible face glitch! There is lotsa optimize to do here.
      {
	const int m2 = 0x7f7f7f7f;
	int tmp, m;
	col  = (col  >> 1) & 0x7f7f7f7f;
	tmp = col/* >> 1*/;

	m = -t[l->a].flags;
	m = -1;
	cola = (((cola >> 1) & ~m) | (tmp & m)) & m2;  

	m = -t[l->b].flags;
	m = -1;
	colb = (((colb >> 1) & ~m) | (tmp & m)) & m2;  

	m = -t[l->c].flags;
	m = -1;
	colc = (((colc >> 1) & ~m) | (tmp & m)) & m2;  
      }
#endif

      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      lflags |= strong_mask & l->flags;
	
      uvl = uvlinks[lflags];

#ifndef SMOOTH_IT
      hw->col = col;
#endif

#ifdef SMOOTH_IT
      hw->col = /*col + */cola + colc;
#endif
      hw->flags = TA_VERTEX_NORMAL;
      hw->x = transform[f->a].p.x;
      hw->y = transform[f->a].p.y;
      hw->z = transform[f->a].p.z;
      hw->u = uvl[0].u;
      hw->v = uvl[0].v;
      ta_commit32_nocopy();

      //	hw->flags = TA_VERTEX;

#ifdef SMOOTH_IT
      hw->col = /*col + */colb + cola;
#endif
      hw->x = transform[f->b].p.x;
      hw->y = transform[f->b].p.y;
      hw->z = transform[f->b].p.z;
      hw->u = uvl[1].u;
      hw->v = uvl[1].v;
      ta_commit32_nocopy();

#ifdef SMOOTH_IT
      hw->col = /*col + */colc + colb;
#endif
      hw->flags = TA_VERTEX_EOL;
      hw->x = transform[f->c].p.x;
      hw->y = transform[f->c].p.y;
      hw->z = transform[f->c].p.z;
      hw->u = uvl[2].u;
      hw->v = uvl[2].v;
      ta_commit32_nocopy();
      
    }
  }
  return 0;
}

static int set_clipping_flags(tvtx_t * v, int nbv)
{
#define Z_NEAR_ONLY 0
  int aflags = 077;
  int oflags = 0;
  if (nbv) {
    do {
#if Z_NEAR_ONLY
      int flags = vtx_znear_clip_flags(&v->v);
#else
      int flags = vtx_clip_flags(&v->v);
#endif
      v->p.flags = flags;
      ++v;
      oflags |= flags;
      aflags &= flags;
    } while (--nbv);
  }
  return ((aflags<<6) | oflags)
#if Z_NEAR_ONLY
		 << 4
#endif
    ;
}

static void draw_tri_single(const tri_t * f, /* triangle to draw */
			    const tlk_t * l, /* link to draw     */
			    const tri_t * t, /* triangle buffer */ 
			    const tvtx_t * transfo)
{
  int lflags;
  uv_t * uvl;
  const pvtx_t * pv;
  
  lflags  = (1&t[l->a].flags) << 0;
  lflags |= (1&t[l->b].flags) << 1;
  lflags |= (1&t[l->c].flags) << 2;
  lflags |= strong_mask & l->flags;

  lflags = 7;

  uvl = uvlinks[lflags];

  hw->flags = TA_VERTEX_NORMAL;
  pv = &transfo[f->a].p;
  hw->x = pv->x;
  hw->y = pv->y;
  hw->z = pv->z;
  hw->u = uvl[0].u;
  hw->v = uvl[0].v;
  ta_commit32_nocopy();

  pv = &transfo[f->b].p;
  hw->x = pv->x;
  hw->y = pv->y;
  hw->z = pv->z;
  hw->u = uvl[1].u;
  hw->v = uvl[1].v;
  ta_commit32_nocopy();

  hw->flags = TA_VERTEX_EOL;
  pv = &transfo[f->c].p;
  hw->x = pv->x;
  hw->y = pv->y;
  hw->z = pv->z;
  hw->u = uvl[2].u;
  hw->v = uvl[2].v;
  ta_commit32_nocopy();
}

static void clip_znear_vtx(tvtx_t * d, const vtx_t * a, const vtx_t * b, 
			   const viewport_t * vp)
{
  const float f = (zNear-a->w) / (b->w - a->w);
/*   const float f2 = -a->z  / (b->z - a->z); */
  const float of = 1.0-f;

  d->v.x = a->x * of + b->x * f;
  d->v.y = a->y * of + b->y * f;
  d->v.z = 0; //a->z * of + b->z * f;
#if 0
  d->v.w = zNear; //a->w * of + b->w * f;
#endif
  proj_vtx(d, vp->mx, vp->my, vp->tx, vp->ty, oozNear);
}

static void draw_clip_znear_tri(const tri_t * f, /* triangle to draw */
				const tlk_t * l, /* link to draw     */
				const tri_t * t, /* triangle buffer  */ 
				const tvtx_t * transform,
				const viewport_t * vp)
{
  const tvtx_t *va, *vb, *vc;
  int flags;

  static tvtx_t cvtx[4];

  static const tri_t ctri[] = {
    { 0,0,0,1 }, /* 0: Invisible */
    { 0,1,2,0 }, /* 1: 1st build triangle, case 1 out/in */
    { 0,2,3,0 }, /* 2: 2st build triangle, case 1 out */
    { 1,0,2,0 }, /* 3: back-side, 1st build triangle, case 1 out/in */
    { 2,0,3,0 }, /* 4: back-side, 2st build triangle, case 1 out */
  };

  static tlk_t ctlk[1] = {
  };

  static struct {
    int va, vb, vc;
  } *ci, clipinfo [8] = {
    { },          /* 0 :  */
    { 0,1,2,  },  /* 1 : A-out */
    { 1,2,0,  },  /* 2 : B-out */
    { 2,0,1,  },  /* 3 : C-in  */
    { 2,0,1,  },  /* 4 : C-out */
    { 1,2,0,  },  /* 5 : B in  */
    { 0,1,2,  },  /* 6 : A in  */
    { },          /* 7 :  */
  };

  flags = f->flags;
  if (flags&1) {
    /* backface (only projected face were flaged...) */
    /* $$$ */
/*     return; */
  }
  
  flags >>= (8+4);
  flags &= 010101;
  if (!flags) {
    /* No clip */
    draw_tri_single(f,l,t,transform);
    return;
  }
  if (flags == 010101) {
    /* All clipped */
    return;
  }

  flags = ((flags >> 0) | (flags >> 5) | (flags >> 10)) & 7;

  ci = clipinfo + flags;
  va = transform + ((int*)&f->a)[ci->va];
  vb = transform + ((int*)&f->a)[ci->vb];
  vc = transform + ((int*)&f->a)[ci->vc];

  switch (flags) {
  case 1: case 2: case 4:
    {
      clip_znear_vtx(&cvtx[0], &va->v, &vb->v, vp);
      cvtx[1] = *vb;
      cvtx[2] = *vc;
      clip_znear_vtx(&cvtx[3], &va->v, &vc->v, vp);

      if (!test_visible(&cvtx[0].p,&cvtx[1].p,&cvtx[2].p)) {
	draw_tri_single(ctri+1,ctlk,ctri,cvtx);
      }
      if (!test_visible(&cvtx[0].p,&cvtx[2].p,&cvtx[3].p)) {
	draw_tri_single(ctri+2,ctlk,ctri,cvtx);
      }
    } break;

  case 3: case 5: case 6:
    {
      cvtx[0] = *va;
      clip_znear_vtx(&cvtx[1], &vb->v, &va->v, vp);
      clip_znear_vtx(&cvtx[2], &vc->v, &va->v, vp);
      if (!test_visible(&cvtx[0].p,&cvtx[1].p,&cvtx[2].p)) {
	draw_tri_single(ctri+1,ctlk,ctri,cvtx);
      }
    } break;
  default:
    BREAKPOINT(0xDEADCCCC);
  }
}

static void draw_clip_znear_object(obj_t *o, viewport_t * vp,
				   const vtx_t * color)
{
  int n = o->nbf;
  const tri_t * t = o->tri;
  tri_t * f = o->tri;
  tlk_t * l = o->tlk;

  if (!l) {
    return;
  }

  DRAW_SET_FLAGS(o->flags);
  hw->addcol = 0;
  hw->col = argb255(color);

  while (n--) {
    if (!(f->flags&1)) {
      /* backface (only projected face were flaged...) */
      draw_clip_znear_tri(f,l,t,transform,vp);
    }
    ++f;
    ++l;
  }
  return;
}

int DrawObject(viewport_t * vp, matrix_t local, matrix_t proj,
	       obj_t *o,
	       const vtx_t * ambient, 
	       const vtx_t * diffuse,
	       const vtx_t * light)
{
  matrix_t mtx;
  int flags;

  /* Check */
  if (!o) {
    return -1;
  }

  /* Init transformed vertex buffer */
  if (init_transform(o->nbv)) {
    return -1;
  }

  /* Compute final matrix */
  MtxMult3(mtx,local,proj);


  /* $$$ Optimize : do the 2 (+proj?) following op in 1 function. */

  /* Transform vertrices */
  MtxVectorsMult(&transform->v.x, &o->vtx->x, mtx, o->nbv,
		 sizeof(*transform), sizeof(*o->vtx));
  /* Set clipping flags */
  flags = set_clipping_flags(transform, o->nbv);

  if ((flags & 07700)) {
    /* All vertrices are clipped in some direction. */
    return flags;
  }
  if (!(flags & 00020)) {
    /* No one is clipped in Znear */
    ProjectVtx(transform, o->nbv, vp);
    TestVisibleFace(o);
    if (SingleColor(o, diffuse) < 0) {
      return -1;
    }
  } else {
    oozNear = -(proj[2][2]/proj[3][2]);
    zNear = -(proj[3][2]/proj[2][2]);
    ProjectVtxNotClipped(transform, o->nbv, vp, 00020);
    TestVisibleFaceNotClipped(o, 00020);
    draw_clip_znear_object(o, vp, diffuse);
  }


  return flags;
}
