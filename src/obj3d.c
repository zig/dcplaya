/**
 * @file    obj3d.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/02/12
 * @brief   Very simple 3D API.
 *
 * @version $Id: obj3d.c,v 1.11 2003-03-10 22:55:35 ben Exp $
 */

#include <stdio.h>
#include <malloc.h>

#include "dcplaya/config.h"
#include "math_float.h"
#include "sysdebug.h"
#include "obj_driver.h"
#include "obj3d.h"
#include "draw/ta.h"
#include "matrix.h"
#include "border.h"

static void swap (int *a, int *b) {
  int tmp = *a;
  *a = *b;
  *b = tmp;
}

static float FaceCosAngle(obj_t *o, int ia, int ib)
{
  return vtx_dot_product(o->nvx + ia, o->nvx + ib);
}

void FaceNormal(float *d, const vtx_t * v, const tri_t *t)
{
  const vtx_t *a=v+t->a,*b=v+t->b,*c=v+t->c;
  vtx_t A,B,*D = (vtx_t*)d;

  vtx_sub3(&A,a,b);
  vtx_sub3(&B,a,c);
  vtx_cross_product3(D,&A,&B);
  vtx_normalize(D);
}


static void ReverseFaces(obj_t *o)
{
  if (o) {
    int i;
    for (i=0; i<o->nbf; ++i) {
      if (o->tri) {
	swap (&o->tri[i].a, &o->tri[i].b);
      }
      if (o->tlk) {
	swap (&o->tlk[i].b, &o->tlk[i].c);
	o->tlk[i].flags = ((o->tlk[i].flags&1))
	  | ((o->tlk[i].flags&2)<<1)
	  | ((o->tlk[i].flags&4)>>1);
      }
    }
  }
}

static void ResizeAndCenter(obj_t *o, const float w)
{
  float m[2][3];
  float c[3];
  float s;
  int i,j;

  if (o->nbv != o->static_nbv || o->nbf+1 != o->static_nbf) {
    // !!! Problem in genenerated object file
    SDCRITICAL("[%s] : Problem in genenerated object file : "
	       "[%d != %d] and/or [%d != %d]\n",
	       o->nbv, o->static_nbv, o->nbf+1, o->static_nbf);
    BREAKPOINT(0xDEAD9872);
  }

  if (o->flags) {
    return;
  }
  o->flags = 1;

  // Set W
  for (i=0; i<o->nbv; ++i) {
    o->vtx[i].w = 1.0f;
  }

  // Set first min-max
  for (j=0; j<3; ++j) {
    m[0][j] =
    m[1][j] = (&o->vtx[0].x)[j];
  }

  // Scans others
  for (i=1; i<o->nbv; ++i) {
    for (j=0; j<3; ++j) {
      const float v = (&o->vtx[i].x)[j];
      if (v < m[0][j]) {
        m[0][j] = v;
      } else if (v > m[1][j]) {
        m[1][j] = v;
      }
    }
  }

  // Calculates center
  for (j=0; j<3; ++j) {
    c[j] = 0.5f * (m[0][j] + m[1][j]);
  }

  // Calculates scale factor
  s = (m[1][0] - m[0][0]);
  for (j=1; j<3; ++j) {
    float s2 = (m[1][j] - m[0][j]);
    if (s2 > s) {
      s = s2;   
    }
  }
  if (s > MF_EPSYLON) {
    s = w / s;
  } else {
    s = 1;
  }

  // Transform all
  {
    matrix_t mtx,mtx2;
    MtxIdentity(mtx);
    MtxTranslate(mtx, -c[0], -c[1], -c[2]);
    MtxScale3(mtx, s,s,s);
    MtxIdentity(mtx2);
    mtx2[1][1] = mtx2[2][2] = 0;
    mtx2[2][1] = mtx2[1][2] = -1;
    MtxMult(mtx,mtx2);
    
    MtxVectorsMult(&o->vtx->x, &o->vtx->x, mtx, o->nbv,
      sizeof(*o->vtx),sizeof(*o->vtx));
  }
/*   for (i=0; i<o->nbv; ++i) { */
/*     float tmp; */
/*     for (j=0; j<3; ++j) { */
/*       (&o->vtx[i].x)[j] = ((&o->vtx[i].x)[j] - c[j]) * s; */
/*     } */
/*     tmp = o->vtx[i].z; */
/*     o->vtx[i].z = o->vtx[i].y; */
/*     o->vtx[i].y = tmp; */
/*   } */

  // Inverse face def
  if (0) {
    ReverseFaces(o);
  }
}

