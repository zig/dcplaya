/**
 * $Id: vmu68.c,v 1.11 2003-01-03 07:06:32 ben Exp $
 */
#include "config.h"

#include <dc/vmu.h>
#include <dc/maple.h>

#include <string.h>
#include <stdio.h>

#include "sysdebug.h"

#include "playa_info.h"

/* Update the VMU LCD */
#include  "vmu_sc68.h"
#include  "vmu_font.h"

#include "fft.h"
/*#include "vupeek.h"*/
#include "option.h"
#include "playa.h"
#include "math_int.h"

static fftbands_t * bands, * bands6;
static char vmu_text_str[256];

static const char vmu_scrolltext[] =
"            "
"dcplaya " DCPLAYA_VERSION_STR " - "
"Ultimate music player - "
"(c)2002 Benjamin Gerard - "
"            "
"Website:"
"            "
DCPLAYA_URL
"            "
"Credits:"
"            "
"Code & Art works:"
"            "
"Benjamin Gerard"
"            "
"Additionnal Coding:"
"            "
"Vincent Penne"
"            "
"3trd party developers:"
"            "
"They are too many to be listed here. "
"See README files and plugin options. "
"            "
"Greetings:"
"            "
"KOS developers - "
"zlib developers - "
"mikmod developers - "
"sidplay developers - "
"ogg-vorbis developers - "
"xing mpeg developers - "
"other involved developers - "
"sashipa members...";

static int scroll = 0, invert = 0;
static int last_valid = 0;
static int fontInit = 0;
static uint8 font[40][64 / 8];

static int titleInit = 0;
static uint8 title[32][48 / 8];
static uint8 vmutmp[32][48 / 8];
static int vmu_visual = OPTION_LCD_VISUAL_FFT;
static int vmu_db = 1;

/* From dreamcast68.c */
extern int dreamcast68_isplaying(void);
char songmenu_selected[64];	//songmenu.c
extern char option_str[];				// option.c

static int addr_xy(int x, int y, int w, int h)
{
  x = (w - x - 1);
  y = (h - y - 1);

  return (((y * (w >> 3)) + (x >> 3)) << 1) + ((x >> 2) & 1);
}

/* return nibble addr */
static int char_addr(int c)
{
  c &= 127;

  return addr_xy((c & 15) << 2, (c >> 4) * 5, 64, 40);
}


static void put_byte_char(uint8 * dst, int c, int xs, int xe)
{
  int msk = (c & 0x80) ? (' ' ^ '.') : 0;
  int srccol, srclig;
  int i;

  c &= 127;

  srccol = (c & 15) << 2;
  srclig = (c >> 4) * 5;
  for (i = 0; i < 4; ++i, dst += 48, ++srclig) {
    char *d = dst;
    int j;
    for (j = xs; j < xe; ++j) {
      *d++ = vmu_font[srclig][srccol + j] ^ msk;
    }
  }
}

static void putchar(uint8 * dst, int x, int y, int c)
{
  uint8 *src = font[0];
  int dstoff = addr_xy(x << 2, y, 48, 32);
  int srcoff = char_addr(c);
  int i;

  int msk = (dstoff & 1) ? 0xF0 : 0x0F;

  dst += dstoff >> 1;
  src += srcoff >> 1;

  if ((dstoff ^ srcoff) & 1) {
    if (srcoff & 1) {
      for (i = 0; i < 4; ++i, dst -= 6, src -= 8) {
	*dst = (*dst & msk) | (*src << 4);
      }
    } else {
      for (i = 0; i < 4; ++i, dst -= 6, src -= 8) {
	*dst = (*dst & msk) | (*src >> 4);
      }
    }
  } else {
    for (i = 0; i < 4; ++i, dst -= 6, src -= 8) {
      *dst = (*dst & msk) | (*src & ~msk);
    }
  }
}

static void putstr(uint8 * dest, int x, int y, const char *s)
{
  int c;

  if (x < 0)
    x = 0;
  if (y < 0)
    y = 0;

  while ((c = *s++), c) {
    if (x >= 12) {
      x = 0;
      y += 5;
      if (y >= 6 * 5) {
	break;
      }
    }
    putchar(dest, x, y, c);
    ++x;
  }
}

