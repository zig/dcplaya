/**
 * @ingroup dcplaya_draw
 * @file    draw.c
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/11/22
 * @brief   drawing system
 *
 * $Id: draw.c,v 1.3 2002-11-29 08:29:41 ben Exp $
 */

#include "draw/draw.h"
#include "draw/gc.h"
#include "draw/ta.h"

#include "sysdebug.h"

viewport_t draw_viewport; 
matrix_t draw_projection;

float draw_screen_width;
float draw_screen_height;

int draw_init(const float screen_width, const float screen_height)
{
  int err;

  SDDEBUG("[%s] W:%.02f H:%.02f\n", __FUNCTION__, screen_width, screen_height);
  SDINDENT;

  draw_screen_width = screen_width;
  draw_screen_height = screen_height;

  /* Set default viewport */
  SDDEBUG("Set viewport.\n");
  viewport_set(&draw_viewport, 0, 0, screen_width, screen_width, 1.0f);

  /* Set default projection. */
  SDDEBUG("Set projection.\n");
  MtxProjection(draw_projection, 70*2.0*3.14159/360,
				0.01, (float)screen_width/screen_height, 1000);

  /* Init renderer (TA). */
  SDDEBUG("Init render:\n");
  SDINDENT;
  err = draw_init_render();
  SDUNINDENT;

  /* Init texture manager. */
  SDDEBUG("Init texture manager:\n");
  SDINDENT;
  err = texture_init();
  SDUNINDENT;
  if (err < 0) goto error;

  /* Init graphic context. */
  SDDEBUG("Init graphic context:\n");
  SDINDENT;
  err = gc_init();
  SDUNINDENT;
  if (err < 0) goto error;


 error:

  SDUNINDENT;
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, err);

  return err;
}

/** Shutdown the drawing system. */
void draw_shutdown(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  gc_shutdown();
  texture_shutdown();

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);
}
