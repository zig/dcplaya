/**
 * dreammp3 visual plugin - fftvlr
 *
 *
 * 2002/08/17
 * (C) COPYRIGHT 2002 Vincent Penne & Ben(jamin) Gerard
 */

#include <stdlib.h>
#include "fft.h"
#include "vis_driver.h"
#include "obj3d.h"

/* From dreamcast68.c */
extern void DrawObject(obj_t * o, matrix_t local, matrix_t proj,
		       const float z, const float a, const float light);

/* From obj3d.c */
void FaceNormal(float *d, const vtx_t * v, const tri_t *t);

/* Here are the constant (for now) parameters */
#define VLR_X 0.5f
#define VLR_Z 0.5f
#define VLR_W 32
#define VLR_H 48

/* Resulting number of triangles */
#define VLR_T ((VLR_W-1) * (VLR_H-1) * 2) 

/* VLR 3d-objects */
static vtx_t *v;   /* Vertex buffer */
static vtx_t *nrm; /* Normal buffer */
static tri_t *tri; /* Triangle buffer */
static tlk_t *tlk; /* Triangle link buffer */
static int idx; 

volatile int ready;

static matrix_t fftvlr_proj; /* Projection matrix */
static matrix_t fftvlr_mtx;  /* Local matrix */

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
  v   = (vtx_t *)malloc(sizeof(vtx_t) * (VLR_H*2)*VLR_W);
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
  idx = 0;

  /* Build vertrices double buffer (ring buffer)  */
  for(j=0, vy=v; j<VLR_H; ++j) {
    for (i=0; i<VLR_W; ++i, ++vy){
      vy->x = (i-VLR_W*0.5f) * (VLR_X / VLR_W);
      vy->y = (i/(float)VLR_W) * (j/(float)VLR_H);
      vy->z = (j-VLR_H*0.5f) * (VLR_Z / VLR_H);
      vy->w = 1.0f;
      vy[VLR_H*VLR_W] = vy[0];
    }
  }

  /* Build faces and link-faces */
  for(k=j=0; j<VLR_H-1; ++j) {
    for (i=0; i<VLR_W-1; ++i, k += 2) {
      const int tpl = (VLR_W-1)*2;

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

static void vlr_update(void)
{
  const int stp = ((1<<(FFT_LOG_2-1))<<12) / VLR_W;
  int i,j;
  const float f = 0.3f / 32000.0f;
  float f2 = f;
  vtx_t *vy;

  /* Scroll FFT towards Y axis */
  for (j=0, vy=v; j<VLR_H-1; ++j) {
    for (i=0; i<VLR_W; ++i, ++v) {
      vy->y = vy[VLR_W].y;
    }
  }
  memmove(nrm, nrm + 2*(VLR_W-1), sizeof(nrm[0])*2*(VLR_W-1) * (VLR_H-1-1));

  /* Update first VLR row with FFT data */
  vy = v + (VLR_H-1) * VLR_W;
  for (i=j=0; i<VLR_W; ++i, j += stp, f2 += f) {
    int w = fft_F[j>>12];
    const float r = 0.85f;
    
    if (w < 0) *(int*)1 = 0x1234; 

    vy[i].y = (float)(w * f2 * (1.0f-r)) +  (vy[i-VLR_W].y * r);
  }

  /* Face normal calculation */
  for (i=0; i<2*(VLR_W-1); i++) {
    FaceNormal(&nrm[(VLR_H-2)*2*(VLR_W-1)+i].x, v,
               tri+(VLR_H-2)*2*(VLR_W-1)+i);

  }
}

static int fftvlr_stop(void)
{
  ready = 0;
  fftvlr_free();
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

static int fftvlr_process(matrix_t projection, int elapsed_ms)
{
  if (ready) {
    MtxCopy(fftvlr_mtx, projection);
    vlr_update();
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
  if (ready) {
    DrawObject(&fftvlr_obj , fftvlr_mtx, 0, 80.0f, 0.90f, 0.0f);
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