static void vmu_create_bitmap(uint8 * dest, char *src[], int w, int h)
{
  int y, offline = (w + 7) >> 3;

  memset(dest, 0, offline * h);

  for (y = 0; y < h; ++y, dest += offline) {
    int x;

    for (x = 0; x < w; ++x) {
      int xi = x >> 3;
      int xb = 0x80 >> (x & 7);

      if (src[h - y - 1][w - x - 1] != '.') {
	dest[xi] |= xb;
      }
    }
  }
}

static void vmu_create_bitmap_2(uint8 * dest, char *src, int w, int h)
{
  int y, offline = (w + 7) >> 3;

  memset(dest, 0, offline * h);

  for (y = 0; y < h; ++y, dest += offline) {
    int x;

    for (x = 0; x < w; ++x) {
      int xi = x >> 3;
      int xb = 0x80 >> (x & 7);

      if (src[(h - y - 1) * w + w - x - 1] != '.') {
	dest[xi] |= xb;
      }
    }
  }
}

int vmu68_init(void)
{
  static fftband_limit_t limits[] = {
    { 0,    125   },
    { 125,  250   },
    { 250,  500   },
    { 500,  1000  },
    { 1000, 2000  },
    { 2000, 44100 },
  }; 

  /* Create font bitmap */
  if (!fontInit) {
    vmu_create_bitmap(font[0], vmu_font, 64, 40);
    fontInit = 1;
  }

  /* Create title page bitmap */
  if (!titleInit) {
    vmu_create_bitmap(title[0], vmu_sc68, 48, 32);
    titleInit = 1;
  }

  bands  = fft_create_bands(48,0);
  bands6 = fft_create_bands(6,limits);

  memset(vmu_text_str,0,sizeof(vmu_text_str));
  vmu_visual = OPTION_LCD_VISUAL_FFT;

  return 0;
}

void vmu_set_text(const char *s)
{
  vmu_text_str[0] = 0;
  if (s) {
    const int max = sizeof(vmu_text_str) - 1;
    int i;
    for (i=0; i<max; ++i) {
      int c = s[i];
      if (!c) break;
      vmu_text_str[i+1] = 0;
      vmu_text_str[i] = c;
    }
  }  
}

int vmu_set_visual(int visual)
{
  int old = vmu_visual;
  if (visual >= 0) {
    vmu_visual = visual;
  }
  return old;
}

int vmu_set_db(int db)
{
  int old = vmu_db;
  if (db >= 0) {
    vmu_db = !!db;
  }
  return old;
}

static int find_sign_change(int *spl, int n)
{
  int i, v;
  v = spl[i = 0] >> 16;

  for (++i; i < n; ++i) {
    int w = spl[i] >> 16;
    if ((v < 0) && w >= 0)
      return i;
    v = w;
  }
  return 0;
}

extern short int_decibel[];

static void draw_samples(char *buf)
{
  short spl[48];
  int x;

  fft_fill_pcm(spl,48);

  for (x = 0; x < 48 / 8; ++x) {
    buf[48 + x] = 0xff;
  }

  for (x = 0; x < 48; ++x) {
    int h = spl[47-x];
    int xi = x >> 3;
    int xb = 0x80 >> (x & 7);
    int o1, o2;
    if (h<0) {
      h = h+1;
      if (vmu_db) {
	h = -int_sqrt(-h << 15);
      }
    } else if (vmu_db) {
      h = int_sqrt(h << 15);
    }
    h = h >> 12; // -8 - 8
    if (h<-8 || h>8) {
      SDWARNING("H:%d\n", h);
      h = h < 0 ? -8 : 8;
    }
    o1 = xi + (48 / 8) * (h + 8);
    o2 = xi + (48 / 8) * (8);
		
    if (o1 < o2) {
      o1 ^= o2;
      o2 ^= o1;
      o1 ^= o2;
    }

    while (o1 > o2) {
      buf[o1] |= xb;
      o1 -= 48 / 8;
    }
  }

}

