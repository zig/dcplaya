/**
 *  @file    draw_clipping.c
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/10/15
 *  @brief   draw primitive vertex definition.
 *
 * $Id: draw_clipping.c,v 1.1 2002-10-16 18:50:40 benjihan Exp $
 */

#include "draw_clipping.h"

float clipbox[4];

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

static void vertex_clip(draw_vertex_t *v,
						const draw_vertex_t *v0, const draw_vertex_t *v1,
						const float f, int flags)
{
  const float of = 1.0f - f;

  v->x = of * v0->x + f * v1->x;
  v->y = of * v0->y + f * v1->y;
  v->z = of * v0->z + f * v1->z;

  if (DRAW_SHADING(flags) == DRAW_FLAT) {
	v->a = v0->a;
	v->r = v0->r;
	v->g = v0->g;
	v->b = v0->b;
  } else {
	v->a = of * v0->a + f * v1->a;
	v->r = of * v0->r + f * v1->r;
	v->g = of * v0->g + f * v1->g;
	v->b = of * v0->b + f * v1->b;
  }

  if (DRAW_TEXTURE(flags) != DRAW_NO_TEXTURE) {
	v->u = of * v0->u + f * v1->u;
	v->v = of * v0->v + f * v1->v;
  }
}

/** Calculates clipping flags for a vertex. */
static int vertex_clip_tb_flags(const draw_vertex_t *v)
{
  return
	((v->y < clipbox[1]) << 1) |
	((v->y > clipbox[3]) << 3);
}

void draw_triangle_no_clip(const draw_vertex_t *v1,
						   const draw_vertex_t *v2,
						   const draw_vertex_t *v3, int flags);

static void draw_triangle_clip_continue(const draw_vertex_t *v1,
										const draw_vertex_t *v2,
										const draw_vertex_t *v3,
										int flags, int bits)
{
  switch (bits) {
  case 0: /* Coming from left clipping. Right clipping flags should be Ok */
	if (flags & 0x444) {
	  draw_triangle_clip_any(v1,v2,v3,flags,2);
	  break;
	}
  case 2: /* Coming from left/right clipping. Calculates new TOP/BOTTOM */
	{
	  int clipflags;
	  clipflags = vertex_clip_tb_flags(v1);
	  clipflags <<= 4;
	  clipflags |= vertex_clip_tb_flags(v2);
	  clipflags <<= 4;
	  clipflags |= vertex_clip_tb_flags(v3);
	  if (!clipflags) {
		draw_triangle_no_clip(v1,v2,v3,flags);
		break;
	  }
	  
	  if (clipflags & (clipflags>>4) & (clipflags>>8)) {
		/* Finish here */
		return;
	  }

	  /* Insert clipping flags into draw flags */
	  flags = (flags & ~0xFFF) | clipflags;

	  if (clipflags & 0x222) {
		draw_triangle_clip_any(v1,v2,v3,flags,1);
	  } else {
		draw_triangle_clip_any(v1,v2,v3,flags,3);
	  }
	} break;
  case 1:  /* Coming from TOP clipping. BOTTOM clipping flags should be Ok. */
	if (flags & 0x888) {
	  draw_triangle_clip_any(v1,v2,v3,flags,3);
	  break;
	}
  case 3:  /* Coming from TOP/BOTTOM clipping. Finish here. */
	draw_triangle_no_clip(v1,v2,v3,flags);
  }
}

void draw_triangle_clip_any(const draw_vertex_t *v1,
							const draw_vertex_t *v2,
							const draw_vertex_t *v3,
							int flags, const int bit)
{
  int clipflags;
  draw_vertex_t w[2];

  /* LT clip, 1 OUT or RB clip, 1 IN
   *    +       +		
   *    | v2  v3|		
   *  w0|/|   |\|w0	
   * v1/| |   | |\ v1	
   *   \| |	  | |/	
   *  w1|\|	  |/|w1	
   *    | v3  v2|     
   *    +       +
   */

  /*
   * bit
   *  HL K=!H  cv1 triangles
   *  00   1    0  v1.L,wK.L,wH.L
   *  01   1    0  v1.L,wK.L,wH.L
   *  10   0    0  v1.L,wK.L,wH.L
   *  11   0    0  v1.L,wK.L,wH.L
   *  00   1    1  wH.L,v2.L,wK.L wK.L,v2.L,v3.L
   *  01   1    1  wH.L,v2.L,wK.L wK.L,v2.L,v3.L  
   *  10   0    1  wH.L,v2.L,wK.L wK.L,v2.L,v3.L 
   *  11   0    1  wH.L,v2.L,wK.L wK.L,v2.L,v3.L 
   */

  /* Keeps interresting bits */
  clipflags = flags & (0x111<<bit);
  /* Removes from original flags */
  flags -= clipflags;
  /* Shift them to bit 0 */
  clipflags >>= bit;
  /* "Pack" bits */
  clipflags = (clipflags | (clipflags>>(4-1)) |  (clipflags>>(8-2))) & 3;

  /* flags  v1   cv1 
   *  abc              ^aa   -   
   *  000    ?    ?   
   *  001    11   1    001  111
   *  010    10   1    010  110
   *  011    01   0    011  101
   *  100    01   1    111  001
   *  101    10   0    110  010
   *  110    11   0    101  011
   *  111    ?    ?
   *
   * cv1 = (flags^(flags>>1)^(flags>>2))&1
   * v1 = (-(flags^(-(flags>>2)&3))) 
   */

  /* Process vertrices rolling so that involved vertex becomes v1 */
  switch ((clipflags^(-(clipflags>>2)&3))&3) {
  case 2:
	{
	  const draw_vertex_t * v0;
	  /* v1=v2, v2=v3, v3=v1 */
	  v0 = v1; v1 = v2;  v2 = v3; v3 = v0;
	  /* Roll clipping flags : 0x123 -> 0x231 */
	  flags = (flags & ~0xFFF) | ((flags & 0x0FF) << 4) | ((flags>>8) & 0x00F);
  }
  case 1:
	{
	  const draw_vertex_t * v0;
	  /* v1=v3, v2=v1, v3=v2 */
	  v0 = v1; v1 = v3; v3 = v2; v2 = v0;
	  /* Roll clipping flags : 0x123 -> 0x312 */
	  flags = (flags & ~0xFFF) | ((flags>>4)&0x0FF) | ((flags&0x00F)<<8);
	}
  }

  /* Computes intersection vertrices (xn can be either xn or yn) */
  {
	const int i = bit & 1;
	const int j = bit >> 1;
	const float x1 = (&v1->x)[i];
	const float x2 = (&v2->x)[i];
	const float x3 = (&v3->x)[i];
	const float f12 = (clipbox[bit]-x1) / (x2-x1);
	const float f13 = (clipbox[bit]-x1) / (x3-x1);
	vertex_clip(&w[j], v1, v2, f12, flags);
	vertex_clip(&w[1-j], v1, v3, f13, flags);
  }


  if ((clipflags^(clipflags>>1)^(clipflags>>2))&1) {
	/* LT clip, 1 OUT */
	draw_triangle_clip_continue(&w[0],v2,&w[1],flags,bit);
	draw_triangle_clip_continue(&w[1],v2,v3,flags,bit);
  } else {
	/* RB clip, 1 IN */
	draw_triangle_clip_continue(v1,&w[0],&w[1],flags,bit);
  }

}
