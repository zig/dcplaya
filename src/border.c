/**
 * @file    border.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/02/16
 * @brief   border rendering
 */

#include <stdarg.h>
#include <stdio.h>

#include "border.h"
#include "syserror.h"

static uint16 mode_colors[][3] = {
  /* border, fill, no-link */
  {0xFFFF, 0xF666, 0xF666}, /* */
  {0xFFFF, 0x8DDD, 0x8DDD}, 
  {0xFFFF, 0x8DDD, 0xE333},
  {0xFFFF, 0x0000, 0x0000},
  {0xFFFF, 0x0000, 0xF000},
};

#define N_BORDER (sizeof(mode_colors) / sizeof(*mode_colors))

borderuv_t borderuv[4];
texid_t bordertex[N_BORDER];
const unsigned int border_max = N_BORDER;

static void make_blk(uint16 *texture, int w, int h, int ws, int mode)
{
  int y = 0;

  mode %= border_max;
  for (y=0; y<h; ++y) {
    int x;
    
    for (x=0; x<w; ++x) {
	  int i;
      uint16 c;

	  c = *texture & 15;
	  if (c < 3) {
		i = 2;
	  }	else if (c<8) {
		i = 1;
	  } else {
		i = 0;
	  }
	  *texture++ = mode_colors[mode][i];
    }
    texture += ws-w;
  }
}

int border_setup(void)
{
  int i, err = -1;
	
  const char *fname = "/rd/bordertile.tga";
  char tname[32];

  /* Alloc 3 textures */
  bordertex[0] = texture_create_file(fname,"4444");
  for (i=1; i<N_BORDER; ++i) {
	sprintf(tname,"bordertile%d",i);
	bordertex[i] = texture_dup(bordertex[0],tname);
  }

  err = 0;
  for (i=0; i<N_BORDER; ++i) {
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
