/**
 * $Id: vmu68.c,v 1.3 2002-09-25 03:21:22 benjihan Exp $
 */
#include "config.h"

#include <dc/vmu.h>
#include <dc/maple.h>

#include <string.h>
#include <stdio.h>

#include "playa_info.h"

/* Update the VMU LCD */
#include  "vmu_sc68.h"
#include  "vmu_font.h"

#include "fft.h"
#include "vupeek.h"
#include "option.h"
#include "playa.h"

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

/* From dreamcast68.c */
extern int dreamcast68_isplaying(void);
extern char songmenu_selected[];	//songmenu.c
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

  return 0;
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

static void draw_samples(char *buf, int *spl, int nbSpl)
{
  int i, x, stp;

  for (i = 0; i < 48 / 8; ++i) {
    buf[48 + i] = 0xff;
  }

  if (!spl)
    return;

  //  i = find_sign_change(spl, nbSpl);
  i = 0;

  stp = ((nbSpl - i) << 12) / 48;
  i <<= 12;
  for (x = 0; x < 48; i += stp, ++x) {
    int h = spl[i >> 12];
    int r, l;
    int xi = x >> 3;
    int xb = 0x80 >> (x & 7);
    int o1, o2;

    r = (signed short) h;				// -32768 +32767
    l = h >> 16;
    h = (r + l);								// -65536 +65534
    ++h;
    h >>= 13;										// -8 - 8
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

extern short int_decibel[4096];

static void draw_fft(char *buf, short *R, short *I, int n)
{
  const int g = 27;
  int i, x, stp;

  if (n<=0)
    return;
		
  stp = (n << 12) / 48;
  for (i = 0, x = 47; x >= 0; i += stp, --x) {
    int xi = x >> 3;
    int xb = 0x80 >> (x & 7);
    int v;
    int o1, hat;

    /* New entry */
    v = fft_D[i>>12];
		
    /* Smoothing */
    if (v > fft_bar[x].v) {
      v = (v + fft_bar[x].v) >> 1;
    } else {
      v = ((fft_bar[x].v * 7) + v) >> 3;
    }
		
    hat = fft_bar[x].hat;
    if (v >= hat) {
      if (0) {
	fft_bar[x].cnt = 32;
	fft_bar[x].fall = 0;
      } else if (v > fft_bar[x].v) {
	const int min = -g * 70;
        int fall;
        const int method = 5;
		  
	fft_bar[x].cnt = 0;
	fall = fft_bar[x].fall;
	if (fall>0) fall = 0;
		    
	// $$ hack !!! 
	hat =  fft_bar[x].v;
		    
	switch (method) {
	case 1: /* Trankil */
	  fall = - ((v + (v - hat)) >> 5);
	  break;
	case 2: /* Dynamic mais pas trop  */
	  fall = -(((v + (v - hat)) * g) >> 10);
	  break;
	case 3: /* Ca monte haut ! */
	  fall += -(((v + (v - hat)) * g) >> 12);
	  break;
	case 4:
	  fall += -((((v>>1) + (v - hat)) * g) >> 11);
	  break;
	case 5: /* Dynamic mais pas trop  */
	  fall = -(((((v>>1) + (v - hat)) * g) * 3)  >> 11);
	  break;
		      
	default:
	  fall = -g * 50;
	}
	if (fall < min) fall = min;
	fft_bar[x].fall = fall;
      }
      hat = v;
    } else {
      if (fft_bar[x].cnt > 0) {
	fft_bar[x].cnt--;
      }
      if (!fft_bar[x].cnt) {
	//  		  const int fall_max = 1024;
	//	  	  const int smooth = 240;
	int fall;
		    
	//		    fall = ((fft_bar[x].fall * smooth) + (fall_max * (256-smooth))) >> 8;
	//		    fall = ((fft_bar[x].fall * smooth) + (fall_max * (256-smooth))) >> 8;
        fall = fft_bar[x].fall + g;
	fft_bar[x].fall = fall;
	hat -= fall;
	if (hat <= 0) {
	  hat = 0;
	  fall = 0;
	}
	else if(hat > (28<<11)) hat = 28<<11;
      }
    }
    fft_bar[x].hat  =   hat;
    hat             >>= 11;
    fft_bar[x].v    =   v;
    v               >>= 11;

    o1 = xi;
    buf[xi+(48/8)*hat] |= xb;
		
    if (v>0) {
      if (v > 15) v = 30;
      do {
	buf[o1] |= xb;
	o1 += 48 / 8;
      } while (--v);
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
  int mlcd = maple_first_lcd();
  if (a && mlcd) vmu_draw_lcd(mlcd, a);
}

void vmu_lcd_title()
{
  draw_lcd(title[0]);
}

void vmu_lcd_update(int *spl, int nbSpl, int splFrame)
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
  if (1) {
    int bar_1 = (peek1.dyn * 48) >> 16;
    //    int bar_2 = (peek2.dyn * 48) >> 16;
    int bar_3 = (peek3.dyn * 48) >> 16;

    /*  draw_bar(vmutmp[31-3], bar_1, 2); */
    draw_bar(vmutmp[31 - 6], bar_1, 2);
    draw_bar(vmutmp[31 - 9], bar_3, 2);
  }

  /* Display either OPTION or SONG-MENU-ENTRY */
  if (1) {
    char *s;
    s = option_str[0] ? option_str : songmenu_selected;
    putstr(vmutmp[0], 0, 2 * 5 + 1, s);
  }
	
  if (1) {
    int option = option_lcd_visual();
	
    if (option != OPTION_LCD_VISUAL_NONE && spl 
	&& nbSpl>0 
	&& playa_isplaying()) {
      if (option == OPTION_LCD_VISUAL_SCOPE) {
	draw_samples(vmutmp[0], spl, nbSpl);
      } else {
        draw_fft(vmutmp[0], fft_R, fft_I,
		 (option == OPTION_LCD_VISUAL_FFT_FULL)
		 ? (1<<(FFT_LOG_2-1))
		 : (1<<(FFT_LOG_2-2))); 

      }
    }
  }
	
  /* Finally copy temporary buffer to VMU LCD */
  draw_lcd(vmutmp[0]);
}
