/**
 * @file    border.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/02/16
 * @brief   border rendering
 */

#include <stdarg.h>
#include <stdio.h>
#include "syserror.h"
#include "gp.h"
#include "texture.h"

borderuv_t borderuv[4];
texid_t bordertex[3];

static void make_blk(uint16 *texture, int w, int h, int ws, int mode)
{
  int y = 0;
  for (y=0; y<h; ++y) {
    int x;
    
    for (x=0; x<w; ++x) {
      uint8 c, r, g, b;

	  c = *texture;
	  c = c>>12;
	  c |= c<<4;
      r = g = b = c;

      if (mode == 2) {
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
      } else {
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
      }
	  
      {
		int v = (((c)>>4)<<12) | (((r)>>4)<<8) | (((g)>>4)<<4) | ((b)>>4);
		*texture++ = v;
      }
    }
    texture += ws-w;
  }
}

int border_setup(void)
{
  int i, err = -1;
	
  const char *fname = "/rd/bordertile.tga";

  /* Alloc 3 textures */
  bordertex[0] = texture_create_file(fname,"4444");
  bordertex[1] = texture_dup(bordertex[0], "bordertile2");
  bordertex[2] = texture_dup(bordertex[0], "bordertile3");

  err = 0;
  for (i=0; i<3; ++i) {
	texture_t * t;

	SDDEBUG("[%s] : border[%d] := %d\n", __FUNCTION__, i, bordertex[i]);

	t = texture_lock(bordertex[i]);
	if (!t) {
	  SDERROR("[%s] : texture [#%d], lock failed\n",
			  __FUNCTION__, bordertex[i]);
	  err = -1;
	  continue;
	}
	make_blk(t->addr, t->width, t->height, 1 << t->wlog2, i);

	SDDEBUG("-> %s\n",t->name);

	texture_release(t);
  }

  return err;
}
