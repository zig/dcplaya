/**
 * $Id: draw_object.c,v 1.16 2003-01-25 11:37:44 ben Exp $
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
  vtx_t v;
  struct {
    float x,y,z;
    int flags;
  } p;
} tvtx_t;

static tvtx_t * transform = 0;
static int transform_sz = 0;

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
			    const float tx, const float ty)
{
  const float oow = Inv(d->v.w); 
  d->p.x = d->v.x * oow * mx + tx;
  d->p.y = d->v.y * oow * my + ty;
  d->p.z = Inv(d->v.z);
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
    proj_vtx(d, mx, my, tx, ty);
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
      proj_vtx(d, mx, my, tx, ty);
    }
    ++d;
  } while (--n);
}


static void TestVisibleFace(const obj_t * o)
{
  tri_t * f = o->tri;
  int     n = o->nbf;
  do {
    const vtx_t *t0 = (const vtx_t *)&transform[f->a].p;
    const vtx_t *t1 = (const vtx_t *)&transform[f->b].p;
    const vtx_t *t2 = (const vtx_t *)&transform[f->c].p;

    const float a = t1->x - t0->x;
    const float b = t1->y - t0->y;
    const float c = t2->x - t0->x;
    const float d = t2->y - t0->y;

    const float sens =  a * d - b * c;

    f->flags = (sens < 0);

    ++f;
  } while(--n);
}

static void TestVisibleFaceNotClipped(const obj_t * o)
{
  tri_t * f = o->tri;
  int     n = o->nbf;
  do {
    const tvtx_t *t0 = &transform[f->a];
    const tvtx_t *t1 = (const tvtx_t *)&transform[f->b];
    const tvtx_t *t2 = (const tvtx_t *)&transform[f->c];
    int flags;

    flags = 0
      | (t0->p.flags << 8)
      | (t1->p.flags << (8+6))
      | (t2->p.flags << (8+12));

    if (!flags) {
      const float a = t1->p.x - t0->p.x;
      const float b = t1->p.y - t0->p.y;
      const float c = t2->p.x - t0->p.x;
      const float d = t2->p.y - t0->p.y;
      const float sens =  a * d - b * c;
      flags = (sens < 0);
    }      
    f->flags = flags;
    ++f;
  } while(--n);
}




typedef int cl_t;

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
      int lflags = 0;
      uv_t * uvl;

      if (f->flags) {
	continue;
      }

      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      // Strong link :
      //lflags |= l->flags & 7;
	
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
      int lflags = 0;
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
      // Strong link :
      //lflags |= l->flags & 7;
	
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
  const int strong_mask = 7;

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
      int lflags = 0;
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
      // Strong link :
      lflags |= l->flags & strong_mask;
	
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
      int lflags = 0;
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
      // Strong link :
      //lflags |= l->flags & 7;
	
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
			    const tvtx_t * transform)
{
  const int strong_mask = 7;
  int lflags;
  uv_t * uvl;
  
  if (t->flags) {
    /* backface */
    return;
  }

  lflags  = (1&t[l->a].flags) << 0;
  lflags |= (1&t[l->b].flags) << 1;
  lflags |= (1&t[l->c].flags) << 2;
  lflags |= l->flags & 7 & strong_mask; // Strong link
  uvl = uvlinks[lflags];

  hw->flags = TA_VERTEX_NORMAL;
  hw->x = transform[f->a].p.x;
  hw->y = transform[f->a].p.y;
  hw->z = transform[f->a].p.z;
  hw->u = uvl[0].u;
  hw->v = uvl[0].v;
  ta_commit32_nocopy();

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



static void draw_clip_znear_tri(const tri_t * f, /* triangle to draw */
				const tlk_t * l, /* link to draw     */
				const tri_t * t, /* triangle buffer  */ 
				const tvtx_t * transform,
				viewport_t * vp)
{
  const tvtx_t *va, *vb, *vc;
  int flags = f->flags;

  if (flags&1) {
    /* backface */
    return;
  }
  
  flags = 0
    | (1 & (flags >> (8+4)))
    | (2 & (flags >> (8+4+6-1)))
    | (4 & (flags >> (8+4+12-2)));
  if (!flags) {
    draw_tri_single(f,l,t,transform);
    return;
  }
  if (flags == 7) {
    return;
  }
  
/*   va = transform + f->a; */
/*   vb = transform + f->b; */
/*   vc = transform + f->c; */


  switch (flags) {
  case 1:
    /* A out */
  case 2:
    /* B out */
  case 3:
    /* C in */
  case 4:
    /* C out */
  case 5:
    /* B in */
  case 6:
  default:
    ;
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
    draw_clip_znear_tri(f,l,t,transform,vp);
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
  if (init_transform(o->nbv + 16)) {
    return -1;
  }

  /* Compute final matrix */
  MtxMult3(mtx,local,proj);

  /* Transform vertrices */
  MtxVectorsMult(&transform->v.x, &o->vtx->x, mtx, o->nbv,
		 sizeof(*transform), sizeof(*o->vtx));
  /* Set clipping flags */
  flags = set_clipping_flags(transform, o->nbv);

  if ((flags & 07700)) {
    /* All vertrices are clipped in some direction. */
    return flags;
  }
  if (!(flags & 0020)) {
    /* No one is clipped in Znear */
    ProjectVtx(transform, o->nbv, vp);
    TestVisibleFace(o);
    if (SingleColor(o, diffuse) < 0) {
      return -1;
    }
  } else {
    ProjectVtxNotClipped(transform, o->nbv, vp, 0020);
    TestVisibleFaceNotClipped(o);
    draw_clip_znear_object(o, vp, diffuse);
  }

  return flags;
}
