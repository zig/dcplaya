/**
 * @file    3dutils.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/10/10
 * @brief   2D drawing primitives.
 *
 * $Id: 3dutils.c,v 1.3 2002-10-16 23:59:50 benjihan Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include "gp.h"

#include "draw_clipping.h"

// #if 0
//   struct poly_hd {
// 	struct command {
// 	  int uvformat      : 1;
// 	  int shading       : 1; /* 0:flat 1:gouraud */
// 	  int specular      : 1;
// 	  int texture       : 1;
// 	  int colortype     : 2; /* 0 : argb 1 : 4-float */
// 	  int modifier_code : 1;
// 	  int modifier      : 1;
// 	  int reserved_080f : 8;
// 	  int clipmode      : 2;
// 	  int striplength   : 2;
// 	  int reserved_1416 : 3;
// 	  int unknown_17    : 1;
// 	  int listtype      : 3; /* 2 : transparent */
// 	  int reserved_1B1C : 2;
// 	  int command       : 3;
// 	};

// 	struct option {
// 	  int reserved_0013 : 20;
// 	  int d_exact       : 1; /* 0:approx 1:exact (mipmap) */
// 	  int reserved_1515 : 1;
// 	  int uvformat      : 1; /* ??? */    
// 	  int shading       : 1; /* ??? */    
// 	  int specular      : 1; /* ??? */
// 	  int texture       : 1; /* 0:disable */
// 	  int zwrite        : 1; /* 0:enable */
// 	  int culling       : 2; /* 0:disable 1:small 2:ccw 3:cw */
// 	  int depthmode     : 3;
// 	};
//   }
// #endif


void draw_triangle_no_clip(const draw_vertex_t *v1,
						   const draw_vertex_t *v2,
						   const draw_vertex_t *v3,
						   int flags)
{
  poly_hdr_t poly;


  vertex_oc_t vert;
  int ta_flags = 0;
  if (DRAW_OPACITY(flags) == DRAW_TRANSLUCENT) {
	ta_flags = TA_TRANSLUCENT;
  }
  /* $$$ TODO : Texture, and shading modes */

  ta_poly_hdr_col(&poly, ta_flags);
  ta_commit_poly_hdr(&poly);

  vert.flags = TA_VERTEX_NORMAL;
  vert.x = v1->x; vert.y = v1->y; vert.z = v1->z;
  vert.a = v1->a; vert.r = v1->r; vert.g = v1->g; vert.b = v1->b;
  ta_commit_vertex(&vert, sizeof(vert));
	
  vert.x = v2->x; vert.y = v2->y; vert.z = v2->z;
  vert.a = v2->a; vert.r = v2->r; vert.g = v2->g; vert.b = v2->b;
  ta_commit_vertex(&vert, sizeof(vert));
	
  vert.flags = TA_VERTEX_EOL;
  vert.x = v3->x; vert.y = v3->y; vert.z = v3->z;
  vert.a = v3->a; vert.r = v3->r; vert.g = v3->g; vert.b = v3->b;
  ta_commit_vertex(&vert, sizeof(vert));
  
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
  clipflags = vertex_clip_flags(v3);

  if (!clipflags) {
	/* No clipping, draw the triangle */
	draw_triangle_no_clip(v1, v2, v3, flags);
	return;
  }

  andflags = clipflags;
  andflags &= andflags>>4;
  andflags &= andflags>>4;
  if (andflags & 15) {
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
	draw_triangle_clip_any(v1,v2,v3,flags,3);
  } else if (flags & 0x888) {
	/* Entering clipping stage with a BOTTOM clipping */
	draw_triangle_clip_any(v1,v2,v3,flags,4);
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
