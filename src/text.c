/* 2002/02/11 */

#include <stdarg.h>
#include <stdio.h>
#include "gp.h"



#define CLIP(a, min, max) ((a)<(min)? (min) : ((a)>(max)? (max) : (a)))



static uint32 fnttexture = 0;
static float xscale;
static float yscale;

static float text_xscale;
static float text_yscale;

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
  float u1, u2, v1, v2;
} myglyph_t;

static myglyph_t myglyph[128];

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
        bounding(g + x*FONT_CHAR_W + y*FONT_TEXTURE_W*FONT_CHAR_H, FONT_CHAR_W, FONT_CHAR_H, FONT_TEXTURE_W, b);
        
        if (c<128) {
          myglyph_t *m = myglyph+c;
          
          m->w = (float)(b[2] - b[0] + 2);
          m->u1 = (float)(x*FONT_CHAR_W+b[0]) * xs;
          m->u2 = (float)(x*FONT_CHAR_W+b[2]+1) * xs;
          m->v1 = (float)y * yscale;
          m->v2 = m->v1 + yscale;
          /*
	    dbglog(DBG_DEBUG, "%02x '%c' w:%.2f [%.2f %.2f %.2f %.2f]\n", c, c, 
            m->w, m->u1, m->v1, m->u2, m->v2);
          */
        }
      }
    }
    
    /* Special case for char 4[pad] & 6[joy] */
    for (c=4; c<8; c+=2) {
      myglyph[c].u1 = (myglyph[c+16  ].u1 < myglyph[c     ].u1)  ? myglyph[c+16  ].u1 : myglyph[c     ].u1;
      myglyph[c].v1 = (myglyph[c+ 1  ].v1 < myglyph[c     ].v1)  ? myglyph[c+ 1  ].v1 : myglyph[c     ].v1;
      myglyph[c].u2 = (myglyph[c+1+16].u2 > myglyph[c+1   ].u2)  ? myglyph[c+1+16].u2 : myglyph[c+1   ].u2;
      myglyph[c].v2 = (myglyph[c+16  ].v2 > myglyph[c+1+16].v2)  ? myglyph[c+16  ].v2 : myglyph[c+1+16].v2;
      myglyph[c].w  = 32.0f;
    }
  }
	
  return 0;
}


static float size_of_char(const float scalex, int c)
{
  if ((unsigned int)c >= 128u) {
    c = 0;
  }
  return myglyph[c].w * scalex;
}

static float size_of_str(const float scalex, const char *s)
{
  int c;
  float sum = 0;
  while ((c=*s++), c) {
    if ((unsigned int)c >= 128u) {
      c = 0;
    }
    sum +=  myglyph[c].w;
  }
  return sum * scalex;
}

/* Draw one font character (16x16); assumes polygon header already sent */
static float draw_text_char(float x1, float y1, float z1,
                            float scalex, float scaley,
                            float a, float r, float g, float b,
                            int c)
{
  float hc = (float)FONT_CHAR_H * scaley;
  float wc;
  int ix,iy;
  float u1,u2,v1,v2;
	
  typedef struct s_v_t {
    int flags;
    float x,y,z,u,v;
    unsigned int col;
    unsigned int addcol;
  } v_t;

  volatile v_t * const hw = 0xe0<<24;

#ifndef USE_FONT_16x16
  c -= 32;
#endif
  if (c < 0 || c >= 128) c = 0;
	
  ix = c % (FONT_TEXTURE_W/FONT_CHAR_W);
  iy = c / (FONT_TEXTURE_W/FONT_CHAR_W);
  u1 = (float)ix * xscale;
  u2 = u1 + xscale;
  v1 = (float)iy * yscale;
  v2 = v1 + yscale;
	
  u1 = myglyph[c].u1;	
  u2 = myglyph[c].u2;	
  v1 = myglyph[c].v1;	
  v2 = myglyph[c].v2;	
  wc = myglyph[c].w * scalex;
  
  if (c==4 || c==6) {
    y1 -= hc * 0.5f;
    hc *= 2.0f;
  }
  

  //  dbglog(DBG_DEBUG, "'%c' : %d,%d %.2f,%.2f,%.2f,%.2f\n", c, ix,iy, u1,v1,u2,v2);

  hw->flags = TA_VERTEX_NORMAL;
  hw->x = x1;
  hw->y = y1 + hc;
  hw->z = z1;
  hw->u = u1;
  hw->v = v2;
  hw->col =
    (((unsigned char)(255.0f * a)) << 24) +
    (((unsigned char)(255.0f * r)) << 16) +
    (((unsigned char)(255.0f * g)) << 8) +
    (((unsigned char)(255.0f * b)) << 0);
  hw->addcol = 0;
  ta_commit32_nocopy();
	
  hw->x = x1;
  hw->y = y1;
  hw->u = u1;
  hw->v = v1;
  ta_commit32_nocopy();
	
  hw->x = x1 + wc;
  hw->y = y1 + hc;
  hw->u = u2;
  hw->v = v2;
  ta_commit32_nocopy();

  hw->flags = TA_VERTEX_EOL;
  hw->x = x1 + wc;
  hw->y = y1;
  hw->u = u2;
  hw->v = v1;
  ta_commit32_nocopy();
	
  return x1 + wc;
}

  

