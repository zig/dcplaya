/**
 * @ingroup dcplaya_draw
 * @file    primitives.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/10/10
 * @brief   2D drawing primitives.
 *
 * $Id: primitives.c,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#include <stdarg.h>
#include <stdio.h>

#include "draw/primitives.h"
#include "draw/ta.h"
#include "draw/gc.h"

#include "sysdebug.h"


static int sature(const float a)
{
  int v;

  v = (int) a;
  v &= ~(v>>31);
  v |= (((255-v)>>31) & 255);
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
  DRAW_SET_FLAGS(flags);

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

  DRAW_SET_FLAGS(flags);
/*   make_poly_hdr(&cur_poly, flags); */
/*   ta_commit32_inline(&cur_poly); */

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
	((v->x < current_gc->clipbox.x1) << 0) |
	((v->x > current_gc->clipbox.x2) << 2) |
	((v->y < current_gc->clipbox.y1) << 1) |
	((v->y > current_gc->clipbox.y2) << 3);
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
