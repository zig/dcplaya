#include <stdlib.h>
#include "fime_ground.h"
#include "draw_object.h"
#include "draw/vertex.h"
#include "draw/texture.h"
#include "sysdebug.h"

#include "fime_analysis.h"

static obj_t grd_obj;
static int grd_w, grd_h, grd_tpl;
static float grd_x, grd_y, grd_z;
static matrix_t mtx;
static texid_t texid;
static int fime_ground_analysis_result;

static void safe_free(void **ptr)
{
  if (*ptr) {
    free(*ptr);
    *ptr = 0;
  }
}

static void free_grd_object(void)
{
  safe_free((void**)&grd_obj.vtx);
  safe_free((void**)&grd_obj.tri);
  safe_free((void**)&grd_obj.tlk);
  safe_free((void**)&grd_obj.nvx);
}

static int alloc_grd_object(void)
{
  grd_obj.vtx = (vtx_t *)malloc(sizeof(vtx_t) * grd_obj.nbv);
  grd_obj.tri = (tri_t *)malloc(sizeof(tri_t) * (grd_obj.nbf+2));
  grd_obj.tlk = (tlk_t *)malloc(sizeof(tlk_t) * (grd_obj.nbf+2));
  grd_obj.nvx = (vtx_t *)malloc(sizeof(vtx_t) * (grd_obj.nbf+2));

  if (!grd_obj.vtx || !grd_obj.tri || !grd_obj.tlk || !grd_obj.nvx) {
    free_grd_object();
    return -1;
  }
  return 0;
}

static int build_grd_vtx(const float tx, const float sx,
			 const float ty, const float sy)
{
  int i,j;
  const int w = grd_w, h = grd_h;
  vtx_t * vy;

  for(j=0, vy=grd_obj.vtx; j<h; ++j) {
    for (i=0; i<w; ++i, ++vy){
      vy->x = (float)i * sx + tx;
      vy->y = 0;
      vy->z = (float)j * sy + ty;
      vy->w = 1.0f;
    }
  }
  return 0;
}

static int build_grd_topo(void)
{
  int i,j,k;
  tri_t * tri = grd_obj.tri;
  tlk_t * tlk = grd_obj.tlk;
  const int nbf = grd_obj.nbf;
  const int w = grd_w;
/*   const int h = grd_h; */

  /* Build faces and link-faces */
  for(k=j=0; j<grd_h-1; ++j) {
    for (i=0; i<grd_w-1; ++i, k += 2) {
/*       const int tpl = grd_tpl; */
      int a = j*w+i, b = a+1, c = a + w, d = b + w;

      tlk[k+0].flags = tri[k+0].flags = 0;
      tlk[k+1].flags = tri[k+1].flags = 0;

      tri[k+0].a = c;
      tri[k+0].b = b;
      tri[k+0].c = a;
	
      tri[k+1].a = b;
      tri[k+1].b = c;
      tri[k+1].c = d;

      tlk[k+0].a = k+1;
      tlk[k+1].a = k+0;
      
      tlk[k+0].b = nbf;
      tlk[k+1].b = nbf;

      tlk[k+0].c = nbf;
      tlk[k+1].c = nbf;
    }
  }
  /* Setup invisible face */
  tri[k].a = tri[k].b = tri[k].c = 0;
  tri[k].flags = 1;
  ++k;
  tlk[k].a = tlk[k].b = tlk[k].c = k;
  tlk[k].flags = 0;

  return 0;
}

int fime_ground_init(float w, float h, int nw, int nh)
{
  SDDEBUG("[fime] : ground init (%.2fx%.2f, %dx%d)\n",w,h,nw,nh);
  MtxIdentity(mtx);
  memset(&grd_obj,0,sizeof(grd_obj));
  fime_ground_analysis_result = 0;

  if (w<=0 || h<=0 || nw < 2 || nh < 2) {
    return -1;
  }

  texid = texture_get("fime_bordertile");
  if (texid < 0) {
    return -1;
  }

  grd_w = nw;
  grd_h = nh;
  grd_tpl = (grd_w-1)*2;
  
  grd_x = (float)w/(float)(nw-1);
  grd_y = (float)h/(float)(nh-1);

  grd_obj.nbv = grd_w * grd_h;
  grd_obj.nbf = grd_tpl * (grd_h-1);

  if (alloc_grd_object()) {
    return -1;
  }
  build_grd_vtx(-0.5 * (float)w, (float)w/(float)(nw-1),
		-1.3f , (float)h/(float)(nh-1));
  build_grd_topo();

  SDDEBUG("[fime] : ground %d vtx, %d faces, %d face/line\n",
	  grd_obj.nbv, grd_obj.nbf, grd_tpl);
  return 0;
}

void fime_ground_shutdown(void)
{
  free_grd_object();
}

extern int rand(void);

static void generate_grd_line(vtx_t *v, int nbv)
{
  int k=0;
  static float a = 0;
  const float pa = 0.4, pa2 = 0.53;
  float a2;
  vtx_t *w = v;
  
  a2 = a;

  k = rand() % (unsigned int)nbv;

  while (nbv--) {
    v->y = (Sin(a2) + 1.0) * 0.3;
    a2 += pa2;
    ++v;
  }


  if (fime_ground_analysis_result & 3) {

    w[k].y -= 0.20 * (float)(fime_ground_analysis_result & 3);
    fime_ground_analysis_result = 0;
  }

  a = Fmod(a + pa, 2*MF_PI);
}

void fime_ground_scroll(float scroll)
{
  float oz = grd_z;
  float z = oz + scroll;
  int i,oi;
  int lines,n,l;
  vtx_t *v,*ve;

  grd_z = Fmod(z, grd_y);

  oi = oz / grd_y;
  i = z / grd_y;

  lines = i - oi;

  if (lines <= 0) {
    return;
  }

  l = lines * grd_w;
  n = grd_obj.nbv - l;

  ve = grd_obj.vtx + grd_obj.nbv;
  /* Scroll  */
  for (v=grd_obj.vtx; v < ve-l; ++v) {
    v->y = v[l].y;
  }

  /* Generate */
  for ( ; v<ve; v += grd_w) {
    generate_grd_line(v, grd_w);
  }

  /* Scroll Normals and colors (norm.w) */
/*   v = grd_obj.nvx; */
/*   if (v) { */
/*     int l = lines * grd_tpl; */
/*     memmove(v, v + l, sizeof(*v) * (grd_obj.nbf - l)); */
/*   } */
}


int fime_ground_update(const float seconds)
{
  int err = 0;

  fime_ground_analysis_result |= fime_analysis_result;
  fime_ground_scroll(0.15);

  return -!!err;
}

static vtx_t grd_ambient = { 1,1,1,1 };
static vtx_t grd_diffuse = { 1,0.5,0.5,1 };

int fime_ground_render(viewport_t *vp, matrix_t camera, matrix_t proj,
		       int opaque)

{
  int err = 0;
  matrix_t tmtx;

  if (!opaque) {
    return 0;
  }

  grd_obj.flags = 0
    | DRAW_NO_FILTER
    | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (texid << DRAW_TEXTURE_BIT);

  MtxIdentity(mtx);
  MtxTranslate(mtx,0,0,-grd_z);

  MtxMult3(tmtx,mtx,camera);
  err = DrawObject(vp, tmtx, proj, &grd_obj, &grd_ambient, &grd_diffuse, 0);

  return -!!(err<0);
}