static struct {
  int v;    /* height of bar */
  int hat;  /* Height of hat */
  int cnt;  /* Sticky hat count down */
  int fall; /* fall speed */
  int dummy;
} fft_bar[48];

static void draw_band(unsigned char * buf)
{
  static int old[6];
  static int val[6];
  static int fall[6];
  const unsigned int maxmin = 100;
  static unsigned int max = maxmin;
  int i,mask;
  const int fall_speed = 500;

  if (!bands6) {
    return;
  }

  fft_fill_bands(bands6);

  for (i=0; i<6; ++i) {
    int v = bands6->band[i].v;
    if (v<0) {
      v = 0;
    } else if (v>32767) {
      v = 32767;
    }
    val[i] = v;
    if (v > max) max = v;
  }
  if (max<maxmin) {
    max = maxmin;
  }
  
  for (i=0; i<6; ++i) {
    int xi = 5-i;
    int v,w;

    v = val[i];

    /* New entry */
    if (vmu_db) {
      v -= 100;
      if (v<0) v=0;
      v = int_decibel[v>>3];
    } else {
      v = (v<<15) / max;
    }

    if (v >= old[i]) {
      fall[i] = 0;
    } else {
      fall[i] += fall_speed;
      w = old[i] - fall[i];
      if (w <= v) {
	w = v;
      }
      v = w;
    }
    old[i] = v;
    v = (v * 20) >> 15;
    if (v>20) v=20;

    buf[xi] = 0xfe;
    mask = 0xd6;
    for (w=1, xi += 48/8; w<v; ++w, xi += 48/8, mask ^= 0x7c) {
      buf[xi] = mask;
    }
    buf[xi] = 0xfe;
  }
  if (max > 0) --max;
}

static void draw_fft(unsigned char * buf)
{
  const int g = 27;
  const int shift = 11;
  int x;

  if (!bands) {
    return;
  }

  fft_fill_bands(bands);
  
  for (x = 47; x >= 0; --x) {
    int xi = x >> 3;
    int xb = 0x80 >> (x & 7);
    int v;
    int o1, hat;

    /* New entry */
    v = bands->band[47-x].v - 100;
    if (v<0) v=0;
    else if (v > 32767) v = 32767;
    v = int_decibel[v>>3];
		
    /* Smoothing */
    if (v > fft_bar[x].v) {
      v = (v + fft_bar[x].v) >> 1;
    } else {
      v = ((fft_bar[x].v * 7) + v) >> 3;
    }
		
    hat = fft_bar[x].hat;
    if (v > hat) {
      fft_bar[x].cnt = 0;
      hat = v;
      if (v <= fft_bar[x].v) {
	/* descending */
	fft_bar[x].fall = 0;
      } else {
	/* accending */
	const int min = -g * 70;
	int dif = v - fft_bar[x].v;
        int fall;

	fall = fft_bar[x].fall;
	if (fall > 0) fall = 0;
	fall += -(dif * g * 4) >> shift;
	if (fall < min) fall = min;
	fft_bar[x].fall = fall;
      }
    } else {
      /* Hat is falling ! */
      int fall, cnt;

      fall = fft_bar[x].fall;
      cnt = fft_bar[x].cnt;
      if (!cnt) {
	/* No tempo */
	int prevfall = fall;
	fall += g;
	if (prevfall < 0 && fall >= 0) {
	  /* Just reach the top */
	  fall = 0;
	  cnt = 10;
	}
      } else {
	/* Wait a while */
	--cnt;
      }

      hat -= fall;
      if (hat <= v) {
	hat = v;
	fall = 0;
	cnt = 0;
      }
      fft_bar[x].fall = fall;
      fft_bar[x].cnt = cnt;
    }
  
    if (hat < 0) {
      hat = 0;
    }  else if (hat > (28<<shift)) {
      hat = 28<<shift;
    }

    if (v > (20<<shift)) {
      v = (20<<shift);
    }
    
    fft_bar[x].hat  =   hat;
    hat             >>= shift;
    fft_bar[x].v    =   v;
    v               >>= shift;
    
    o1 = xi;
    buf[xi+(48/8)*(hat)] |= xb;

    while (v--) {
      buf[o1] |= xb;
      o1 += 48 / 8;
    }

  }
}