static int BuildNormals(obj_t *o)
{
  int i;

  /* One more for safety-net in lighted object. */
  if (!o->nvx) {
    o->nvx = (vtx_t *)malloc(sizeof(vtx_t) * (o->nbf+1));
  }
  if (!o->nvx) {
    return -1;
  }
  for (i=0; i<o->nbf; ++i) {
    FaceNormal(&o->nvx[i].x, o->vtx, o->tri+i);
  }
  o->nvx[i].x = o->nvx[i].y = o->nvx[i].z = o->nvx[i].w = 0;
  return 0;
}

int obj3d_build_normals(obj_t *o)
{
  if (!o) return -1;
  return BuildNormals(o);
}

static void BuildLinks(obj_t *o)
{
  int i;
  
  if (!o->tlk) {
    return;
  }
  
  /* Make unlinked point to special added faces : -1:invisible -2:visible. */
  for (i=0; i<o->nbf+1; ++i) {
    int * tlk = &o->tlk[i].a;
    int j;
    for (j=0;j<3;++j) {
      if (tlk[j] == -1) {
	tlk[j] = o->nbf;
      } else if (tlk[j] == 2) {
	tlk[j] = i; /* visible points on itself. */
      }
    }
  }
  
  /* Build special faces (invisible) */
  o->tri[o->nbf].flags = 1;
  o->tri[o->nbf].a = o->tri[o->nbf].b = o->tri[o->nbf].c = 0;
  
  /* Build strong (not smooth) links */
  for (i=0; i<o->nbf; ++i) {
    int j;
    
    o->tlk[i].flags = 0;
    for (j=0; j<3; ++j) {
      int k = (&o->tlk[i].a)[j]; 
      int res = 1;
      
      if ( (unsigned int)k < (unsigned int)o->nbf) {
        float cos_v = FaceCosAngle(o,i,k);
        res = (cos_v <= 0.59f);
      }
      o->tlk[i].flags |= (res << j);
    }
  }

}

static void PrepareObject(obj_t *o)
{
  SDDEBUG("%s vtx:%d/%d tri:%d/%d\n",
    o->name, o->nbv, o->static_nbv, o->nbf, o->static_nbf);
  
  ResizeAndCenter(o, 1.0f);
  BuildNormals(o);
  BuildLinks(o);
  // $$$ BEN : Add this test here !
  obj3d_verify(o);
}

static int verify_vertrices(obj_t * o)
{
  int i,j;

  if (o->nbv <= 0) {
    SDDEBUG("- Weird number of vertex [%d]\n", o->nbv);
    return -1;
  }
  if (!o->vtx) {
    SDDEBUG("- No vertrice buffer\n");
    return -1;
  }
  for (i=0; i<o->nbv; ++i) {
    vtx_t * v = o->vtx + i;
    float * w = &v->x;
    if (v->w != 1) {
      SDDEBUG(" - vtx #%d, W coordinate [%f != 1]\n", i, v->w);
      return -1;
    }
    for (j=0; j<3; ++j) {
      float a = Fabs(w[j]);
      if (a > 1000.0f) {
	SDDEBUG(" - vtx %d, %c coordinate large value [%f]\n", i, 'X'+j, a);
	return -1;
      }
    }
  }
  SDDEBUG("- %d vertrices [PASSED]\n", i);
  return 0;
}

