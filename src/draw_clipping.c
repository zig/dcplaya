/**
 *  @file    draw_clipping.c
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/10/15
 *  @brief   draw primitive vertex definition.
 *
 * $Id: draw_clipping.c,v 1.5 2002-10-21 14:57:00 benjihan Exp $
 */

#include "draw_clipping.h"
#include "sysdebug.h"

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
						const float f, int flags, int bit)
{
  const float of = 1.0f - f;
  const float clip = clipbox[bit];/*  + ((bit>1) ? -3 : 3); */

  if (!(bit & 1)) {
	v->x = clip;
	v->y = of * v0->y + f * v1->y;
  } else {
	v->x = of * v0->x + f * v1->x;
	v->y = clip;
  }
  v->z = of * v0->z + f * v1->z;

  v->a = of * v0->a + f * v1->a;
  v->r = of * v0->r + f * v1->r;
  v->g = of * v0->g + f * v1->g;
  v->b = of * v0->b + f * v1->b;

  if (DRAW_TEXTURE(flags) != DRAW_NO_TEXTURE) {
	v->u = of * v0->u + f * v1->u;
	v->v = of * v0->v + f * v1->v;
  }
}

/** Calculates TOP/BOTTOM clipping flags for a vertex. */
static int vertex_clip_tb_flags(const draw_vertex_t *v)
{
  return
	((v->y < clipbox[1]) << 1) |
	((v->y > clipbox[3]) << 3);
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


void draw_triangle_no_clip(const draw_vertex_t *v1,
						   const draw_vertex_t *v2,
						   const draw_vertex_t *v3, int flags);

static void dump_triangle(const char * label,
						  const draw_vertex_t *v1,
						  const draw_vertex_t *v2,
						  const draw_vertex_t *v3,
						  int flags, int vflags, int bits)
{
  printf("Clip %c(%d) : %s\n"
		 " %03X %03X [ (%.3f,%.3f) (%.3f,%.3f) (%.3f,%.3f) ]\n",
		 bits["LTRB"], bits, label, flags, vflags,
		 v1->x,v1->y, v2->x,v2->y, v3->x,v3->y);
}

static void draw_triangle_clip_continue(const draw_vertex_t *v1,
										const draw_vertex_t *v2,
										const draw_vertex_t *v3,
										int flags, int bits)
{
  switch (bits) {
  case 0: /* Coming from left clipping. Right clipping flags should be Ok */
	if (flags & 0x444) {
	  draw_triangle_clip_any(v1,v2,v3,flags,2);
	  return;
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
		return;
	  }
	  
	  if (clipflags & (clipflags>>4) & (clipflags>>8)) {
		/* Finish here */
		return;
	  }

	  /* Insert clipping flags into draw flags */
	  flags = (flags & ~0xFFF) | clipflags;

	  if (flags & 0x222) {
		/* clip TOP */
		draw_triangle_clip_any(v1,v2,v3,flags,1);
		return;
	  }
	}
  case 1:  /* Coming from TOP clipping. BOTTOM clipping flags should be Ok. */
	if (flags & 0x888) {
	  draw_triangle_clip_any(v1,v2,v3,flags,3);
	  return;
	}

  case 3:  /* Coming from TOP/BOTTOM clipping. Finish here. */
	draw_triangle_no_clip(v1,v2,v3,flags);
	return;
  }
}

void draw_triangle_clip_any(const draw_vertex_t *v1,
							const draw_vertex_t *v2,
							const draw_vertex_t *v3,
							int flags, const int bit)
{
  int clipflags;
  draw_vertex_t w[2];

  /*                    *											 
   *    +       +		* bit										 
   *    | v2  v3|		*  HL K=~H  cv1 triangles					 
   *  w0|/|   |\|w1		*  00   1    0  v1.L,w0.L,w1.L				 
   * v1/| |   | |\ v1	*  01   1    0  v1.L,w0.L,w1.L				 
   *   \| |	  | |/		*  10   0    0  v1.L,w0.L,w1.L				 
   *  w1|\|	  |/|w0		*  11   0    0  v1.L,w0.L,w1.L				 
   *    | v3  v2|     	*  00   1    1  w0.L,v2.L,w1.L w1.L,v2.L,v3.L 
   *    +       +		*  01   1    1  w0.L,v2.L,w1.L w1.L,v2.L,v3.L 
   *					*  10   0    1  w0.L,v2.L,w1.L w1.L,v2.L,v3.L 
   *					*  11   0    1  w0.L,v2.L,w1.L wK.L,v2.L,v3.L 
   */

  /* Keeps interresting bits */
  clipflags = flags & (0x111<<bit);
  /* Removes from original flags */
  flags ^= clipflags;
  /* Shift them to bit 0 */
  clipflags >>= bit;

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
   * cv1 = a^b^c
   * v1 = (-(flags^(-(flags>>2)&3))) 
   */

  /* Process vertrices rolling so that involved vertex becomes v1 */
  switch ((clipflags^(-(clipflags>>8)))&0x11) { 
  case 0x10:
	{
	  const draw_vertex_t * v0;
	  /* v1=v2, v2=v3, v3=v1 */
	  v0 = v1; v1 = v2;  v2 = v3; v3 = v0;
	  /* Roll clipping flags : 0x123 -> 0x231 */
	  flags = (flags & ~0xFFF) | ((flags & 0x0FF) << 4) | ((flags>>8) & 0x00F);
	} break;
  case 0x01:
	{
	  const draw_vertex_t * v0;
	  /* v1=v3, v2=v1, v3=v2 */
	  v0 = v1; v1 = v3; v3 = v2; v2 = v0;
	  /* Roll clipping flags : 0x123 -> 0x312 */
	  flags = (flags & ~0xFFF) | ((flags>>4)&0x0FF) | ((flags&0x00F)<<8);
	} break;
  }
  
  /* Computes intersection vertrices (xn can be either xn or yn) */
  {
	const int i = bit & 1;
	const float x1 = (&v1->x)[i];
	const float x2 = (&v2->x)[i];
	const float x3 = (&v3->x)[i];
	const float x12 = x2-x1;
	const float x13 = x3-x1;
	float f12;
	float f13;

	if ((x12<0 ? -x12 : x12) < 0.0001) {
	  f12 = 0.5;
	} else {
	  f12 = (clipbox[bit]-x1) / (x12);
	}
	if ((x13<0 ? -x13 : x13) < 0.0001) {
	  f13 =  0.5;
	} else {
	  f13 = (clipbox[bit]-x1) / (x13);
	}
	vertex_clip(&w[0], v1, v2, f12, flags, bit);
	vertex_clip(&w[1], v1, v3, f13, flags, bit);
  }

  if ((clipflags^(clipflags>>4)^(clipflags>>8))&1) {
	draw_triangle_clip_continue(&w[0], v2, &w[1], flags & ~0xF0F , bit);
	draw_triangle_clip_continue(&w[1], v2,    v3, flags & ~0xF00, bit);
  } else {
	draw_triangle_clip_continue(v1, &w[0], &w[1], flags & ~0x0FF, bit);
  }
}
