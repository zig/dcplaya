/**
 * @file    border.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/02/16
 * @brief   border rendering
 */

#include <stdarg.h>
#include <stdio.h>

#include "dcplaya/config.h"
#include "border.h"
#include "syserror.h"
#include "draw/color.h"
#include "translator/SHAtranslator/SHAtranslatorBlitter.h"

static const uint16 mode_colors[][3] = {
  /* border, fill, no-link */
  {0xFFFF, 0xF666, 0xF666}, /* */
  {0xFFFF, 0x8DDD, 0x8DDD}, 
  {0xFFFF, 0x8DDD, 0xE333},
  {0xFFFF, 0x0000, 0x0000},
  {0xFFFF, 0x0000, 0xF000},
  {0xFFFF, 0xF000, 0xF000},
  {0xFFFF, 0x5000, 0x8000},
  {0xFFFF, 0x0000, 0xAFFF},
};

#define N_BORDER (sizeof(mode_colors) / sizeof(*mode_colors))

texid_t bordertex;
static texid_t bordertex_org;
static const unsigned int border_max = N_BORDER;

static void convert_blk(uint16 * dst, uint16 *src, int w, int h, int ws,
			const uint16 colors[])
{
  int y;
  uint16 convert[16];

  for (y=0; y<3; ++y) {
    convert[y] = colors[2];
  }
  for (; y<8; ++y) {
    convert[y] = colors[1];
  }
  for (; y<16; ++y) {
    convert[y] = colors[0];
  }

  ws -= w;
  for (y=0; y<h; ++y) {
    int x;
    
    for (x=0; x<w; ++x) {
      *dst++ = convert[*src++ & 15];
    }
    src += ws;
    dst += ws;
  }
}


static void make_blk(uint16 *texture, int w, int h, int ws, int mode)
{
  convert_blk(texture, texture, w, h, ws, mode_colors[mode%border_max]);
}

int border_customize(texid_t texid, border_def_t def)
{
  int i, err = 0;
  texture_t * torg, * t = 0;
  uint16 color16[3], * org, * dst;
  int worg,horg,lorg;

  /* Convert colors to ARGB4444. */
  for (i=0;i<3;++i) {
    draw_argb_t tmp;
    tmp = draw_color_float_to_argb(def+i);
    ARGB32toARGB4444(color16+i, &tmp, 1);
  }

  /*   SDDEBUG("[%f]:\n" */
  /* 		  "%08x -> %04x\n" */
  /* 		  "%08x -> %04x\n" */
  /* 		  "%08x -> %04x\n", */
  /* 		  border, color16[0], fill, color16[1], link, color16[2]); */

  /* Lock the original border texture the time to get its properties. */
  torg = texture_lock(bordertex_org);
  if (!torg) {
    return -1;
  }
  worg = torg->width;
  horg = torg->height;
  lorg = 1 << torg->wlog2;
  org = torg->addr;
  texture_release(torg);

  /* Lock the custom border texture ...  */
  t = texture_lock(texid);
  if (!t || t->width != worg || t->height != horg || (1<<t->wlog2) != lorg) {
    err = -1;
    goto error;
  }
  dst = t->addr;
  texture_release(t);
  t = 0;
  convert_blk(dst, org, worg, horg, lorg, color16);

 error:
  if (t) {
    texture_release(t);
  }
  return err;
}

void border_get_def(border_def_t def, int n)
{
  const uint16 * color;
  int i;
  color = mode_colors[n %= border_max];
  for (i=0; i<3; ++i) {
    def[i].a = ((color[i] >> 12) & 15) / 15.0f;
    def[i].r = ((color[i] >>  8) & 15) / 15.0f;
    def[i].g = ((color[i] >>  4) & 15) / 15.0f;
    def[i].b = ((color[i] >>  0) & 15) / 15.0f;
  }
}

int border_init(void)
{
  const char *fname = "/rd/bordertile.tga";
  texture_t * t;

  /* Create original border tile. */
  bordertex_org = texture_create_file(fname,"4444");
  /* Create a second version used as standard border. */
  bordertex = texture_dup(bordertex_org, "bordertile2");

  /* Apply texture conversion (+1 for custom). */
  t = texture_lock(bordertex);
  if (t) {
    make_blk(t->addr, t->width, t->height, 1 << t->wlog2, 2);
    texture_release(t);
  }

  return -(!t);
}
