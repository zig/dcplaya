/* $Id */

#ifndef _GP_H_
#define _GP_H_

#include <kos.h>


#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @name Background functions.
 *  @{
 */
/** Init background. */
int bkg_init(void);
/** Render background in opaque mode */
void bkg_render(float fade, int info_flag);
/**@}*/

/** @name Draw primitives
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
/** Draw line.*/
void draw_line(float x1, float y1, float z1, float x2, float y2, float z2,
			   float a1, float r1, float g1, float b1,
			   float a2, float r2, float g2, float b2,
			   float w);
/**@}*/

/** @name Clipping fucntions.
 *  @{
 */
/** Set clipping box. */
void draw_set_clipping(const float xmin, const float ymin,
					   const float xmax, const float ymax);
/** Gt clipping box. */
void draw_get_clipping(float * xmin, float * ymin,
					   float * xmax, float * ymax);
/**@}*/

/* songmenu.c */
void song_menu_render();

/* text.c */
int text_setup(void);
int text_set_escape(int n);

float text_set_font_size(float size);
int text_set_font(int n);

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

/* 3dutils.c */

                     

/* border.c */

typedef struct {
  uint32 texid;
  uint32 align;
  struct {
    float u,v;
  } uv[3];
} borderuv_t;

int border_setup();
extern borderuv_t borderuv[];
extern uint32 bordertex, bordertex2;

DCPLAYA_EXTERN_C_END

#endif	/* #ifndef _GP_H_ */