static int draw_bar(char *buf, int lvl, int h)
{
  //  char *bufe = buf + 48 / 8;
  int i, j;

  if (lvl > 48)	lvl = 48;

  i = (lvl >> 3);
  while (i--) {
    int o;
    for (o = j = 0; j < h; ++j, o += 48 / 8) {
      buf[o] = 0xee;
    }
    buf++;
  }

  i = lvl & 7;
  if (i) {
    static int msk[] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe };
    int o, v = msk[i];
    for (o = j = 0; j < h; ++j, o += 48 / 8) {
      buf[o] = v & 0xee;
    }
    buf++;
  }
}

static void draw_lcd(void *a)
{
  uint8 mlcd = maple_first_lcd();

  if (a && mlcd) {
    vmu_draw_lcd(mlcd, a);
  }
}

void vmu_lcd_title()
{
  draw_lcd(title[0]);
}

void vmu_lcd_update(/*int *spl, int nbSpl, int splFrame*/)
{
  const char *info_str = vmu_scrolltext;

  //  const int spd = 8;
  playa_info_t *info;

  info = playa_info_lock();

  if (info->valid && info->info[PLAYA_INFO_VMU].s) {
    info_str = info->info[PLAYA_INFO_VMU].s;
  }

  if (info->valid != last_valid) {
    last_valid = info->valid;
    scroll = 0;
    invert = 0;
  }

  memset(vmutmp, 0, sizeof(vmutmp));

  if (1) {
    const int spd = 64;
    char tmp[48 * 5];
    int col, idx, x, c, rem, scr;

    scr = (scroll * spd) >> 8;

    col = scr & 3;
    idx = scr >> 2;

    x = 0;
    if ((c = info_str[idx++])) {
      put_byte_char(tmp, c, col, 4);
      x = 4 - col;
      while ((48 - x) >= 4 && (c = info_str[idx++])) {
	put_byte_char(tmp + x, c, 0, 4);
	x += 4;
      }

      rem = 48 - x;
      if (c && rem && (c = info_str[idx++])) {
	put_byte_char(tmp + x, c, 0, rem);
	x += rem;
      }

      ++scroll;
    } else {
      scroll = 0;
    }

    while (x < 48) {
      tmp[x + 0 * 48] = tmp[x + 1 * 48] =
	tmp[x + 2 * 48] = tmp[x + 3 * 48] = tmp[x + 4 * 48] = '.';
      ++x;
    }
    vmu_create_bitmap_2(vmutmp[31 - 3], tmp, 48, 4);
  }
  playa_info_release(info);

  /* Display VU-meters */
/*   if (1) { */
/*     int bar_1 = (peek1.dyn * 48) >> 16; */
/*     //    int bar_2 = (peek2.dyn * 48) >> 16; */
/*     int bar_3 = (peek3.dyn * 48) >> 16; */

/*     /\*  draw_bar(vmutmp[31-3], bar_1, 2); *\/ */
/*     draw_bar(vmutmp[31 - 6], bar_1, 2); */
/*     draw_bar(vmutmp[31 - 9], bar_3, 2); */
/*   } */

  /* Display either OPTION or SONG-MENU-ENTRY */
  if (vmu_text_str[0]) {
    putstr(vmutmp[0], 0, 1 * 5 + 1, vmu_text_str);
  }
	
  if (1) {
    switch (vmu_visual) {
    case OPTION_LCD_VISUAL_SCOPE:
      draw_samples(vmutmp[0]);
      break;
    case OPTION_LCD_VISUAL_FFT:
      draw_fft(vmutmp[0]);
      break;
    case OPTION_LCD_VISUAL_BAND:
      draw_band(vmutmp[0]);
      break;
    }
  }

  /* Finally copy temporary buffer to VMU LCD */
  draw_lcd(vmutmp[0]);
}
