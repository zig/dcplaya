/**
 * @file    3dutils.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/10/10
 * @brief   2D drawing primitives.
 *
 * $Id: 3dutils.c,v 1.8 2002-11-14 23:40:28 benjihan Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include "ta_defines.h"
#include "gp.h"
#include "texture.h"
#include "sysdebug.h"

#include "draw_clipping.h"

typedef struct {
  uint32 word1;
  uint32 word2;
  uint32 word3;
  uint32 word4;
} ta_poly_t;

/* $$$ shading mode not handled, since I don't know how it works with
   the TA. */
static const ta_poly_t poly_table[4] = {
  /* TO   texture     opacity                        */
  /* ----------------------------------------------- */
  /* 00   no         transparent                     */
  { 0x82840012, 0x90800000, 0x949004c0, 0x00000000 },
  /* 01   no         opaque                          */
  { 0x80840012, 0x90800000, 0x20800440, 0x00000000 },
  /* 10   yes        transparent                     */
  { 0x8284000a, 0x92800000, 0x949004c0, 0x00000000 },
  /* 11   yes        opaque                          */
  { 0x8084000a, 0x90800000, 0x20800440, 0x00000000 },
};

static ta_hw_poly_t cur_poly;

static const int draw_zwrite  = TA_ZWRITE_ENABLE;
static const int draw_ztest   = TA_DEPTH_GREATER;
static const int draw_culling = TA_CULLING_CCW;

static void make_poly_hdr(ta_hw_poly_t * poly, int flags)
{
  const ta_poly_t * p;
  texture_t * t;
  texid_t texid;
  int idx;
  uint32 word3, word4;

  t = 0;
  texid = DRAW_TEXTURE(flags);
  if (texid != DRAW_NO_TEXTURE) {
	t = texture_lock(texid);
  }

  idx = 0
	| ((t != 0) << 1)
	| ((flags >> DRAW_OPACITY_BIT) & 0x1);

  p = poly_table + idx;
  poly->word1 = p->word1;
  poly->word2 = p->word2;
  word3 = p->word3;
  word4 = 0;
  if (t) {
	/* $$$ does not match documentation !!!  */
	word3 |= ((DRAW_FILTER(flags)^DRAW_FILTER_MASK)) << (13-DRAW_FILTER_BIT);
	word3 |= (t->wlog2-3) << 3;
	word3 |= (t->hlog2-3);
	/* $$$ does not match documentation !! */
	word4 = (t->format<<26) | (t->ta_tex >> 3);

	texture_release(t);
  }
  poly->word3 = word3;
  poly->word4 = word4;
}

void draw_poly_hdr(ta_hw_poly_t * poly, int flags)
{
  make_poly_hdr(poly, flags);
}

static int sature(const float a)
{
  int v;

  v = (int) a;
  v = v & ~(v>>31);
  v = v | (((255-v)>>31) & 255);
  return v;
}

static unsigned int argb255(const draw_vertex_t * v)
{
  return
    (sature(v->a*255.0f) << 24) |
    (sature(v->r*255.0f) << 16) |
    (sature(v->g*255.0f) << 8)  |
    (sature(v->b*255.0f) << 0);
}


void draw_triangle_no_clip(const draw_vertex_t *v1,
						   const draw_vertex_t *v2,
						   const draw_vertex_t *v3,
						   int flags)
{
  make_poly_hdr(&cur_poly, flags);
  ta_commit32_inline(&cur_poly);

  if (DRAW_TEXTURE(flags) == DRAW_NO_TEXTURE) {
	/* No texture */
	volatile ta_hw_col_vtx_t * v = HW_COL_VTX;

	/* Vertex 1 */
	v->flags = TA_VERTEX_NORMAL;
	v->x = v1->x; v->y = v1->y; v->z = v1->z;
	v->a = v1->a; v->r = v1->r; v->g = v1->g; v->b = v1->b;
	ta_commit32_nocopy();

	/* Vertex 2 */
	v->x = v2->x; v->y = v2->y; v->z = v2->z;
	v->a = v2->a; v->r = v2->r; v->g = v2->g; v->b = v2->b;
	ta_commit32_nocopy();
	
	/* Vertex 3 */
	v->flags = TA_VERTEX_EOL;
	v->x = v3->x; v->y = v3->y; v->z = v3->z;
	v->a = v3->a; v->r = v3->r; v->g = v3->g; v->b = v3->b;
	ta_commit32_nocopy();

  } else {
	volatile ta_hw_tex_vtx_t  * v = HW_TEX_VTX;

	/* Vertex 1 */
	v->flags = TA_VERTEX_NORMAL;
	v->x = v1->x; v->y = v1->y; v->z = v1->z;
	v->u = v1->u; v->v = v1->v;
	v->col = argb255(v1);
	v->addcol = 0;
	ta_commit32_nocopy();

	/* Vertex 2 */
	v->x = v2->x; v->y = v2->y; v->z = v2->z;
	v->u = v2->u; v->v = v2->v;
	v->col = argb255(v2);
	ta_commit32_nocopy();
	
	/* Vertex 3 */
	v->flags = TA_VERTEX_EOL;
	v->x = v3->x; v->y = v3->y; v->z = v3->z;
	v->u = v3->u; v->v = v3->v;
	v->col = argb255(v3);
	ta_commit32_nocopy();
  }
}

