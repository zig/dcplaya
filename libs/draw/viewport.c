/**
 * @ingroup dcplaya_draw_3d
 * @file    viewport.c
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/02/21
 * @brief   viewport definition.
 *
 * $Id: viewport.c,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#include "draw/viewport.h"

void viewport_set(viewport_t *v,
				  int posX, int posY,
				  int width, int height,
				  const float scale)
{
  v->tx = (float)(posX + (width>>1));
  v->ty = (float)(posY + (height>>1));
  v->mx = scale * 0.5f * (float)width;
  v->my = scale * 0.5f * (float)height;
}

void viewport_apply(viewport_t *v,
					float *d, int dbytes,
					const float *s, int sbytes,
					int nb)
{
  if (nb) {
    const float mx = v->mx;
    const float my = v->my;
    const float tx = v->tx;
    const float ty = v->ty;
  
    do {
      const float oow = 1.0f / s[3];
      
      d[0] = s[0] * oow * mx + tx;
      d[1] = s[1] * oow * my + ty;
      d[2] = s[2] * oow;
      
      d = (float *) ((char *)d + dbytes);
      s = (float *) ((char *)s + sbytes);
    } while (--nb);
  }
}
