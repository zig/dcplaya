/* 2002/02/11 */

#include <stdarg.h>
#include "sysdebug.h"
#include "gp.h"

#define CLIP(a, min, max) ((a)<(min)? (min) : ((a)>(max)? (max) : (a)))



static uint32 fnttexture = 0;
static float xscale;
static float yscale;

static float text_xscale;
static float text_yscale;
static unsigned int text_argb;

static float text_xscale_save;
static float text_yscale_save;
static unsigned int text_argb_save;

//static float text_color_a, text_color_r, text_color_g, text_color_b;

/* define me to use 16x16 instead of 8x12 font */
#define USE_FONT_16x16 1

#ifdef USE_FONT_16x16
# define FONT_TEXTURE_W 256
# define FONT_TEXTURE_H 128
# define FONT_CHAR_W 16
# define FONT_CHAR_H 16
#else
# define FONT_TEXTURE_W 128
# define FONT_TEXTURE_H 128
# define FONT_CHAR_W 8
# define FONT_CHAR_H 12
#endif

typedef struct {
  float w;
  float h;
  float u1, u2, v1, v2;
} myglyph_t;

static myglyph_t myglyph_p[128]; // proportional spacing
static myglyph_t myglyph_f[128]; // fixed spacing
static myglyph_t * myglyph = myglyph_p;

static int escape_char = '%';

static float clipbox[4];

static int do_escape(int c, va_list * list);

static void bounding(uint8 *img, int w, int h, int bpl, int *box)
{
  int x,y;
  uint8 *b;

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
  
  return;
}

static void make_blk(uint16 *texture, int w, int h, uint8 *g, int ws)
{
  while (h--) {
    int x;
    
    for (x=0; x<w; ++x) {
      int v = *g++;
      v = (v>>4)&15;
      v = v | (v<<4);
      v = v | (v<<8);
      *texture++ = v;
    }
    g += ws-w;
  }
} 

int text_setup()
{
  uint16 *texture;
  uint32 h;
  uint8  *g;
  const char *fontname = 
#ifdef USE_FONT_16x16
    "/rd/font16x16.ppm";
#else  
  "/rd/font8x12.ppm";
#endif
	
  /* set clip box */
  clipbox[0]=0;
  clipbox[1]=0;
  clipbox[2]=640;
  clipbox[3]=480;
  text_argb = 0xFFFFFFFF;

  h = fs_open(fontname, O_RDONLY);
  g = fs_mmap(h) + fs_total(h) - FONT_TEXTURE_W*FONT_TEXTURE_H;
  fnttexture = ta_txr_allocate(FONT_TEXTURE_W*FONT_TEXTURE_H*2);
  texture = (uint16*)ta_txr_map(fnttexture);
  make_blk(texture, FONT_TEXTURE_W, FONT_TEXTURE_H, g, FONT_TEXTURE_W);
  if (h) fs_close(h);
	
  xscale = (float)FONT_CHAR_W / (float)FONT_TEXTURE_W;
  yscale = (float)FONT_CHAR_H / (float)FONT_TEXTURE_H;
  text_xscale = text_yscale = 1.0f;
  
  {
    int x,y,c;
    int b[4];
    
    float xs = 1.0f / (float)FONT_TEXTURE_W;
    
    for (c=y=0; y<FONT_TEXTURE_H/FONT_CHAR_H; ++y) {
      for (x=0; x<FONT_TEXTURE_W/FONT_CHAR_W; ++x,++c) {
        bounding(g + x*FONT_CHAR_W + y*FONT_TEXTURE_W*FONT_CHAR_H,
				 FONT_CHAR_W, FONT_CHAR_H, FONT_TEXTURE_W, b);
        
        if (c<128) {
          myglyph_t *m = myglyph_p+c;
          
          m->w = (float)(b[2] - b[0] + 2);
		  m->h = FONT_CHAR_H;
          m->u1 = (float)(x*FONT_CHAR_W+b[0]) * xs;
          m->u2 = (float)(x*FONT_CHAR_W+b[2]+1) * xs;
          m->v1 = (float)y * yscale;
          m->v2 = m->v1 + yscale;
          /*
			dbglog(DBG_DEBUG, "%02x '%c' w:%.2f [%.2f %.2f %.2f %.2f]\n", c, c, 
            m->w, m->u1, m->v1, m->u2, m->v2);
          */

          m = myglyph_f+c;
          m->w = (float)(11);
          m->u1 = (float)(x*FONT_CHAR_W+(b[0]+b[2])/2 - 5) * xs;
          m->u2 = (float)(x*FONT_CHAR_W+(b[0]+b[2])/2 + 6-1) * xs;
          m->v1 = (float)y * yscale;
          m->v2 = m->v1 + yscale;

        }
      }
    }
    
    /* Special case for char 4[pad] & 6[joy] */
    for (c=4; c<8; c+=2) {
      for (myglyph = myglyph_p ; 
		   myglyph; 
		   myglyph = (myglyph==myglyph_p? myglyph_f : 0) ) {
		myglyph[c].u1 = (myglyph[c+16  ].u1 < myglyph[c     ].u1)  ? 
		  myglyph[c+16  ].u1 : myglyph[c     ].u1;
		myglyph[c].v1 = (myglyph[c+ 1  ].v1 < myglyph[c     ].v1)  ? 
		  myglyph[c+ 1  ].v1 : myglyph[c     ].v1;
		myglyph[c].u2 = (myglyph[c+1+16].u2 > myglyph[c+1   ].u2)  ? 
		  myglyph[c+1+16].u2 : myglyph[c+1   ].u2;
		myglyph[c].v2 = (myglyph[c+16  ].v2 > myglyph[c+1+16].v2)  ? 
		  myglyph[c+16  ].v2 : myglyph[c+1+16].v2;
		myglyph[c].w  = 32.0f;
		myglyph[c].h  = 32.0f;
      }
    }
  }
	
  return 0;
}

