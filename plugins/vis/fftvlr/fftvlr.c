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
 * $Id: fftvlr.c,v 1.10 2002-09-19 01:32:46 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "fft.h"
#include "vis_driver.h"
#include "obj3d.h"
#include "draw_object.h"

/* from border.c */
extern int bordertex, bordertex2;

/* From obj3d.c */
void FaceNormal(float *d, const vtx_t * v, const tri_t *t);

/* Here are the constant (for now) parameters */
#define VLR_X 0.5f
#define VLR_Z 0.5f
#define VLR_Y (4.5f)
#define VLR_W 32
#define VLR_H 48
/*#define VLR_W 32
#define VLR_H 96*/

/* Resulting number of triangles */
#define VLR_TPL ((VLR_W-1)*2)
#define VLR_T   (VLR_TPL * (VLR_H-1))

/* VLR 3d-objects */
static vtx_t *v;   /* Vertex buffer */
static vtx_t *nrm; /* Normal buffer */
static tri_t *tri; /* Triangle buffer */
static tlk_t *tlk; /* Triangle link buffer */

volatile int ready;

static viewport_t viewport;  /* Graphic viewport */
static matrix_t fftvlr_proj; /* Projection matrix */
static matrix_t fftvlr_mtx;  /* Local matrix */

static vtx_t light_normal = { 
  0.8,
  0.4,
  0.5
};

static vtx_t tlight_normal;

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


/* static vtx_t light_color = { */
/*     1.5, */
/*     1.0, */
/*     0.5, */
/*     -0.3 */
/* }; */

/* static vtx_t ambient_color = { */
/*   0, */
/*   0, */
/*   0, */
/*   0.8 */
/* }; */



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
  nrm = (vtx_t *)malloc(sizeof(vtx_t) * (VLR_T));
  tri = (tri_t *)malloc(sizeof(tri_t) * (VLR_T+1)); /* +1 for invisible */
  tlk = (tlk_t *)malloc(sizeof(tlk_t) * (VLR_T));
  if (!v || !nrm || !tri || !tlk) {
    fftvlr_free();
    return -1;
  }
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
  tri[k].flags = 1; /* Setup invisible face */

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
  v = v & ~(v>>31);
  v = v | (((255-v)>>31) & 255);
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
  const float f0 = VLR_Y / 32768.0f;
  vtx_t *vy;
  int i,j,k,stp, ow;

  /* Scroll FFT towards Z axis */
  for (i=0,vy=v; i<VLR_W*(VLR_H-1); ++i, ++vy) {
      vy->y = vy[VLR_W].y;
  }
  /* Scroll Normals and colors (norm.w) */
  memmove(nrm, nrm + VLR_TPL, sizeof(nrm[0]) * (VLR_T - VLR_TPL));

  /* Update first VLR row with FFT data */
  vy = v + (VLR_H-1) * VLR_W;
  for (ow=i=j=k=0, stp=(1<<(FFT_LOG_2-1+12))/VLR_W; i<VLR_W; ++i, j += stp) {
    //    int w = fft_F[j>>12];
    const float r = 0.45f;
    //const float r = 0.75f;
    int w = 0;
    int n = 0;
    int l = (j>>12);
    while (k<l) {
      w += fft_F[k];
      k++;
      n++;
    }
    if (n) {
      w /= n;
      ow = w;
    } else
      w = ow;

    vy[i].y = (float)(w * f0 * (1.0f-r)) +  (vy[i-VLR_W].y * r);
  }


  {
    const float la = light_color.w * 255.0f, aa = ambient_color.w * 255.0f;
    const float lr = light_color.x * 255.0f, ar = ambient_color.x * 255.0f;
    const float lg = light_color.y * 255.0f, ag = ambient_color.y * 255.0f;
    const float lb = light_color.z * 255.0f, ab = ambient_color.z * 255.0f;

    /* Face normal calculation  and lightning */
    for (i=0; i<VLR_TPL; i++) {
      vtx_t *n = &nrm[(VLR_H-2)*VLR_TPL+i];
      float coef;
      
      FaceNormal(&n->x, v,
		 tri+(VLR_H-2)*VLR_TPL+i);
      
      coef = n->x * tlight_normal.x 
	+ n->y * tlight_normal.y 
	+ n->z * tlight_normal.z;

      if (coef < 0) {
	coef = 0;
      }
      *(int*)&n->w = argb4(aa + coef * la, ar + coef * lr,
			   ag + coef * lg, ab + coef * lb);
      
    }
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
  ready = 0;
  v = 0;
  tri = 0;
  nrm = 0;
  tlk = 0;

  return 0;
}

static int fftvlr_shutdown(any_driver_t * d)
{
  ready = 0;
  fftvlr_free();
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
    fftvlr_mtx[3][2] = 0.6;

    MtxIdentity(tmp);
    MtxRotateZ(tmp, 3.14159);
    MtxRotateY(tmp, -0.33468713*ay);
    MtxRotateX(tmp, 0.4);
    MtxTranspose(tmp);
    MtxVectMult(&tlight_normal, &light_normal, tmp);

    vlr_update();


    fftvlr_obj.flags = bordertex2;
    return 0;
  }
  return -1;
}

static int fftvlr_opaque(void)
{
  return 0;
}

static int fftvlr_transparent(void)
{
  static vtx_t color = {
    80.0f, 0.90f, 0.0f, 0.4f
  };
  if (ready) {
/*     DrawObjectSingleColor(&viewport, fftvlr_mtx, fftvlr_proj, */
/* 			  &fftvlr_obj, &color); */
    DrawObjectPrelighted(&viewport, fftvlr_mtx, fftvlr_proj,
			  &fftvlr_obj);

    return 0;
  } else {
    return -1;
  }
}

static driver_option_t * fftvlr_options(any_driver_t * d, int idx,
                                        driver_option_t * o)
{
  return o;
}

static vis_driver_t fftvlr_driver =
{

  /* Any driver */
  {
    0,                    /**< Next driver (see any_driver.h)  */
    VIS_DRIVER,           /**< Driver type                     */      
    0x0100,               /**< Driver version                  */
    "FFT-VLR",            /**< Driver name                     */
    "Vincent Penne\0"     /**< Driver authors                  */
    "Benjamin Gerard\0",
    "Virtual Landscape "  /**< Description                     */
    "Reality based on "
    "Fast Fourier "
    "Transform",
    0,                    /**< DLL handler                     */
    fftvlr_init,          /**< Driver init                     */
    fftvlr_shutdown,      /**< Driver shutdown                 */
    fftvlr_options,       /**< Driver options                  */
  },

  fftvlr_start,           /**< Start                           */
  fftvlr_stop,            /**< Stop                            */
  fftvlr_process,         /**< Process                         */
  fftvlr_opaque,          /**< Opaque render list              */
  fftvlr_transparent,     /**< Transparent render list         */
  
};

EXPORT_DRIVER(fftvlr_driver)
