/**
 * @ingroup dcplaya_draw
 * @file    text.c
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/02/11
 * @brief   drawing and formating text primitives
 *
 * $Id: text.c,v 1.7 2003-03-06 19:59:41 zigziggy Exp $
 */

#include <stdarg.h>
#include <stdlib.h>
#include <arch/spinlock.h>

#include "sysdebug.h"
#include "draw/gc.h"
#include "draw/ta.h"

#include "sysdebug.h"

#define CLIP(a, min, max) ((a)<(min)? (min) : ((a)>(max)? (max) : (a)))

typedef struct {
  float w;
  float h;
  float u1, u2, v1, v2;
} myglyph_t;

typedef struct {
  texid_t texid;
  float wc;
  float hc;
  float xs;
  float ys;
  unsigned int n;

  float wadd; /**< additional value to add to x when printing several */
              /**  adjacent characters in a string. */

  /* NEED TO BE LAST */
  myglyph_t glyph[1];

} font_t;

static font_t dummyfont = {
  0, 16, 16, 1, 1, 1, { { 16, 16, 0, 1, 0, 1 } }
};

static spinlock_t mutex;
#define LOCK() spinlock_lock(&mutex)
#define UNLOCK() spinlock_unlock(&mutex)
static fontid_t nfont;
static font_t * fonts[8];

#define curfont (fonts[current_gc->text.fontid])

/* static font_t * curfont; */
/* static fontid_t fontid; */
/* static float text_size; */
/* static float text_xscale; */
/* static float text_yscale; */
/* static unsigned int text_argb; */

/* static fontid_t fontid_save; */
/* static float text_size_save; */
/* static float text_xscale_save; */
/* static float text_yscale_save; */
/* static unsigned int text_argb_save; */

//static int escape_char = '%';

static int do_escape(int c, va_list * list);
static void set_properties(fontid_t n, const float size, const float aspect,
					  int filter);

static void bounding(uint16 * img, int w, int h, int bpl, int *box)
{
  int x , y;
  uint16 * b;

  box[0] = w;
  box[1] = h;
  box[2] = -1;
  box[3] = -1;
 
  for (b=img, y=0; y<h; y++, b+=bpl) {
    for (x=0; x<w; ++x) {
      if (b[x]) {
        if (x < box[0]) {
          box[0] = x;
        }
        if (y < box[1]) {
          box[1] = y;
        }
        if (x > box[2]) {
          box[2] = x;
        }
        if (y > box[3]) {
          box[3] = y;
        }
      }
    }
  }
  
  if (box[0] == w) {
    box[0] = 0;
    box[1] = 0;
    box[2] = 2;
    box[3] = h-1;
  }
}

fontid_t text_new_font(texid_t texid, int wc, int hc, int fixed, float wspace, float wadd)
{
  font_t * fnt = 0;
  texture_t * t = 0;
  int x,y,c;
  int b[4];
  float xs, ys;
  int lines, cpl, chars;
  uint16 * base;

  if (nfont >= sizeof(fonts)/sizeof(*fonts)) {
	goto error;
  }

  t = texture_lock(texid);
  if (!t) {
	goto error;
  }

  lines = t->height / (int) hc;
  cpl   = t->width / (int) wc;
  chars = lines * cpl;

  fnt = malloc(sizeof(*fnt) - sizeof(fnt->glyph) + chars*sizeof(*fnt->glyph));
  if (!fnt) {
	goto error;
  }

  fnt->texid = texid;
  fnt->n  = chars;
  fnt->wc = wc;
  fnt->hc = hc;
  fnt->xs = (float)wc / (float)t->width;
  fnt->ys = (float)hc / (float)t->height;
  fnt->wadd = wadd;

  xs = 1.0f / (float)t->width;
  ys = fnt->ys;

  base = t->addr;

  for (c = y = 0; y < lines; ++y, base += hc << t->wlog2) {
	uint16 * ba;
	for (x = 0, ba = base; x < cpl; ++x, ++c, ba += wc) {
	  myglyph_t *m = fnt->glyph + c;

	  if (!fixed) {
		bounding(ba, wc, hc, 1<<t->wlog2, b);
		m->w = (float)(b[2] - b[0] + 2);
		m->h = hc;
		m->u1 = (float) (x * wc + b[0]) * xs;
		m->u2 = (float) (x * wc + b[2]+1) * xs;
	  } else {
		m->w = wc;
		m->h = hc;
		m->u1 = (float)x * fnt->xs;
		m->u2 = m->u1 + fnt->xs;
	  }
	  m->v1 = (float)y * ys;
	  m->v2 = m->v1 + ys;
	}
  }
  texture_release(t);
    
  /* Special case for char 4[pad] & 6[joy] */
  for (c=4; c<8; c+=2) {
	fnt->glyph[c].u1 = (fnt->glyph[c+cpl  ].u1 < fnt->glyph[c     ].u1)  ? 
	  fnt->glyph[c+cpl  ].u1 : fnt->glyph[c     ].u1;
	fnt->glyph[c].v1 = (fnt->glyph[c+ 1  ].v1 < fnt->glyph[c     ].v1)  ? 
	  fnt->glyph[c+ 1  ].v1 : fnt->glyph[c     ].v1;
	fnt->glyph[c].u2 = (fnt->glyph[c+1+cpl].u2 > fnt->glyph[c+1   ].u2)  ? 
	  fnt->glyph[c+1+cpl].u2 : fnt->glyph[c+1   ].u2;
	fnt->glyph[c].v2 = (fnt->glyph[c+cpl  ].v2 > fnt->glyph[c+1+cpl].v2)  ? 
	  fnt->glyph[c+cpl  ].v2 : fnt->glyph[c+1+cpl].v2;
	fnt->glyph[c].w  = wc * 2;
	fnt->glyph[c].h  = hc * 2;
  }

  if (!fixed && wspace)
    fnt->glyph[' '].w = wspace;

  SDDEBUG("New font : id:%d %dx%d [%s]\n", nfont, (int)wc, (int)hc,
		  fixed ? "fixed" : "proportionnel");

  fonts[nfont] = fnt;
  return nfont++;

 error:
  if (t) {
	texture_release(t);
  }
  return -1;
}