static void save_state(void)
{
  text_xscale_save = text_xscale;
  text_yscale_save = text_yscale;
  text_argb_save   = text_argb;
}

static void restore_state(void)
{
  text_xscale = text_xscale_save;
  text_yscale = text_yscale_save;
  text_argb   =  text_argb_save;
}


static float size_of_char(float * h, int c)
{
  if ((unsigned int)c >= 128u) {
    c = 0;
  }
  if (h) *h = myglyph[c].h * text_yscale;
  return myglyph[c].w * text_xscale;
}

float measure_poly_char(int c)
{
  return (size_of_char(0, c));
}

static float size_of_str(float * h, const char *s)
{
  int c;
  float sum = 0, max_h = 0;
  while ((c=*s++), c) {
    if ((unsigned int)c >= 128u) {
      c = 0;
    }
	if (myglyph[c].h > max_h) max_h = myglyph[c].h;
    sum += myglyph[c].w;
  }
  if (h) *h = max_h * text_yscale;
  return sum * text_xscale;
}

static float size_of_strf(float *h, const char *s, va_list list)
{
  int c, esc = 0;
  float sum = 0, maxh = 0;

  save_state();
  while ((c=*s++), c) {
	if (esc) {
	  c = do_escape(c, &list);
	  esc = 0;
	} else if (c==escape_char) {
      esc = 1;
	  c = -1;
    } else if ((unsigned int)c >= 128u) {
		c = 0;
	}
	if (c == -1) continue;
	if ((unsigned int)c >= 128u) c = 0;
	{
	  float ch = myglyph[c].h * text_yscale;
	  if (ch > maxh) maxh = ch;
	  sum += myglyph[c].w * text_xscale;
	}
  }
  restore_state();
  if (h) *h = maxh;
  return sum;
}

float measure_poly_text(const char * s)
{
  return size_of_str(0, s);
}

float measure_poly_vtextf(const char * s, va_list list)
{
  return size_of_strf(0, s, list);
}

float measure_poly_textf(const char * s, ...)
{
  float h;
  va_list list;
  va_start(list,s);
  h = size_of_strf(0, s, list);
  va_end(list);

  return h;
}

