/**
 * @ingroup  dcplaya_draw_text
 * @file     gc.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    graphic context interface.
 *
 * $Id: gc.c,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#include "draw/gc.h"
#include "sysdebug.h"

#define MAX_GC 16

static gc_t gc_stack[MAX_GC];
gc_t * current_gc;

int gc_init(const float width, const float height)
{
  int err;

  SDDEBUG("[%s] : W:%.02f H:%.02f\n", __FUNCTION__, width, height);
  SDINDENT;

  current_gc = gc_stack;

  /* Init clipping box. */
  current_gc->clipbox.x1 = 0;
  current_gc->clipbox.y1 = 0;
  current_gc->clipbox.x2 = width;
  current_gc->clipbox.y2 = height;

  /* Init text drawing system : load font ... */
  err = text_init();
  
  SDUNINDENT;
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, err);

  return err;
}

void gc_shutdown(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  text_shutdown();

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);
}

gc_t * gc_set(gc_t * gc)
{
  gc_t * old = current_gc;

  if (current_gc != gc && current_gc) {
	*current_gc = *gc;
  }
  return old;
}

int gc_push(void)
{
  if (current_gc >= &gc_stack[MAX_GC]) {
	return -1;
  }

  current_gc[1] = current_gc[0];
  ++current_gc;
  return 0;
}

int gc_pop(void)
{
  if (current_gc <= gc_stack) {
	return -1;
  }
  --current_gc;
  current_gc[0] = current_gc[1];
  return 0;
}