static int verify_triangles(obj_t * o)
{
  int i,j,nbf;

  if (o->nbf <= 0) {
    SDDEBUG("- Weird number of face [%d]\n", o->nbf);
    return -1;
  }
  if (!o->tri) {
    SDDEBUG("- No triangle buffer\n");
    return -1;
  }
  nbf = o->nbf + (o->tlk != 0);
  for (i=0; i<nbf; ++i) {
    tri_t * t = o->tri + i;
    int * w = &t->a;
    for (j=0; j<3; ++j) {
      if ((unsigned int)w[j] >= (unsigned int)o->nbv) {
	SDDEBUG("- face %d, vertex %c : out of range [%d,%d]\n",
		i, 'A'+j, w[j], o->nbv);
	return -1;
      }
    }
  }
  SDDEBUG("- %d triangles [PASSED]\n", i);
  return 0;
}

static int verify_links(obj_t * o)
{
  int i,j;
  tlk_t * t;

  if (o->nbf <= 0) {
    SDDEBUG("- Weird number of face [%d]\n", o->nbf);
    return -1;
  }
  if (!o->tlk) {
    SDDEBUG("- No links buffer\n");
    return -1;
  }

  for (i=0, t=o->tlk; i<o->nbf; ++i, ++t) {
    int * w = &t->a;
    for (j=0; j<3; ++j) {
      if ((unsigned int)w[j] > (unsigned int)o->nbf) {
	SDDEBUG("- face %d, link %c : out of range [%d,%d]\n",
		i, 'A'+j, w[j], o->nbf);
	return -1;
      }
    }
  }

  {
    int * w = &t->a;
    for (j=0; j<3; ++j) {
      if ((unsigned int)w[j] != i) {
	SDDEBUG("- face %d, link %c : not on itself [%d,%d]\n",
		i, 'A'+j, w[j], i);
	return -1;
      }
    }
    ++i;
  }

  SDDEBUG("- %d link [PASSED]\n", i);
  return 0;
}

static int verify_normals(obj_t * o)
{
  int i;

  if (!o->nvx) {
    SDDEBUG("- normal [SKIPPED]\n");
    return 0;
  }
  if (o->nbf <= 0) {
    SDDEBUG("- Weird number of face [%d]\n", o->nbf);
    return -1;
  }
  for (i=0; i<o->nbf; ++i) {
    float n = vtx_norm(o->nvx+i);
    if (Fabs(1.0-n) > MF_EPSYLON) {
      SDDEBUG("- normal #%d : weird lenght [%f]\n", i, n);
      return -1;
    }
  }
  SDDEBUG("- normal [PASSED]\n");
  
  return 0;
}

int obj3d_reverse_faces(obj_t *o)
{
  if (!o) {
    return -1;
  }
  ReverseFaces(o);
  return 0;
}

int obj3d_verify(obj_t *o)
{
  int err = 0;
  if (!o) {
    return -1;
  }
  SDDEBUG("[obj3d_verify] : [%s]\n", o->name);
  SDINDENT;
  err = err || verify_vertrices(o);
  err = err || verify_triangles(o);
  err = err || verify_links(o);
  err = err || verify_normals(o);
  SDUNINDENT;
  SDDEBUG("[obj3d_verify] : [%s] : [%s]\n", o->name, !err ? "OK" : "FAILED");
  return -(!!err);
}

int obj3d_shutdown(any_driver_t * driver)
{
  /* $$$ Free allocated buffer here */
  return 0;
}

int obj3d_init(any_driver_t * driver)
{
  obj_driver_t * d;
  if (!driver || driver->type != OBJ_DRIVER) {
    return -1;
  }
  d = (obj_driver_t *)driver;
  d->obj.name = d->common.name;
  PrepareObject(&d->obj);
  return 0;
}

driver_option_t * obj3d_options(any_driver_t* driver, int idx,
                                driver_option_t * opt)
{
  return opt;
}
