/* $Id: gp.h,v 1.11 2002-11-14 23:40:27 benjihan Exp $ */

#ifndef _GP_H_
#define _GP_H_

#include <kos.h>
#include "extern_def.h"
#include "draw_vertex.h"
#include "texture.h"

DCPLAYA_EXTERN_C_START

/** @name  Tile accelerator interface.
 */

typedef struct {
  volatile uint32 word1;
  volatile uint32 word2;
  volatile uint32 word3;
  volatile uint32 word4;
  volatile uint32 words[4];
} ta_hw_poly_t;

typedef struct {
  volatile uint32 flags;
  volatile float x;
  volatile float y;
  volatile float z;
  volatile float a;
  volatile float r;
  volatile float g;
  volatile float b;
} ta_hw_col_vtx_t;

typedef struct {
  volatile uint32 flags;
  volatile float x;
  volatile float y;
  volatile float z;
  volatile float u;
  volatile float v;
  volatile uint32 col;
  volatile uint32 addcol;
} ta_hw_tex_vtx_t;

#define HW_COL_VTX ((ta_hw_col_vtx_t *)(0xe0<<24))
#define HW_TEX_VTX ((ta_hw_tex_vtx_t *)(0xe0<<24))
#define HW_POLY    ((ta_hw_poly_t    *)(0xe0<<24))

/**@}*/

/** Build a TA poly header. */
void draw_poly_hdr(ta_hw_poly_t * poly, int flags);

/** @name Draw primitives.
 *  @{
 */

/** Draw uniform box in translucent mode. */
void draw_box1(float x1, float y1, float x2, float y2, float z,
			   float a, float r, float g, float b);
/** Draw horizontal gradiant box. */
void draw_box2h(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2);
/** Draw vertical gradiant box. */
void draw_box2v(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2);
/** Draw diagonal gradiant box. */
void draw_box2d(float x1, float y1, float x2, float y2, float z,
				float a1, float r1, float g1, float b1,
				float a2, float r2, float g2, float b2);
/** Draw general gradiant box. */
void draw_box4(float x1, float y1, float x2, float y2, float z,
			   float a1, float r1, float g1, float b1,
			   float a2, float r2, float g2, float b2,
			   float a3, float r3, float g3, float b3,
			   float a4, float r4, float g4, float b4);
/**@}*/

/** @name Draw primtives.
 *  @{
 */

/** Draw line.*/
void draw_line(float x1, float y1, float z1, float x2, float y2, float z2,
			   float a1, float r1, float g1, float b1,
			   float a2, float r2, float g2, float b2,
			   float w);

/** Draw triangle. */
void draw_triangle(const draw_vertex_t *v1,
				   const draw_vertex_t *v2,
				   const draw_vertex_t *v3,
				   int flags);

/** Draw strip. */
void draw_strip(const draw_vertex_t *v, int n, int flags);

/**@}*/

/* songmenu.c */
void song_menu_render();

/* text.c */
typedef unsigned int fontid_t;

/** Init the text primitive. */
int text_setup(void);

/** Create a new font. */
fontid_t text_new_font(texid_t texid, int wc, int hc, int fixed);

/** Set text format strong escape char. */
int text_set_escape(int n);

/** Set current font size. */
float text_set_font_size(float size);

/** Set current font. */
fontid_t text_set_font(fontid_t fontid);

unsigned int text_get_argb(void);
void text_get_color(float *a, float *r, float *g, float *b);
void text_set_color(const float a, const float r,
					const float g, const float b);
void text_set_argb(unsigned int argb);

float text_draw_vstrf(float x1, float y1, float z1, const char *s,
					  va_list list);
float text_draw_strf(float x1, float y1, float z1, const char *s, ...);
float text_draw_str(float x1, float y1, float z1, const char *s);
float text_draw_str_inside(float x1, float y1, float x2, float y2, float z1,
						   const char *s);

float text_draw_strf_center(float x1, float y1, float x2, float y2, float z1,
                            const char *s, ...);

float text_measure_char(int c);
float text_measure_str(const char * s);
float text_measure_vstrf(const char * s, va_list list);
float text_measure_strf(const char * s, ...);
void text_size_str(const char * s, float * w, float * h);

/* border.c */

typedef struct {
  texid_t texid;
  uint32 align;
  struct {
    float u,v;
  } uv[3];
} borderuv_t;

int border_setup(void);

extern borderuv_t borderuv[];
extern texid_t bordertex[];

DCPLAYA_EXTERN_C_END

#endif	/* #ifndef _GP_H_ */