/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
static float draw_poly_textv(float x1, float y1, float z1,
			     float a, float r, float g, float b,
			     const char *s, va_list list)
{
  poly_hdr_t poly;
  int esc = 0, c;
  float max_yscale = text_yscale;
  int filter = (text_yscale == 1.0f && text_xscale == 1.0f) ? TA_NO_FILTER : TA_BILINEAR_FILTER;



  a = CLIP(a, 0, 1);
  r = CLIP(r, 0, 1);
  g = CLIP(g, 0, 1);
  b = CLIP(b, 0, 1);


  ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, FONT_TEXTURE_W, FONT_TEXTURE_H, fnttexture, filter);
  poly.flags1 &= ~(3<<4);
  ta_commit_poly_hdr(&poly);
  while (c=*s++, c) {
    if (esc) {
      esc = 0;
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
	c = 0;
	a = (float)va_arg(list, int) * (1.0f/255.0f);
	a = CLIP(a, 0, 1);
	break;
      case 'r':
	c = 0;
	r = (float)va_arg(list, int) * (1.0f/255.0f);
	r = CLIP(r, 0, 1);
	break;
      case 'g':
	c = 0;
	g = (float)va_arg(list, int) * (1.0f/255.0f);
	g = CLIP(g, 0, 1);
	break;
      case 'b':
	c = 0;
	b = (float)va_arg(list, int) * (1.0f/255.0f);
	b = CLIP(b, 0, 1);
	break;
      case 'c':
	{
	  unsigned int argb = va_arg(list, unsigned int);
	  c = 0;
	  a = (float)((argb>>24)&255) * (1.0f/256.0f);
	  r = (float)((argb>>16)&255) * (1.0f/256.0f);
	  g = (float)((argb>> 8)&255) * (1.0f/256.0f);
	  b = (float)((argb    )&255) * (1.0f/256.0f);
	  a = CLIP(a, 0, 1);
	  r = CLIP(r, 0, 1);
	  g = CLIP(g, 0, 1);
	  b = CLIP(b, 0, 1);
	} break;
      case 's':
	{
	  c = 0;
	  text_xscale = text_yscale = (float)va_arg(list, int) * (1.0f/(float)FONT_CHAR_W);
	} break;
      default:
	dbglog(DBG_DEBUG, "** " __FUNCTION__ " weird escape char : 0x%02X\n",c);
      }
    } else if (c=='%') {
      c = 0;
      esc = 1;
    }
	  
    if (c == ' ') {
      x1 += myglyph[32].w;
    } else if (c) {
      x1 = draw_text_char(x1, y1, z1, text_xscale, text_yscale, a, r, g, b, c);
    }
 		
    if (text_yscale > max_yscale) {
      max_yscale = text_yscale;
    }
		
  }
  return (float)FONT_CHAR_H * max_yscale;
}


/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float draw_poly_text(float x1, float y1, float z1,
                     float a, float r, float g, float b,
                     const char *s, ...)
{
  va_list list;
  float res;

  va_start(list, s);
  res = draw_poly_textv(x1, y1, z1, a, r, g, b, s, list);
  va_end(list);
  return res;
}

/* */
float draw_poly_center_text(float y1, float z1,
                            float a, float r, float g, float b,
                            const char *s, ...)
{
  const float border=30.0f;
  const float screen=640.0f-2.0f*border;
  
  float sum = 0.0f, x1;
  int esc = 0, c;
  const char *s2 = s;
  va_list list;
  float res;

  /* $$$ Don't work with scale change !!! */
  while ((c=*s++), c) {
    if (esc) {
      esc = 0;
      continue;
    }
    if (c == '%') {
      esc = 1;
      continue;
    }
  
    if ((unsigned int)c >= 128u) {
      c = 0;
    }
    sum += myglyph[c].w * text_xscale;
  }
  
  x1 = border;
  x1 += (screen - sum) * 0.5f;
  
  s = s2;
  va_start(list, s);
  res = draw_poly_textv(x1, y1, z1, a, r, g, b, s, list);
  va_end(list);
  return res;
}


void draw_poly_get_text_size(const char *s, float * w, float * h)
{
  *w = size_of_str(text_xscale, s);
  *h = FONT_CHAR_H * text_yscale;
}

/* Draw a set of textured polygons at the given depth and color that
   represent a string of text. */
float draw_poly_layer_text(float y1, float z1, const char *s)
{
  const float border=30.0f;
  const float screen=640.0f-2.0f*border;
  float scale = 2.0f;
  poly_hdr_t poly;
  int c;
  float strw, x1=border;
	
  if (!s) {
    return 0.0f;
  }
	
  strw = size_of_str(scale, s);
  if (strw > screen) {
    scale = scale * screen / strw; 
  } else {
    x1 += (screen - strw) * 0.5f;
  }

  ta_poly_hdr_txr(&poly, TA_TRANSLUCENT, TA_ARGB4444, 
		  FONT_TEXTURE_W, FONT_TEXTURE_H, fnttexture, TA_NO_FILTER);
  poly.flags1 &= ~(3<<4);
  ta_commit_poly_hdr(&poly);
  while (c=*s++, c) {
    if (c == ' ') {
      x1 += myglyph[32].w * scale;
    } else if (c) {
      x1 = draw_text_char(x1, y1, z1, scale, scale, 1.0f, 0, 1.0f, 0.0f, c);
    }
		
  }
  return (float)FONT_CHAR_H * scale;
}

float text_set_font_size(float size)
{
  float save = text_xscale * (float)FONT_CHAR_W;
  text_xscale = text_yscale = size / (float)FONT_CHAR_W;
  return save;
}
