/* $Id */

#ifndef _GP_H_
#define _GP_H_

#include <kos.h>


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


/* Floating-point Sin/Cos; 256 angles, -1.0 to 1.0 */
extern float sintab[];
#define msin(angle) sintab[angle]
#define mcos(angle) sintab[((angle)+64) % 256]

/* bkg.c */
int bkg_init(void);
void bkg_render(float fade, int info_flag);

/* 3dutils.c */
void rotate(int zang, int xang, int yang, float *x, float *y, float *z);
void draw_poly_mouse(int ptrx, int ptry);
void draw_poly_char(float x1, float y1, float z1, float a, float r, float g, float b, int c);
void draw_poly_strf(float x1, float y1, float z1, float a, float r, float g, float b, char *fmt, ...);
void draw_poly_box(float x1, float y1, float x2, float y2, float z,
		float a1, float r1, float g1, float b1,
		float a2, float r2, float g2, float b2);
float measure_poly_char(int c);
float measure_poly_text(const char * s);

/* songmenu.c */
void song_menu_render();

/* text.c */
int text_setup();
float text_set_font_size(float size);
int text_set_font(int n);
int text_set_escape(int n);
float draw_poly_text(float x1, float y1, float z1,
                     float a, float r, float g, float b,
                     const char *s, ...);
float draw_poly_center_text(float y1, float z1,
                            float a, float r, float g, float b,
                            const char *s, ...);
                     
float draw_poly_layer_text(float y1, float z1, const char *s);
void draw_poly_get_text_size(const char *txt, float *w, float *h);

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

