/** @ingroup dcplaya_vis_driver
 *  @file    fime_bordertex.c
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME : border texture
 *  $Id: fime_bordertex.c,v 1.3 2003-03-10 22:55:34 ben Exp $
 */ 

#include "dcplaya/config.h"
#include "fime_bordertex.h"
#include "border.h"
#include "sysdebug.h"

static int allocated;

static struct {
  const char * name;
  unsigned short def[3];
} default_borders[] = {
  {
    "fime_bordertile",
    { 0xFFFF, 0xFAAA, 0xFAAA }
  }
};

#define N_DEFAULT_BORDER (sizeof(default_borders)/sizeof(*default_borders))

static texid_t fime_bordertex[32];

static void borderdef_color(draw_color_t *c, unsigned short v)
{
  c->a = ((v>>12) & 15) / 15.0f;
  c->r = ((v>> 8) & 15) / 15.0f;
  c->g = ((v>> 4) & 15) / 15.0f;
  c->b = ((v>> 0) & 15) / 15.0f;
}

static void create_borderdef(border_def_t def, const unsigned short * s)
{
  borderdef_color(def+0, s[0]);
  borderdef_color(def+1, s[1]);
  borderdef_color(def+2, s[2]);
}

int fime_bordertex_init(void)
{
  texid_t texid;
  int i;

  allocated = 0;

  for (i=0; i<N_DEFAULT_BORDER; ++i) {
    SDDEBUG("fime_bordertex_init : [%s]\n", default_borders[i].name);
    texid = fime_bordertex_add(default_borders[i].name,
			       default_borders[i].def);
    if (texid < 0) {
      break;
    }
  }

  SDDEBUG("[fime : bordertex init := [%s]\n", texid>=0 ? "OK" : "FAILED" );

  return -(texid<0);
}

void fime_bordertex_shutdown(void)
{
  int i;
  for (i=0; i<32; ++i) {
    if (fime_bordertex[i] > 0) {
      texture_destroy(fime_bordertex[i], 0);
    }
    fime_bordertex[i] = -1;
  }
  allocated = 0;
  SDDEBUG("[fime : bordertex shutdowned\n");
}

int fime_bordertex_add(const char *name, const unsigned short *def16)
{
  texid_t texid;

  texid = texture_get(name);
  if (texid < 0 && allocated != -1) {
    int idx;
    for (idx = 0; allocated & (idx<<1); ++idx)
      ;
    texid = texture_dup(texture_get("bordertile"), name);
    if (texid >= 0) {
      border_def_t def;
      allocated |= (1<<idx);
      fime_bordertex[idx] = texid;
      create_borderdef(def, def16);
      border_customize(texid, def);
    }
  }
  return texid;
}

