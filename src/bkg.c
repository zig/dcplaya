/* 2002/02/12 */

#include "gp.h"

static struct {
  uint32 tex;
  float x1,x2,tw;
} layer[2] = 
  {
    { 0, 0,   512,  512 },
    { 0, 512, 640,  128}
  };

static void make_blk(uint16 *texture, int w, int h, uint8 *r, uint8 *g, uint8 *b, int ws)
{
  while (h--) {
    int x;
    
    for (x=0; x<w; ++x) {
      // RGB565			int v = (((*r++)>>3)<<11) | (((*g++)>>2)<<5) | ((*b++)>>3);
      int v = (((*r++)>>3)<<10) | (((*g++)>>3)<<5) | ((*b++)>>3);
      *texture++ = v;
    }
    r += ws-w;
    g += ws-w;
    b += ws-w;
  }
} 

int bkg_init(void)
{
  uint16 *texture;
  uint32 rh,gh,bh;
  uint8  *r, *g, *b;
	
  rh = fs_open("/rd/dream68.red", O_RDONLY);
  gh = fs_open("/rd/dream68.grn", O_RDONLY);
  bh = fs_open("/rd/dream68.blu", O_RDONLY);
	
  r = fs_mmap(rh) + fs_total(rh) - 640*480;
  g = fs_mmap(gh) + fs_total(gh) - 640*480;
  b = fs_mmap(bh) + fs_total(bh) - 640*480;
	
  layer[0].tex = ta_txr_allocate(512*512*2);
  layer[1].tex = ta_txr_allocate(128*512*2);
	
  texture = (uint16*)ta_txr_map(layer[0].tex);
  make_blk(texture, 512, 512, r, g, b, 640);
	
  texture = (uint16*)ta_txr_map(layer[1].tex);
  make_blk(texture, 128, 512, r+512, g+512, b+512, 640);
	
  if (rh) fs_close(rh);
  if (gh) fs_close(gh);
  if (bh) fs_close(bh);
	
  return 0;
}

/* Draws the floor polygon */
static void draw_background_poly(const float fade, int close)
{
  int i;
  vertex_ot_t vert;
  poly_hdr_t poly;

  vert.dummy1 = vert.dummy2 = 0;
  vert.a = fade; vert.r = fade; vert.g = fade; vert.b = fade;
  vert.oa = vert.or = vert.og = vert.ob = 0.0f;
  vert.z = 1.0f/1200.0f;
	
  for (i=0; i<2; i++) {
    const float X1 = layer[i].x1,
      Y1 = 0,
      X2 = layer[i].x2,
      Y2 = close ? 335 : 480,
      U1 = 0,
      U2 = 1,
      V1 = 0,
      V2 = Y2 * (1.0f/512.0f),
      TH = 512;

    ta_poly_hdr_txr(&poly, TA_OPAQUE, TA_ARGB1555,
		    layer[i].tw, TH, layer[i].tex, TA_NO_FILTER);
    ta_commit_poly_hdr(&poly);
  	
    vert.flags = TA_VERTEX_NORMAL;
    vert.x = X1; vert.y = Y2;
    vert.u = U1; vert.v = V2;
    ta_commit_vertex(&vert, sizeof(vert));

    /*vert.x = X1;*/ vert.y = Y1;
    /*vert.u = U1;*/ vert.v = V1;
    ta_commit_vertex(&vert, sizeof(vert));

    vert.x = X2; vert.y = Y2;
    vert.u = U2; vert.v = V2;
    ta_commit_vertex(&vert, sizeof(vert));

    vert.flags = TA_VERTEX_EOL;
    /*vert.x = X2;*/ vert.y = Y1;
    /*vert.u = U2;*/ vert.v = V1;
    ta_commit_vertex(&vert, sizeof(vert));
  }
}

void bkg_render(float fade, int info_flag)
{
  draw_background_poly(fade, info_flag);
}