void draw_strip_no_clip(const draw_vertex_t *vtx,
						int n, int flags)
{

  make_poly_hdr(&cur_poly, flags);
  ta_commit32_inline(&cur_poly);

  if (DRAW_TEXTURE(flags) == DRAW_NO_TEXTURE) {
	/* No texture */
	volatile ta_hw_col_vtx_t * const v = HW_COL_VTX;

	while (--n > 0) {
	  v->flags = TA_VERTEX_NORMAL;
	  v->x = vtx->x; v->y = vtx->y; v->z = vtx->z;
	  v->a = vtx->a; v->r = vtx->r; v->g = vtx->g; v->b = vtx->b;
	  ta_commit32_nocopy();
	}
	v->flags = TA_VERTEX_EOL;
	v->x = vtx->x; v->y = vtx->y; v->z = vtx->z;
	v->a = vtx->a; v->r = vtx->r; v->g = vtx->g; v->b = vtx->b;
	ta_commit32_nocopy();

  } else {
	/* Textured */
	volatile ta_hw_tex_vtx_t * const v = HW_TEX_VTX;

	v->flags = TA_VERTEX_NORMAL;
	while (--n > 0) {
	  v->x = vtx->x; v->y = vtx->y; v->z = vtx->z;
	  v->u = vtx->u; v->v = vtx->v;
	  v->col = argb255(vtx);
	  v->addcol = 0;
	  ta_commit32_nocopy();
	  ++vtx;
	}

	v->flags = TA_VERTEX_EOL;
	v->x = vtx->x; v->y = vtx->y; v->z = vtx->z;
	v->u = vtx->u; v->v = vtx->v;
	v->col = argb255(vtx);
	v->addcol = 0;
	ta_commit32_nocopy();
  }
}

/** Calculates clipping flags for a vertex. */
static int vertex_clip_flags(const draw_vertex_t *v)
{
  return
	((v->x < clipbox[0]) << 0) |
	((v->x > clipbox[2]) << 2) |
	((v->y < clipbox[1]) << 1) |
	((v->y > clipbox[3]) << 3);
}



void draw_triangle(const draw_vertex_t *v1,
				   const draw_vertex_t *v2,
				   const draw_vertex_t *v3,
				   int flags)
{
  int clipflags, andflags;

  /* Compute clipping flags */
  clipflags = vertex_clip_flags(v1);
  clipflags <<= 4;
  clipflags |= vertex_clip_flags(v2);
  clipflags <<= 4;
  clipflags |= vertex_clip_flags(v3);

  if (!clipflags) {
	/* No clipping, draw the triangle */
	draw_triangle_no_clip(v1, v2, v3, flags);
	return;
  }

  andflags = clipflags;
  andflags &= andflags>>4;
  andflags &= andflags>>4;
  if (andflags) {
	/* Outside clipping area */
	return;
  }

  /* Insert clipping flags into draw flags */
  flags = (flags & ~0xFFF) | clipflags;

  if (flags & 0x111) {
	/* Entering clipping stage with a LEFT clipping */
	draw_triangle_clip_any(v1,v2,v3,flags,0);
  } else if (flags & 0x444) {
	/* Entering clipping stage with a RIGHT clipping */
	draw_triangle_clip_any(v1,v2,v3,flags,2);
  } else if (flags & 0x222) {
	/* Entering clipping stage with a TOP clipping */
	draw_triangle_clip_any(v1,v2,v3,flags,1);
  } else if (flags & 0x888) {
	/* Entering clipping stage with a BOTTOM clipping */
	draw_triangle_clip_any(v1,v2,v3,flags,3);
  }
#ifdef DEBUG 
  else {
	SDCRITICAL("Something wrong here\n");
  }
#endif

}

void draw_triangle_indirect(const draw_vertex_t *v,
							int a, int b, int c, int flags)
{
  draw_triangle(v+a, v+b, v+c, flags);
}

void draw_triangles(const draw_vertex_t *v, int n, int flags)
{
  while (n--) {
	draw_triangle(v+0,v+1,v+2,flags);
	v+=3;
  }
}

void draw_triangles_indirect(const draw_vertex_t *v, const int * idx, int n,
							 int flags)
{
  while (n--) {
	int a = *idx;
	if (a < 0) return;
	draw_triangle(v+a,v+idx[1],v+idx[2],flags);
	idx+=3;
  }	
}

void draw_strip(const draw_vertex_t *v, int n, int flags)
{
  int i, andf, orrf;

  if (n<3) {
	return;
  }

  for (i=0, orrf=0, andf=0xF; i<n; ++i) {
	int f = vertex_clip_flags(v+i);
	andf &= f;
	orrf |= f;
  }

  /* Clipped out */
  if (andf) {
	return;
  }

  /* No clipping, let's go */
  if (!orrf) {
	draw_strip_no_clip(v,n,flags);
	return;
  }

  /* $$$ Just split the strip into triangles ! We can do better here. */
  for (i=0; i<n-2; ++i, ++v) {
	int j = i&1;
	draw_triangle(v, v+1+j, v-j+2, flags);
  }
}