void text_shutdown(void)
{
  int i;

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;
 
  for (i=0; i<nfont; ++i) {
	font_t * f = fonts[i];
	if (f && f != &dummyfont) {
	  free(f);
	}
	fonts[i] = 0;
  }
  nfont = 0;

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);
}

int text_init(void)
{
  int err = 0;
  texid_t texid;

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;
  
  spinlock_init(&mutex);

  if (!current_gc) {
	SDERROR("No GC.\n");
	err = -1;
	goto error;
  }

  nfont = 0;
  current_gc->text.fontid = 0;
  memset(fonts,0, sizeof(fonts));
  current_gc->text.argb = 0xFFFFFFFF;
  current_gc->text.size = 16;
  current_gc->text.aspect = 1.0f;

  fonts[0] = &dummyfont;

  texid = texture_create_file("/rd/font16x16", "4444");
/*  text_new_font(texid,16,16,0, 0, 0); */
  text_new_font(texid,16,16,0, 9, -4);
  texid = texture_create_file("/rd/font8x14", "4444");
  text_new_font(texid,8,14,1, 0, 0);
  text_set_font(0);

 error:
  SDUNINDENT;
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, err);

  return err;
}

static void save_state(gc_text_t * save)
{
  *save = current_gc->text;
}

static void restore_state(const gc_text_t * restore)
{
  current_gc->text = *restore;
}

static myglyph_t * curglyph(unsigned int c)
{
  if (c >= curfont->n) {
	  c = 0;
  }
  return curfont->glyph + c;
}

static float size_of_char(float * h, int c)
{
  myglyph_t * g;
  float wc, hc;
  const float xscale = current_gc->text.size / curfont->wc;
  const float yscale = xscale * current_gc->text.aspect;

  g = curglyph(c);
  hc = g->h * yscale;
  wc = (curfont->wadd + g->w) * xscale;
  if (h) *h = hc;
  return wc;
}

float text_measure_char(int c)
{
  float r;
  LOCK();
  r = (size_of_char(0, c));
  UNLOCK();
  return r;
}

void text_size_char(char c, float * w, float * h)
{
  float w2;
  LOCK();
  w2 = size_of_char(h, c);
  if (w) *w = w2;
  UNLOCK();
}

static float size_of_str(float * h, const char *s)
{
  unsigned int c;
  float sum = 0, max_h = 0;

  const float xscale = current_gc->text.size / curfont->wc;
  const float yscale = xscale * current_gc->text.aspect;

  while ((c=(*s++)&255), c) {
	myglyph_t * g;
	  
	if (c >= curfont->n) {
	  c = 0;
	}
	g = curfont->glyph + c;
	if (g->h > max_h) max_h = g->h;
	sum += (g->w + curfont->wadd);
  }
  if (h) *h = max_h * yscale;

  return sum * xscale;
}

