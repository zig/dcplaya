/* Tryptonite

   pvrutils.c
   (c)2000 Dan Potter

   PVR Utility Functions
*/

static char id[] = "KOS $Id: pvrutils.c,v 1.1 2004-07-04 14:16:45 vincentp Exp $";

#include <kos.h>
#include "dc/ta.h"
//#include "pvrutils.h"

#define MIN(a, b) ( (a)<(b)? (a):(b) )

/* Linear/iterative twiddling algorithm from Marcus' tatest */
#define TWIDTAB(x) ( (x&1)|((x&2)<<1)|((x&4)<<2)|((x&8)<<3)|((x&16)<<4)| \
	((x&32)<<5)|((x&64)<<6)|((x&128)<<7)|((x&256)<<8)|((x&512)<<9) )
#define TWIDOUT(x, y) ( TWIDTAB((y)) | (TWIDTAB((x)) << 1) )

/* Twiddle function -- copies from a source rectangle in SH-4 ram to a
   destination texture in PVR ram. Areas outside the source texture will
   be filled with bgcol. */
void txr_twiddle_copy(const uint16 *src, uint32 srcw, uint32 srch,
		uint32 dest, uint32 destw, uint32 desth, uint16 bgcol) {
	int	x, y;
	uint16	*vtex;
	uint16	val;
	
	vtex = (uint16*)ta_txr_map(dest);
	
	for (y=0; y<desth; y++) {
		for (x=0; x<destw; x++) {
			if (x >= srcw || y >= srch)
				val = bgcol;
			else
				val = src[y*srcw+x];
			vtex[TWIDOUT(x,y)] = val;
		}
	}
}

/* General twiddle function -- copies a rectangle from SH-4 ram to PVR ram. 
   Texture can be 16bpp, 8bpp or 4bpp (i.e. paletted). Contrary to other
   twiddle copy functions,  the rectangle does not
   need to be a square (not tested  with h>w, but this is just
   a matter of a rotation by PI/2 and UVs swapping).

   w and h must be a power of 2.
   bpp must be either 4, 8 or 16.

   Returns 0 if success, -1 otherwise.

   Contributed by Vincent Penne.

   NOTE: May eventually be integrated with the normal twiddle_copy.
 */
int txr_twiddle_copy_general(const void *src, uint32 dest, 
			      uint32 w, uint32 h, uint32 bpp) {
	int x, y, min, mask;

	min = MIN(w, h);
	mask = min - 1;

	switch (bpp) {
	case 4: {
		uint8 * pixels;
		uint16 * vtex;
		pixels = (uint8 *) src;
		vtex = (uint16*)ta_txr_map(dest);
		for (y=0; y<h; y += 2) {
			for (x=0; x<w; x += 2) {
				vtex[TWIDOUT((x&mask)/2, (y&mask)/2) + 
					(x/min + y/min)*min*min/4] = 
					(pixels[(x+y*w) >>1]&15) | ((pixels[(x+(y+1)*w) >>1]&15)<<4) | 
					((pixels[(x+y*w) >>1]>>4)<<8) | ((pixels[(x+(y+1)*w) >>1]>>4)<<12);
			}
		}
		return 0;
	}
	case 8: {
		uint8 * pixels;
		uint16 * vtex;
		pixels = (uint8 *) src;
		vtex = (uint16*)ta_txr_map(dest);
		for (y=0; y<h; y += 2) {
			for (x=0; x<w; x++) {
				vtex[TWIDOUT((y&mask)/2, x&mask) + 
					(x/min + y/min)*min*min/2] = 
					pixels[y*w+x] | (pixels[(y+1)*w+x]<<8);
			}
		}
		return 0;
	}
	case 16: {
		uint16 * pixels;
		uint16 * vtex;
		pixels = (uint16 *) src;
		vtex = (uint16*)ta_txr_map(dest);
		for (y=0; y<h; y++) {
			for (x=0; x<w; x++) {
				vtex[TWIDOUT(x&mask,y&mask) + 
					(x/min + y/min)*min*min] = pixels[y*w+x];
			}
		}
		return 0;
	}
	default:
		return -1;
	}
}



/* Twiddle function -- copies from a source rectangle in SH-4 ram to a
   destination texture in PVR ram. The image will be scaled to the texture
   size. */
void txr_twiddle_scale(const uint16 *src, uint32 srcw, uint32 srch,
		uint32 dest, uint32 destw, uint32 desth) {
	int	x, y, srcx, srcy;
	uint16	*vtex;
	uint16	val;
	float	scalex, scaley;
	
	vtex = (uint16*)ta_txr_map(dest);
	scalex = srcw * 1.0f / destw;
	scaley = srch * 1.0f / desth;
	
	for (y=0; y<desth; y++) {
		for (x=0; x<destw; x++) {
			srcx = (int)(x*scalex);
			srcy = (int)(y*scaley);
			val = src[srcy*srcw+srcx];
			vtex[TWIDOUT(x,y)] = val;
		}
	}
}

/* Adjusts a 16-bit image so that instead of RGB565 gray scales, you will
   have ARGB4444 alpha scales. The resulting image will be entirely white. */
void txr_to_alpha(uint16 *img, int x, int y) {
	int i;
	short v;
	
	for (i=0; i<x*y; i++) {
		v = img[i] & 0x1f;
		v = ((v >> 1) << 12) | 0x0fff;
		img[i] = v;
	}
}

/* Commits a dummy polygon (for unused lists). Specify the polygon
   type (opaque/translucent). */
void pvr_dummy_poly(int type) {
	poly_hdr_t poly;
	ta_poly_hdr_col(&poly, type);
	ta_commit_poly_hdr(&poly);
}

/* Commits an entire blank frame. Assumes two lists active (opaque/translucent) */
void pvr_blank_frame() {
	ta_begin_render();
	pvr_dummy_poly(TA_OPAQUE);
	ta_commit_eol();
	pvr_dummy_poly(TA_TRANSLUCENT);
	ta_commit_eol();
	ta_finish_frame();
}


