/**
 * $Id: draw_object.c,v 1.5 2002-09-27 03:20:20 benjihan Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <dc/fmath.h>
#include <dc/ta.h>

#include "sysdebug.h"
#include "draw_object.h"

extern int bordertex, bordertex2;

typedef struct {
  float u;
  float v;
} uv_t;

typedef struct s_v_t {
  int flags;
  float x,y,z,u,v;
  unsigned int col, addcol;
} v_t;

static volatile v_t * const hw = (volatile v_t *)(0xe0<<24);
static matrix_t mtx;

static vtx_t * transform = 0;
static int transform_sz = 0;

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

static int sature(const float a)
{
  int v;

  v = (int)a;
  v = v & ~(v>>31);
  v = v | (((255-v)>>31) & 255);
  return v;
}

static unsigned int argb255(const vtx_t *color)
{
  return
    (sature(color->w*255.0f) << 24) |
    (sature(color->x*255.0f) << 16) |
    (sature(color->y*255.0f) << 8)  |
    (sature(color->z*255.0f) << 0);
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

static int init_transform(int nb)
{
  vtx_t *t;

  if (nb <= transform_sz) {
    return 0;
  }

  t = (vtx_t *)realloc(transform, nb * sizeof(vtx_t));
  if (!t) {
    return -1;
  }
  transform = t;
  transform_sz = nb;
  return 0;
}

static void TransformVtx(vtx_t * d, const obj_t *o,
			 const viewport_t *vp, matrix_t m)
{
  int n = o->nbv;

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

  const vtx_t * v = o->vtx;

  // $$$ Optimize with special opcode
  do { 
    const float x = v->x, y = v->y, z = v->z; 
    const float oow = 1.0f / ((x * m03 + y * m13 + z * m23) + m33);

    d->x = ((x * m00 + y * m10 + z * m20) + m30) * oow * mx + tx;
    d->y = ((x * m01 + y * m11 + z * m21) + m31) * oow * my + ty;
    d->z = 1.0f / ((x * m02 + y * m12 + z * m22) + m32);
    ++d; 
    ++v; 
  } while (--n);
}

static void TestVisibleFace(const obj_t * o)
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
    ++f;
  } while(--n);
}

int DrawObjectPostProcess(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o)
{
 if (!vp || !o) {
    return -1;
  }

 if (!o->flags) {
   SDWARNING("3D-object [%s] has no texture-id, set default\n", o->name);
   o->flags = bordertex2;
 }

 if (init_transform(o->nbv) < 0) {
    return -1;
  }

  MtxCopy(mtx, local);
  MtxMult(mtx, proj);

  TransformVtx(transform, o, vp, mtx);
  TestVisibleFace(o);

  return 0;
}

int DrawObjectSingleColor(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o, vtx_t *color)
{
  poly_hdr_t poly;
  unsigned int col;

  if (DrawObjectPostProcess(vp, local, proj, o) < 0) {
    return -1;
  }
  col = argb255(color);

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;
    
    ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 64, 64, o->flags,
		    TA_NO_FILTER);

    poly.flags1 &= ~(3<<4);
    ta_commit_poly_hdr(&poly);

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
  return 0;
}

int DrawObjectLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
		      vtx_t *ambient, vtx_t *light, vtx_t *diffuse)
{
  poly_hdr_t poly;

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
    
    ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 64, 64, o->flags,
		  TA_NO_FILTER);

    poly.flags1 &= ~(3<<4);
    ta_commit_poly_hdr(&poly);

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

      if (coef < 0) {
	coef = 0;
      }
      hw->col = argb4(aa + coef * la, ar + coef * lr,
		      ag + coef * lg, ab + coef * lb);
	
      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      // Strong link :
      //lflags |= l->flags & 7;
	
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
  return 0;
}

int DrawObjectFrontLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
		      vtx_t *ambient, vtx_t *diffuse)
{
  poly_hdr_t poly;

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
    
    ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 64, 64, o->flags,
		  TA_NO_FILTER);

    poly.flags1 &= ~(3<<4);
    ta_commit_poly_hdr(&poly);

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
      if (coef < 0) {
	coef = 0;
      }
      hw->col = argb4(aa + coef * la, ar + coef * lr,
		      ag + coef * lg, ab + coef * lb);
	
      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      // Strong link :
      //lflags |= l->flags & 7;
	
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
  return 0;
}


int DrawObjectPrelighted(viewport_t * vp, matrix_t local, matrix_t proj,
			 obj_t *o)
{
  poly_hdr_t poly;

  if (DrawObjectPostProcess(vp, local, proj, o) < 0) {
    return -1;
  }

  if (o->nbf) {
    const tri_t * t = o->tri;
    tri_t * f = o->tri;
    tlk_t * l = o->tlk;
    vtx_t * nrm = o->nvx;
    int     n = o->nbf;
    
    ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 64, 64, o->flags,
		  TA_NO_FILTER);

    poly.flags1 &= ~(3<<4);
    ta_commit_poly_hdr(&poly);

    hw->addcol = 0;
 
    if (!l) {
      // $$$ !
      return -1;
    }
    for(n=o->nbf ;n--; ++f, ++l, ++nrm)  {
      int lflags = 0;
      unsigned int cola, colb, colc, col;
      uv_t * uvl;

      if (f->flags) {
	continue;
      }

      col = *(int *)&nrm->w;
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

      lflags  = t[l->a].flags << 0;
      lflags |= t[l->b].flags << 1;
      lflags |= t[l->c].flags << 2;
      // Strong link :
      //lflags |= l->flags & 7;
	
      uvl = uvlinks[lflags];

      hw->col = /*col + */cola + colc;
      hw->flags = TA_VERTEX_NORMAL;
      hw->x = transform[f->a].x;
      hw->y = transform[f->a].y;
      hw->z = transform[f->a].z;
      hw->u = uvl[0].u;
      hw->v = uvl[0].v;
      ta_commit32_nocopy();

      //	hw->flags = TA_VERTEX;
      hw->col = /*col + */colb + cola;
      hw->x = transform[f->b].x;
      hw->y = transform[f->b].y;
      hw->z = transform[f->b].z;
      hw->u = uvl[1].u;
      hw->v = uvl[1].v;
      ta_commit32_nocopy();

      hw->col = /*col + */colc + colb;
      hw->flags = TA_VERTEX_EOL;
      hw->x = transform[f->c].x;
      hw->y = transform[f->c].y;
      hw->z = transform[f->c].z;
      hw->u = uvl[2].u;
      hw->v = uvl[2].v;
      ta_commit32_nocopy();
      
    }
  }
  return 0;
}