/* Draw one font character (16x16); assumes polygon header already sent */
static float draw_text_char(float x1, float y1, float z1,
                            float scalex, float scaley,
/*                             float a, float r, float g, float b, */
                            int c)
{
  float hc = (float)FONT_CHAR_H * scaley;
  int ix,iy;
  float u1,u2,v1,v2;
  float x2,y2;
	
  typedef struct s_v_t {
    int flags;
    float x,y,z,u,v;
    unsigned int col;
    unsigned int addcol;
  } v_t;

  volatile v_t * const hw = (v_t *)(0xe0<<24);

#ifndef USE_FONT_16x16
  c -= 32;
#endif
/*   if ((unsigned int)c >= 128) c = 0; */

  if (c==4 || c==6) {
    y1 -= hc * 0.5f;
    hc *= 2.0f;
  }

  if (x1 >= clipbox[2] || y1 >= clipbox[3]) {
	return 0;
  }
  x2 = x1 + myglyph[c].w * scalex;
  y2 = y1 + hc;
  if (x2 < clipbox[0] || y2 < clipbox[1]) {
	return 0;
  }
	
  ix = (unsigned int)c % (FONT_TEXTURE_W/FONT_CHAR_W);
  iy = (unsigned int)c / (FONT_TEXTURE_W/FONT_CHAR_W);
  u1 = (float)ix * xscale;
  u2 = u1 + xscale;
  v1 = (float)iy * yscale;
  v2 = v1 + yscale;
	
  u1 = myglyph[c].u1;
  u2 = myglyph[c].u2;
  v1 = myglyph[c].v1;
  v2 = myglyph[c].v2;
  
  

  //  dbglog(DBG_DEBUG, "'%c' : %d,%d %.2f,%.2f,%.2f,%.2f\n", c, ix,iy, u1,v1,u2,v2);

  hw->flags = TA_VERTEX_NORMAL;
  hw->x = x1;
  hw->y = y1 + hc;
  hw->z = z1;
  hw->u = u1;
  hw->v = v2;
  hw->col = text_argb;
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
	
  return x2;
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
	text_argb = (text_argb & 0x00FFFFFF) | (va_arg(*list, int)<<24);
	break;
  case 'r':
	c = -1;
	text_argb = (text_argb & 0xFF00FFFF) | (va_arg(*list, int)<<16);
	break;
  case 'g':
	c = -1;
	text_argb = (text_argb & 0xFFFF00FF) | (va_arg(*list, int)<<8);
	break;
  case 'b':
	c = -1;
	text_argb = (text_argb & 0xFFFFFF00) | va_arg(*list, int);
	break;
  case 'c':
	c = -1;
	text_argb = va_arg(*list, unsigned int);
	break;
  case 's':
	{
	  c = -1;
	  text_xscale = text_yscale =
		(float)va_arg(*list, int) * (1.0f/(float)FONT_CHAR_W);
	} break;
  default:
	SDWARNING("[%s] : weird escape char : 0x%02X\n",__FUNCTION__, c);
  }
  return c;
}


/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
static float draw_poly_textv(float x1, float y1, float z1,
/* 			     float a, float r, float g, float b, */
			     const char *s, va_list list)
{
  poly_hdr_t poly;
  int esc = 0, c;
  float maxh = 0;
  int filter = (text_yscale == 1.0f && text_xscale == 1.0f)
	? TA_NO_FILTER
	: TA_BILINEAR_FILTER;

  if (x1>=clipbox[2] || y1>=clipbox[3]) {
	return 0;
  }

  ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, FONT_TEXTURE_W,
				  FONT_TEXTURE_H, fnttexture, filter);
  poly.flags1 &= ~(3<<4);
  ta_commit_poly_hdr(&poly);


  while (c=*s++, c) {
	float curh;



    if (esc) {
      esc = 0;
	  c = do_escape(c, &list);
    } else if (c==escape_char) {
      c = -1;
      esc = 1;
    }
	if (c == -1) continue;

    if (c == ' ') {
      x1 += myglyph[32].w * text_xscale;
	  curh = myglyph[32].h * text_yscale;
    } else {
	  if ((unsigned int)c >= 128) c = 0;
      x1 = draw_text_char(x1, y1, z1, text_xscale, text_yscale, c);
	  curh = myglyph[c].h * text_yscale;
    }

	if (x1>=clipbox[2]) {
	  break;
	}

	if (curh > maxh) maxh = curh;
  }
  return maxh;
}