static float size_of_strf(float *h, const char *s, va_list list)
{
  int c, esc = 0;
  float sum = 0, maxh = 0;
  gc_text_t savegc;
  float xscale = current_gc->text.size / curfont->wc;
  float yscale = xscale * current_gc->text.aspect;


  save_state(&savegc);
  while ((c=(*s++)&255), c) {
	myglyph_t * g;
	float ch;
	if (esc) {
	  c = do_escape(c, &list);
	  xscale = current_gc->text.size / curfont->wc;
	  yscale = xscale * current_gc->text.aspect;
	  esc = 0;
	} else if (c==current_gc->text.escape) {
      esc = 1;
	  c = -1;
    }
	if (c == -1) continue;
	g = curglyph(c);
	ch = g->h * yscale;
	if (ch > maxh) maxh = ch;
	sum += g->w * xscale;
  }
  restore_state(&savegc);
  if (h) *h = maxh;

  return sum;
}

float text_measure_str(const char * s)
{
  float r;
  LOCK();
  r = size_of_str(0, s);
  UNLOCK();
  return r;
}

float text_measure_vstrf(const char * s, va_list list)
{
  float r;
  LOCK();
  r = size_of_strf(0, s, list);
  UNLOCK();
  return r;
}

float text_measure_strf(const char * s, ...)
{
  float h;
  va_list list;
  va_start(list,s);
  LOCK();
  h = size_of_strf(0, s, list);
  UNLOCK();
  va_end(list);
  return h;
}

void text_size_str(const char * s, float * w, float * h)
{
  float w2;

  LOCK();
  w2 =  size_of_str(h, s);
  UNLOCK();
  if (w) *w = w2;
}

void text_size_str_prop(const char * s, float * w, float * h,
						fontid_t n, const float size, const float aspect)
{
  float w2;
  gc_text_t save;
  LOCK();
  save = current_gc->text;
  set_properties(n, size, aspect, -1);
  w2 =  size_of_str(h, s);
  current_gc->text = save;
  UNLOCK();
  if (w) *w = w2;
}

/* Draw one font character (16x16); assumes polygon header already sent */
static float draw_text_char(float x1, float y1, float z1,
                            float scalex, float scaley,
                            int c)
{
  float wc, hc;
  float u1,u2,v1,v2;
  float x2,y2;
  myglyph_t * g;
  draw_argb_t argb;
	
  ta_hw_tex_vtx_t * hw = HW_TEX_VTX;

  g = curglyph(c);
  if (!g) {
	return x1;
  }

  /* Compute width and height */
  wc = g->w * scalex;
  hc = g->h * scaley;

  argb = current_gc->text.argb;

#define A_BUTTON_COLOR 0x00FF6363
#define B_BUTTON_COLOR 0x0000bded
#define X_BUTTON_COLOR 0x00fff021
#define Y_BUTTON_COLOR 0x006fe066
#define DPAD_COLOR     0x00CCCCCC
#define JOY_COLOR      0x00EEEEEE


  if (c==4 || c==6) {
	/* Joypad special case */
	argb = (argb & 0xFF000000) | (c==4 ? DPAD_COLOR : JOY_COLOR);
    y1 -= hc * 0.5f;
    hc *= 2.0f;
  } else if (c >= 16 && c <= 19) {
	/* Pad buttons special case */
	static draw_argb_t colors[4] = {
	  A_BUTTON_COLOR, B_BUTTON_COLOR, X_BUTTON_COLOR, Y_BUTTON_COLOR
	};
	argb = (argb & 0xFF000000) | colors[c-16];
  }

  /* Clip very small char */
  if (hc < 1E-5 || wc < 1E-5) {
	return x1;
  }

  /* Compute right and bottom */
  x2 = x1 + wc;
  y2 = y1 + hc;

  /* Clip right out and bottom out*/
  /* Clip left out and top out */
  if (x1 >= current_gc->clipbox.x2 || y1 >= current_gc->clipbox.y2 ||
	  x2 <= current_gc->clipbox.x1 || y2 <= current_gc->clipbox.y1) {
	return x2;
  }

  /* Compute UV */
  u1 = g->u1;
  u2 = g->u2;
  v1 = g->v1;
  v2 = g->v2;
  
  /* Left clip */
  if (x1 < current_gc->clipbox.x1) {
	float f = (current_gc->clipbox.x1 - x1) / wc;
	x1 = current_gc->clipbox.x1;
	u1 = u2 * f + u1 * (1.0f-f);
  }
  /* Top clip */
  if (y1 < current_gc->clipbox.y1) {
	float f = (current_gc->clipbox.y1 - y1) / hc;
	y1 = current_gc->clipbox.y1;
	v1 = v2 * f + v1 * (1.0f-f);
  }
  /* Right clip */
  if (x2 > current_gc->clipbox.x2) {
	float f = (x2 - current_gc->clipbox.x2) / wc;
	x2 = current_gc->clipbox.x2;
	u2 = u1 * f + u2 * (1.0f-f);
  }
  /* Bottom clip */
  if (y2 > current_gc->clipbox.y2) {
	float f = (y2 - current_gc->clipbox.y2) / hc;
	y2 = current_gc->clipbox.y2;
	v2 = v1 * f + v2 * (1.0f-f);
  }

  hw->flags = TA_VERTEX_NORMAL;
  hw->x = x1;
  hw->y = y2;
  hw->z = z1;
  hw->u = u1;
  hw->v = v2;
  hw->col = argb;
  hw->addcol = 0;
  ta_commit32_nocopy();
	
  hw->x = x1;
  hw->y = y1;
  hw->u = u1;
  hw->v = v1;
  ta_commit32_nocopy();
	
  hw->x = x2;
  hw->y = y2;
  hw->u = u2;
  hw->v = v2;
  ta_commit32_nocopy();

  hw->flags = TA_VERTEX_EOL;
  hw->x = x2;
  hw->y = y1;
  hw->u = u2;
  hw->v = v1;
  ta_commit32_nocopy();
	
  return x2 + curfont->wadd*scalex;
}

