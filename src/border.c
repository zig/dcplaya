/* 2002/02/16 */

#include <stdarg.h>
#include <stdio.h>
#include "gp.h"

borderuv_t borderuv[4];
uint32 bordertex;

static void make_blk(uint16 *texture, int w, int h, uint8 *g, int ws)
{
  int y = 0;
#ifdef RECOLOR  /* Define me to have nice border tiles debug colors ! */
  uint16 recolor[2][2][3] =
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
      uint8 c = *g++;
      uint8 r,g,b;
#ifdef RECOLOR
      int xi, yi;

      xi = (x >> 5);
      yi = (y >> 5);
      r = (c * recolor[yi][xi][0]) >> 8;
      g = (c * recolor[yi][xi][1]) >> 8;
      b = (c * recolor[yi][xi][2]) >> 8;
#else
      r = g = b = c;

/*      if (c <= 0x30) c = 0x7d; */

/*      if (c <= 0x30) c = 0x7d; 
      else if (c <= 0x80) {
	c = 0x60;
	r = g = b = 180;
      }*/
      
      if (c <= 0x30) {
	c = 180; 
	r = 0.4*255;
	g = 0.4*240;
	b = 0.4*75;
      } else if (c <= 0x80) {
	c = 140;
	r = 255;
	g = 240;
	b = 75;
      }
      

#endif
      {
	int v = (((c)>>4)<<12) | (((r)>>4)<<8) | (((g)>>4)<<4) | ((b)>>4);
	*texture++ = v;
      }
    }
    g += ws-w;
  }
} 

int border_setup()
{
  const int w=64, h=64;
  uint16 *texture;
  uint32 f;
  uint8  *g;
	
  const char *fname = "/rd/bordertile.ppm";
	
  dbglog( DBG_DEBUG, ">> " __FUNCTION__ "\n");
    
  f = fs_open(fname, O_RDONLY);
  g = fs_mmap(f) + fs_total(f) - w*h;
  dbglog( DBG_DEBUG, "** " __FUNCTION__ " : file open\n");
  bordertex = ta_txr_allocate(w*h*2);
  dbglog( DBG_DEBUG, "** " __FUNCTION__ " : texture alloc [%u]\n", bordertex);
  texture = (uint16*)ta_txr_map(bordertex);
  dbglog( DBG_DEBUG, "** " __FUNCTION__ " texture mapped (%p)\n", texture);
  make_blk(texture, w, h, g, w);
  dbglog( DBG_DEBUG, "** " __FUNCTION__ " make block\n");
  if (f) fs_close(f);
  dbglog( DBG_DEBUG, "<< " __FUNCTION__ "\n");
	
  return 0;
}


