/**
 * @ingroup  dcplaya_draw_text
 * @file     gc.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    graphic context interface.
 *
 * $Id: gc.c,v 1.4 2002-11-29 08:29:41 ben Exp $
 */

#include "draw/gc.h"
#include "draw/draw.h"
#include "sysdebug.h"

#define MAX_GC 16

static gc_t gc_stack[MAX_GC];
static gc_t * sp_gc;

gc_t default_gc;
gc_t * current_gc;

void gc_default(void)
{
  int i;
  gc_t * gc = current_gc;

  /* Default clipping box. */
  gc->clipbox.x1 = 0;
  gc->clipbox.y1 = 0;
  gc->clipbox.x2 = draw_screen_width;
  gc->clipbox.y2 = draw_screen_height;

  /* Default colors. */
  for (i=0; i< sizeof(gc->colors)/ sizeof(*gc->colors); ++i) {
	gc->colors[i].a = gc->colors[i].r = gc->colors[i].g = gc->colors[i].b = 1;
  }

  /* Default text properties. */
  text_set_color(1,1,1,1);
  text_set_properties(0,16,1);
  text_set_escape('%');
}

void gc_reset(void)
{
  sp_gc = gc_stack;
  gc_default();
}

int gc_init(void)
{
  int err;

  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  current_gc = &default_gc;
  sp_gc = gc_stack;

  /* Init text drawing system : load font ... */
  err = text_init();
  if (!err) {
	gc_reset();
  }
  
  SDUNINDENT;
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, err);

  return err;
}

void gc_shutdown(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  SDINDENT;

  current_gc = 0;
  sp_gc = 0;
  text_shutdown();

  SDUNINDENT;
  SDDEBUG("[%s] := [0]\n", __FUNCTION__);
}

gc_t * gc_set(gc_t * gc)
{
  gc_t * old = current_gc;
  current_gc = gc;
  text_set_font(current_gc->text.fontid);
  return old;
}

int gc_push(int flags)
{
  if (sp_gc >= &gc_stack[MAX_GC]) {
	return -1;
  }

  /* Restore font face. */
  if (flags & GC_RESTORE_TEXT_FONT) {
	sp_gc->text.fontid = current_gc->text.fontid;
  }

  /* Restore size parameters. */
  if (flags & GC_RESTORE_TEXT_SIZE) {
	sp_gc->text.size = current_gc->text.size;
	sp_gc->text.aspect = current_gc->text.aspect;
  }

  /* Restore text color. */
  if (flags & GC_RESTORE_TEXT_COLOR) {
	sp_gc->text.argb = current_gc->text.argb;
  }

  /* Restore background colors. */
  if (flags & GC_RESTORE_COLORS_0) {
	sp_gc->colors[0] = current_gc->colors[0];
  }
  if (flags & GC_RESTORE_COLORS_1) {
	sp_gc->colors[1] = current_gc->colors[1];
  }
  if (flags & GC_RESTORE_COLORS_2) {
	sp_gc->colors[2] = current_gc->colors[2];
  }
  if (flags & GC_RESTORE_COLORS_3) {
	sp_gc->colors[3] = current_gc->colors[3];
  }

  /* Restore clipping box. */
  if (flags & GC_RESTORE_CLIPPING) {
	sp_gc->clipbox = current_gc->clipbox;
  }

  ++sp_gc;
  return 0;
}

int gc_pop(int flags)
{
  if (sp_gc <= gc_stack) {
	return -1;
  }

  /* Restore font face. */
  if (flags & GC_RESTORE_TEXT_FONT) {
	current_gc->text.fontid = sp_gc->text.fontid;
  }

  /* Restore size parameters. */
  if (flags & GC_RESTORE_TEXT_SIZE) {
	current_gc->text.size = sp_gc->text.size;
	current_gc->text.aspect = sp_gc->text.aspect;
  }

  /* Restore text color. */
  if (flags & GC_RESTORE_TEXT_COLOR) {
	current_gc->text.argb = sp_gc->text.argb;
  }

  /* Restore background colors. */
  if (flags & GC_RESTORE_COLORS_0) {
	current_gc->colors[0] = sp_gc->colors[0];
  }
  if (flags & GC_RESTORE_COLORS_1) {
	current_gc->colors[1] = sp_gc->colors[1];
  }
  if (flags & GC_RESTORE_COLORS_2) {
	current_gc->colors[2] = sp_gc->colors[2];
  }
  if (flags & GC_RESTORE_COLORS_3) {
	current_gc->colors[3] = sp_gc->colors[3];
  }

  /* Restore clipping box. */
  if (flags & GC_RESTORE_CLIPPING) {
	current_gc->clipbox = sp_gc->clipbox;
  }

  --sp_gc;
  return 0;
}

void draw_set_color(int idx, const draw_color_t * color)
{
  const unsigned int max =
	sizeof(current_gc->colors) / sizeof(current_gc->colors[0]);

  if ((unsigned int)idx < max) {
	current_gc->colors[idx] = *color;
  }
}

void draw_set_argb(int idx, const draw_argb_t argb)
{
  draw_color_t color;
  draw_color_argb_to_float(&color, argb);
  draw_set_color(idx, &color);
}