static int do_escape(int c, va_list *list)
{
  switch (c) {
  case 'A': case 'B':
	c = 16 + c - 'A';
	break;
  case 'X': case 'Y':
	c = 18 + c - 'X';
	break;
  case '+':
	c = 4;
	break;
  case 'o':
	c = 6;
	break;
  case 'a':
	c = -1;
	current_gc->text.argb = (current_gc->text.argb & 0x00FFFFFF) | (va_arg(*list, int)<<24);
	break;
  case 'r':
	c = -1;
	current_gc->text.argb = (current_gc->text.argb & 0xFF00FFFF) | (va_arg(*list, int)<<16);
	break;
  case 'g':
	c = -1;
	current_gc->text.argb = (current_gc->text.argb & 0xFFFF00FF) | (va_arg(*list, int)<<8);
	break;
  case 'b':
	c = -1;
	current_gc->text.argb = (current_gc->text.argb & 0xFFFFFF00) | va_arg(*list, int);
	break;
  case 'c':
	c = -1;
	current_gc->text.argb = va_arg(*list, unsigned int);
	break;
  case 's':
	text_set_font_size((float)va_arg(*list, int));
	break;
  default:
	SDWARNING("[%s] : weird escape char : 0x%02X\n",__FUNCTION__, c);
  }
  return c;
}


/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float text_draw_vstrf(float x1, float y1, float z1,
							 const char *s, va_list list)
{
  int esc = 0, c;
  float maxh = 0;
  int flags;
  float xscale, yscale;

  if (x1>=current_gc->clipbox.x2 || y1>=current_gc->clipbox.y2) {
	return 0;
  }

  xscale = current_gc->text.size / curfont->wc;
  yscale = xscale * current_gc->text.aspect;

  flags = 0
	| DRAW_TRANSLUCENT
	| (current_gc->text.filter ? DRAW_BILINEAR : DRAW_NO_FILTER)
	| (curfont->texid << DRAW_TEXTURE_BIT);

  DRAW_SET_FLAGS(flags);

  while (c=(*s++)&255, c) {
	float curh;

    if (esc) {
      esc = 0;
	  c = do_escape(c, &list);
	  xscale = current_gc->text.size / curfont->wc;
	  yscale = xscale * current_gc->text.aspect;
    } else if (c==current_gc->text.escape) {
      c = -1;
      esc = 1;
    }
	if (c == -1) continue;

    if (c == ' ') {
      x1 += curfont->glyph[32].w * xscale;
	  curh = curfont->glyph[32].h * yscale;
    } else {
	  myglyph_t * g = curglyph(c);
      x1 = draw_text_char(x1, y1, z1, xscale, yscale, c);
	  curh = g->h * yscale;
    }
	if (curh > maxh) maxh = curh;

	if (x1>=current_gc->clipbox.x2) {
	  break;
	}

  }
  return maxh;
}

/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float text_draw_strf(float x1, float y1, float z1, const char *s, ...)
{
  va_list list;
  float res;

  va_start(list, s);
  res = text_draw_vstrf(x1, y1, z1, s, list);
  va_end(list);
  return res;
}

