/* 2002/02/16 */

#include <stdarg.h>
#include <stdio.h>
#include "syserror.h"
#include "gp.h"

borderuv_t borderuv[4];
uint32 bordertex , bordertex2;

static void make_blk(uint16 *texture, int w, int h, uint8 *d, int ws, int mode)
{
  int y = 0;
#ifdef RECOLOR  /* Define me to have nice border tiles debug colors ! */
  static uint16 recolor[2][2][3] =
    {
      {
	{256,0,0},
	{0,256,0},
      },
      {
	{0,0,256},
	{256,256,256},
      }
    };
#endif

  for (y=0; y<h; ++y) {
    int x;
    
    for (x=0; x<w; ++x) {
      uint8 c = *d++, r, g, b;
#ifdef RECOLOR
      int xi, yi;
      xi = (x >> 5);
      yi = (y >> 5);
      r = (c * recolor[yi][xi][0]) >> 8;
      g = (c * recolor[yi][xi][1]) >> 8;
      b = (c * recolor[yi][xi][2]) >> 8;
#else
      r = g = b = c;
     
      if (c <= 0x30 && !mode) {
	/* Not linked */
	c = 200; 
	r = g = b = 0x30;
      } else if (c <= 0x80) {
	/* Fill color */
	c = 140;
	r = 230;
	g = 230;
	b = 230;
      } else {
	/* Border color */
	c = r = g = b = 255;
      }
#endif
      {
	int v = (((c)>>4)<<12) | (((r)>>4)<<8) | (((g)>>4)<<4) | ((b)>>4);
	*texture++ = v;
      }
    }
    d += ws-w;
  }
} 

int border_setup()
{
  int err = -1;
  const int w=64, h=64;
  uint32 f;
  uint8  *g;
	
  const char *fname = "/rd/bordertile.ppm";

  SDDEBUG(">>\n");
  sysdbg_indent(1,0);
    
  f = fs_open(fname, O_RDONLY);
  if (!f) {
    STHROW_ERROR(error);
  }
  
  g = fs_mmap(f) + fs_total(f) - w*h;
  if (!g) {
    STHROW_ERROR(error);
  }
  /* Alloc 2 textures */
  bordertex = ta_txr_allocate(w*h*2);
  if (!bordertex) {
    STHROW_ERROR(error);
  }
  bordertex2 = ta_txr_allocate(w*h*2);
  if (!bordertex2) {
    STHROW_ERROR(error);
  }

  make_blk((uint16*)ta_txr_map(bordertex), w, h, g, w, 0);
  make_blk((uint16*)ta_txr_map(bordertex2), w, h, g, w, 1);

  SDDEBUG("bordertex=%d %d\n", bordertex, bordertex2);
	
  err = 0;
error:
  if (f) {
    fs_close(f);
  }
  /* $$$ BEN: There is no texture release !! Can't desaloc on error ! */
  sysdbg_indent(-1,0);
  SDDEBUG("<< = %d\n", err);
  return err;
}


