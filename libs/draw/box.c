/**
 * @ingroup dcplaya_draw
 * @file    box.c
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/11/22
 *
 * $Id: box.c,v 1.4 2002-12-24 04:07:54 ben Exp $
 */

#include "draw/box.h"
#include "draw/gc.h"
#include "draw/ta.h"

#include <dc/ta.h>
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
  if (x1 >= current_gc->clipbox.x2 || y1 >= current_gc->clipbox.y2 ||
      x2 <= current_gc->clipbox.x1 || y2 <= current_gc->clipbox.y1) {
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
  if (x1 < current_gc->clipbox.x1) {
    float f1 = (current_gc->clipbox.x1 - x1) / w;
    float f2 = 1.0f - f1;
    x1 = current_gc->clipbox.x1;
    w = x2 - x1;
    if (w < 1E-5) {
      return;
    }

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
  if (y1 < current_gc->clipbox.y1) {
    float f1 = (current_gc->clipbox.y1 - y1) / h;
    float f2 = 1.0f - f1;
    y1 = current_gc->clipbox.y1;
    h = y2 - y1;
    if (h < 1E-5) {
      return;
    }

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
  if (x2 > current_gc->clipbox.x2) {
    float f2 = (x2 - current_gc->clipbox.x2) / w;
    float f1 = 1.0f - f2;
    x2 = current_gc->clipbox.x2;
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
  if (y2 > current_gc->clipbox.y2) {
    float f2 = (y2 - current_gc->clipbox.y2) / h;
    float f1 = 1.0f - f2;
    y2 = current_gc->clipbox.y2;
    CLIPME(a,3,1);
    CLIPME(r,3,1);
    CLIPME(g,3,1);
    CLIPME(b,3,1);
    CLIPME(a,4,2);
    CLIPME(r,4,2);
    CLIPME(g,4,2);
    CLIPME(b,4,2);
  }

  DRAW_SET_FLAGS(DRAW_NO_TEXTURE|DRAW_TRANSLUCENT|DRAW_NO_FILTER);

  HW_COL_VTX->flags = TA_VERTEX_NORMAL;
  HW_COL_VTX->x = x1;
  HW_COL_VTX->y = y2;
  HW_COL_VTX->z = z;
  HW_COL_VTX->a = a3;
  HW_COL_VTX->r = r3;
  HW_COL_VTX->g = g3;
  HW_COL_VTX->b = b3;
  ta_commit32_nocopy();

  HW_COL_VTX->y = y1;
  HW_COL_VTX->a = a1;
  HW_COL_VTX->r = r1;
  HW_COL_VTX->g = g1;
  HW_COL_VTX->b = b1;
  ta_commit32_nocopy();
	
  HW_COL_VTX->x = x2;
  HW_COL_VTX->y = y2;
  HW_COL_VTX->a = a4;
  HW_COL_VTX->r = r4;
  HW_COL_VTX->g = g4;
  HW_COL_VTX->b = b4;
  ta_commit32_nocopy();

  HW_COL_VTX->flags = TA_VERTEX_EOL;
  HW_COL_VTX->y = y1;
  HW_COL_VTX->a = a2;
  HW_COL_VTX->r = r2;
  HW_COL_VTX->g = g2;
  HW_COL_VTX->b = b2;
  ta_commit32_nocopy();

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
