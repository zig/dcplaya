/* 2002/02/12*/

#include <stdio.h>
#include <math.h>
#include <malloc.h>
#include "obj_driver.h"
#include "obj3d.h"
#include "matrix.h"

static void swap (int *a, int *b) {
  int tmp = *a;
  *a = *b;
  *b = tmp;
}


static float FaceCosAngle(obj_t *o, int ia, int ib)
{
  vtx_t *va = o->nvx + ia;
  vtx_t *vb = o->nvx + ib;
  
  return va->x * vb->x + va->y * vb->y + va->z * vb->z;
}

void FaceNormal(float *d, const vtx_t * v, const tri_t *t)
{
  const vtx_t *a=v+t->a,*b=v+t->b,*c=v+t->c;
  float n;
  vtx_t A,B;

  A.x = a->x - b->x;
  A.y = a->y - b->y;
  A.z = a->z - b->z;

  B.x = a->x - c->x;
  B.y = a->y - c->y;
  B.z = a->z - c->z;

  CrossProduct(d,&A.x,&B.x);
  //n = frsqrt((d[0]*d[0] + d[1]*d[1] + d[2]*d[2]));
  n = 1.0f / sqrt((d[0]*d[0] + d[1]*d[1] + d[2]*d[2]));
  d[0] *= n;
  d[1] *= n;
  d[2] *= n;
}

static void ResizeAndCenter(obj_t *o, const float w)
{
  float m[2][3];
  float c[3];
  float s;
  int i,j;

/*    dbglog( DBG_DEBUG, ">> " __FUNCTION__ "\n"); */

  if (o->nbv != o->static_nbv || o->nbf+1 != o->static_nbf) {
    // !!! Problem in genenerated object file
    //BREAKPOINT68;
    *(int*)1 = 0x12345678;
  }

  if (o->flags&1) {
    return;
  }
  o->flags |= 1;

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
  s = w / (m[1][0] - m[0][0]);
  for (j=1; j<3; ++j) {
    float s2 = w / (m[1][j] - m[0][j]);
    if (s2 < s) {
      s = s2;                                    }
  }

  // Transform all
  for (i=0; i<o->nbv; ++i) {
    float tmp;
    for (j=0; j<3; ++j) {
      (&o->vtx[i].x)[j] = ((&o->vtx[i].x)[j] - c[j]) * s;
    }
    tmp = o->vtx[i].z;
    o->vtx[i].z = o->vtx[i].y;
    o->vtx[i].y = tmp;
  }

  // Inverse face def
  for (i=0; i<o->nbf; ++i) {
    swap (&o->tri[i].a, &o->tri[i].b);
    swap (&o->tlk[i].b, &o->tlk[i].c);
    o->tlk[i].flags = ((o->tlk[i].flags&1))
      | ((o->tlk[i].flags&2)<<1)
      | ((o->tlk[i].flags&4)>>1);
  }
}

static void BuildNormals(obj_t *o)
{
  int i;

/*    dbglog( DBG_DEBUG, ">> " __FUNCTION__ "\n"); */
  if (!o->nvx) {
    o ->nvx = (vtx_t *)malloc(sizeof(vtx_t) * o->nbf);
  }
  if (!o->nvx) {
    return;
  }
  for (i=0; i<o->nbf; ++i) {
    
    FaceNormal((float *)(o->nvx+i),o->vtx,o->tri+i);
/*    o->nvx.x = - o->nvx.x;
    o->nvx.y = - o->nvx.y;
    o->nvx.z = - o->nvx.z;*/
  }
/*    dbglog( DBG_DEBUG, "<< " __FUNCTION__ "\n"); */
}

static void BuildLinks(obj_t *o)
{
  int i;
  
/*    dbglog( DBG_DEBUG, ">> " __FUNCTION__ "\n"); */
  if (!o->tlk) {
    dbglog( DBG_DEBUG, "<< " __FUNCTION__ " : No links\n");
    return;
  }
  
  /* Make unlinked point to special added faces */
  for (i=0; i<o->nbf; ++i) {
    if (o->tlk[i].a < 0) o->tlk[i].a = o->nbf;
    if (o->tlk[i].b < 0) o->tlk[i].b = o->nbf;
    if (o->tlk[i].c < 0) o->tlk[i].c = o->nbf;
  }
  
  /* Build the special face */
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
/*    dbglog( DBG_DEBUG, "<< " __FUNCTION__ "\n"); */
  
}

static void PrepareObject(obj_t *o)
{
  dbglog( DBG_DEBUG, ">> " __FUNCTION__ " : %s vtx:%d/%d tri:%d/%d\n",
    o->name, o->nbv, o->static_nbv, o->nbf, o->static_nbf);
  
  ResizeAndCenter(o, 1.0f);
  BuildNormals(o);
  BuildLinks(o);
  
  dbglog( DBG_DEBUG, "<< " __FUNCTION__ " : %s\n", o->name);
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