float text_draw_str(float x1, float y1, float z1, const char *s)
{
  va_list list;
  float res;
  int save_escape = current_gc->text.escape;
  current_gc->text.escape = 0;

/*   va_start(list, s); */
  res = text_draw_vstrf(x1, y1, z1, s, list);
/*   va_end(list); */
  current_gc->text.escape = save_escape;
  return res;
}



/* */
float text_draw_strf_center(float x1, float y1, float x2, float y2, float z1,
                            const char *s, ...)
{
  float res, w, h;
  va_list list;

  va_start(list, s);
  w =  size_of_strf(&h, s, list);

  x1 = x1 + ((x2-x1)-w) * 0.5f;
  y1 = y1 + ((y2-y1)-h) * 0.5f;
  
  va_start(list, s);
  res = text_draw_vstrf(x1, y1, z1, s, list);

  va_end(list);
  return res;
}

/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float text_draw_str_inside(float x1, float y1, float x2, float y2, float z1,
						   const char *s)
{
  const float boxw = x2 - x1;
  const float boxh = y2 - y1;
  float scale = 1.0f, strw, strh, xscale, yscale;
  int c, flags;
	
  if (!s) {
    return 0.0f;
  }

  xscale = current_gc->text.size / curfont->wc;
  strw = size_of_str(&strh, s);

  if (strw > boxw || strh > boxh) {
	float f1 = boxw / strw, f2 =  boxh / strh;
	scale = (f2 < f1) ? f2 : f1;
	strw *= scale;
	strh *= scale;
	xscale *= scale;
  }
  yscale = xscale * current_gc->text.aspect;

  x1 += (boxw - strw) * 0.5f;
  y1 += (boxh - strh) * 0.5f;

  flags = 0
	| DRAW_TRANSLUCENT
	| (current_gc->text.filter ? DRAW_BILINEAR : DRAW_NO_FILTER)
	| (curfont->texid << DRAW_TEXTURE_BIT);

  DRAW_SET_FLAGS(flags);

  while (c=(*s++)&255, c) {
    if (c == ' ') {
      x1 += (curfont->glyph[32].w + curfont->wadd) * xscale;
    } else {
      x1 = draw_text_char(x1, y1, z1, xscale, yscale, c);
    }
  }

  return strh;
}

float text_set_font_size(const float size)
{
  float old;

  LOCK();
  old = current_gc->text.size;
  if (size >= 0) {
	current_gc->text.size = size;
  }
  UNLOCK();
  return old;
}

float text_set_font_aspect(const float aspect)
{
  float old;
  LOCK();
  old = current_gc->text.aspect;
  if (aspect >= 0) {
	current_gc->text.aspect = aspect;
  }
  UNLOCK();
  return old;
}

fontid_t text_set_font(fontid_t n)
{
  int old;
  LOCK();
  old = current_gc->text.fontid;

  if (n < nfont) {
	current_gc->text.fontid = n;
  }
  UNLOCK();
  return old;
}

static void set_properties(fontid_t n, const float size, const float aspect,
						   int filter)
{
  if (n < nfont) {
	current_gc->text.fontid = n;
  }
  if (size >= 0) {
	current_gc->text.size = size;
  }
  if (aspect >= 0) {
	current_gc->text.aspect = aspect;
  }
  if (filter >= 0) {
	current_gc->text.filter = !!filter;
  }
}
 
void text_set_properties(fontid_t n, const float size, const float aspect,
						 int filter)
{
  LOCK();
  set_properties(n, size, aspect, filter);
  UNLOCK();
}

int text_set_escape(int n)
{
  int old;
  LOCK();
  old = current_gc->text.escape;
  current_gc->text.escape = n;
  UNLOCK();
  return old;
}

unsigned int text_get_argb(void)
{
  return current_gc->text.argb;
}

void text_get_color(float *a, float *r, float *g, float *b)
{
  const float f = 1.0f/255.0f;
  const draw_argb_t text_argb = current_gc->text.argb;

  if (a) *a = ((text_argb>>24)&255) * f;
  if (r) *r = ((text_argb>>16)&255) * f;
  if (g) *g = ((text_argb>> 8)&255) * f;
  if (b) *b = ((text_argb    )&255) * f;
}

void text_set_argb(draw_argb_t argb)
{
  current_gc->text.argb = argb;
}

void text_set_color(const float a, const float r, const float g, const float b)
{
  current_gc->text.argb = draw_color_4float_to_argb(a,r,g,b);
}
