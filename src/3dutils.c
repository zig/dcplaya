/**
 * @file    3dutils.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/10/10
 * @brief   2D drawing primitives.
 *
 * $Id: 3dutils.c,v 1.2 2002-10-11 12:09:28 benjihan Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include "gp.h"

float clipbox[4];

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

void draw_set_clipping(const float xmin, const float ymin,
					   const float xmax, const float ymax)
{
  clipbox[0] = xmin;
  clipbox[1] = ymin;
  clipbox[2] = xmax;
  clipbox[3] = ymax;
}

void draw_get_clipping(float * xmin, float  * ymin,
					   float * xmax, float  * ymax)
{
  if (xmin) *xmin = clipbox[0];
  if (ymin) *ymin = clipbox[1];
  if (xmax) *xmax = clipbox[2];
  if (ymax) *ymax = clipbox[3];
}