/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float draw_poly_text(float x1, float y1, float z1,
/*                      float a, float r, float g, float b, */
                     const char *s, ...)
{
  va_list list;
  float res;

  va_start(list, s);
  res = draw_poly_textv(x1, y1, z1, /*a, r, g, b,*/ s, list);
  va_end(list);
  return res;
}


/* */
float draw_poly_center_text(float x1, float y1, float x2, float y2, float z1,
                            const char *s, ...)
{
  float res, w, h;
  va_list list;

  va_start(list, s);
  w =  size_of_strf(&h, s, list);

  x1 = x1 + ((x2-x1)-w) * 0.5f;
  y1 = y1 + ((y2-y1)-h) * 0.5f;
  
  va_start(list, s);
  res = draw_poly_textv(x1, y1, z1, s, list);

  va_end(list);
  return res;
}

void draw_poly_get_text_size(const char *s, float * w, float * h)
{
  *w = size_of_str(h, s);
}

/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float draw_poly_layer_text(float y1, float z1, const char *s)
{
  const float border=30.0f;
  const float screen=640.0f-2.0f*border;
  float scale = 2.0f, h;
  poly_hdr_t poly;
  int c;
  float strw, x1=border;
	
  if (!s) {
    return 0.0f;
  }

  save_state();
  text_xscale = text_yscale = scale;
  strw = size_of_str(&h, s);
  if (strw > screen) {
	const float f = screen / strw;
	scale *= f;
	h *= f;
  } else {
    x1 += (screen - strw) * 0.5f;
  }
  text_argb = 0xFF00FF00;

  ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 
		  FONT_TEXTURE_W, FONT_TEXTURE_H, fnttexture, TA_NO_FILTER);
  poly.flags1 &= ~(3<<4);
  ta_commit_poly_hdr(&poly);
  while (c=*s++, c) {
    if (c == ' ') {
      x1 += myglyph[32].w * scale;
    } else {
	  if ((unsigned int)c >= 128u) c = 0;
      x1 = draw_text_char(x1, y1, z1, scale, scale, c);
    }
  }
  restore_state();
  return h;
}

float text_set_font_size(float size)
{
  float save = text_xscale * (float)FONT_CHAR_W;
  text_xscale = text_yscale = size / (float)FONT_CHAR_W;
  return save;
}

void text_set_clipping(const float xmin, const float ymin,
					   const float xmax, const float ymax)
{
  clipbox[0] = xmin;
  clipbox[1] = ymin;
  clipbox[2] = xmax;
  clipbox[3] = ymax;
}

int text_set_font(int n)
{
  int old = (myglyph == myglyph_f);

  myglyph = n? myglyph_f : myglyph_p;

  return old;
}

int text_set_escape(int n)
{
  int old = escape_char;

  escape_char = n;

  return old;
}

unsigned int text_get_argb(void)
{
  return text_argb;
}

void text_get_color(float *a, float *r, float *g, float *b)
{
  const float f = 1.0f/255.0f;
  if (a) *a = ((text_argb>>24)&255) * f;
  if (r) *r = ((text_argb>>16)&255) * f;
  if (g) *g = ((text_argb>> 8)&255) * f;
  if (b) *b = ((text_argb    )&255) * f;
}

void text_set_color(const float a, const float r, const float g, const float b)
{
  text_argb =
	((int)(255.0f*CLIP(a,0,1))<<24) |
	((int)(255.0f*CLIP(r,0,1))<<16) |
	((int)(255.0f*CLIP(g,0,1))<<8) |
	 (int)(255.0f*CLIP(b,0,1));
}

void text_set_argb(unsigned int argb)
{
  text_argb = argb;
}