/* Box points mapping
 * 12
 * 34
 */
void draw_box4(float x1, float y1, float x2, float y2, float z,
			   float a1, float r1, float g1, float b1,
			   float a2, float r2, float g2, float b2,
			   float a3, float r3, float g3, float b3,
			   float a4, float r4, float g4, float b4)
{
  float w,h;

  /* Clip out */
  if (x1 >= clipbox[2] || y1 >= clipbox[3] ||
	  x2 <= clipbox[0] || y2 <= clipbox[1]) {
	return;
  }

  /* Clip tiny or negative box */
  w = x2 - x1;
  h = y2 - y1;
  if (w < 1E-5 || h < 1E-5) {
	return;
  }

#define CLIPME(A,X,Y) A##X = A##Y * f1 + A##X * f2
  /* Left clip */
  if (x1 < clipbox[0]) {
	float f1 = (clipbox[0] - x1) / w;
	float f2 = 1.0f - f1;
	x1 = clipbox[0];
	CLIPME(a,1,2);
	CLIPME(r,1,2);
	CLIPME(g,1,2);
	CLIPME(b,1,2);
	CLIPME(a,3,4);
	CLIPME(r,3,4);
	CLIPME(g,3,4);
	CLIPME(b,3,4);
  }

  /* Top clip */
  if (y1 < clipbox[1]) {
	float f1 = (clipbox[1] - y1) / h;
	float f2 = 1.0f - f1;
	y1 = clipbox[1];
	CLIPME(a,1,3);
	CLIPME(r,1,3);
	CLIPME(g,1,3);
	CLIPME(b,1,3);
	CLIPME(a,2,4);
	CLIPME(r,2,4);
	CLIPME(g,2,4);
	CLIPME(g,2,4);
  }

  /* Right clip */
  if (x2 > clipbox[2]) {
	float f1 = (x2 - clipbox[2]) / w;
	float f2 = 1.0f - f2;
	x2 = clipbox[2];
	CLIPME(a,2,1);
	CLIPME(r,2,1);
	CLIPME(g,2,1);
	CLIPME(b,2,1);
	CLIPME(a,4,3);
	CLIPME(r,4,3);
	CLIPME(g,4,3);
	CLIPME(b,4,3);
  }
  /* Bottom clip */
  if (y2 > clipbox[3]) {
	float f1 = (y2 - clipbox[3]) / h;
	float f2 = 1.0f - f2;
	CLIPME(a,3,1);
	CLIPME(r,3,1);
	CLIPME(g,3,1);
	CLIPME(b,3,1);
	CLIPME(a,4,2);
	CLIPME(r,4,2);
	CLIPME(g,4,2);
	CLIPME(b,4,2);
  }

  {
	poly_hdr_t poly;
	vertex_oc_t vert;

	ta_poly_hdr_col(&poly, TA_TRANSLUCENT);
	ta_commit_poly_hdr(&poly);

	vert.flags = TA_VERTEX_NORMAL;
	vert.x = x1; vert.y = y2; vert.z = z;
	vert.a = a3; vert.r = r3; vert.g = g3; vert.b = b3;
	ta_commit_vertex(&vert, sizeof(vert));
	
	vert.y = y1;
	vert.a = a1; vert.r = r1; vert.g = g1; vert.b = b1;
	ta_commit_vertex(&vert, sizeof(vert));
	
	vert.x = x2; vert.y = y2;
	vert.a = a4; vert.r = r4; vert.g = g4; vert.b = b4;
	ta_commit_vertex(&vert, sizeof(vert));
	
	vert.flags = TA_VERTEX_EOL;
	vert.y = y1;
	vert.a = a2; vert.r = r2; vert.g = g2; vert.b = b2;
	ta_commit_vertex(&vert, sizeof(vert));
  }

}

void draw_box1(float x1, float y1, float x2, float y2, float z,
			   float a, float r, float g, float b)
{
  draw_box4(x1,y1,x2,y2,z, a,r,g,b, a,r,g,b, a,r,g,b, a,r,g,b);
}

void draw_box2h(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2)
{
  draw_box4(x1,y1,x2,y2,z, a1,r1,g1,b1, a2,r2,g2,b2, a1,r1,g1,b1, a2,r2,g2,b2);
}

/** Draw vertical gradiant box. */
void draw_box2v(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2)
{
  draw_box4(x1,y1,x2,y2,z, a1,r1,g1,b1, a1,r1,g1,b1, a2,r2,g2,b2, a2,r2,g2,b2);
}

/** Draw diagonal gradiant box. */
void draw_box2d(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2)
{
  float a,r,g,b;
  a = (a1 + a2) * 0.5f;
  r = (r1 + r2) * 0.5f;
  g = (g1 + g2) * 0.5f;
  b = (b1 + b2) * 0.5f;
  draw_box4(x1,y1,x2,y2,z, a1,r1,g1,b1, a,r,g,b, a,r,g,b, a2,r2,g2,b2);
}
