/**
 * @ingroup  dcplaya_draw
 * @file     gc.h
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    graphic context interface
 *
 * $Id: gc.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _GC_H_
#define _GC_H_

#include "draw/color.h"
#include "draw/clipping.h"
#include "draw/text.h"

/** Text graphic context structure.
 * @ingroup  dcplaya_draw
 */
typedef struct {
  fontid_t fontid;     /**< font identifier.                */
  float size;          /**< text size.                      */
  float xscale;        /**< text X scale (depends on size). */
  float yscale;        /**< text Y scale (depends on size). */
  draw_argb_t argb;    /**< text ARGB packed color.         */
  draw_color_t color;  /**< text floating point color.      */
  int escape;          /**< text escape char.               */
} gc_text_t;

/** Graphic context structure.
 * @ingroup  dcplaya_draw
 */
typedef struct {

  /** Text graphic context. */
  gc_text_t text;

  /** Graphic context clipping box. */
  draw_clipbox_t clipbox;

} gc_t;

/** Pointer to current graphic context.
 * @ingroup  dcplaya_draw
 */
extern gc_t * current_gc;

/** @name graphic context functions.
 *  @ingroup  dcplaya_draw
 */

/** Initialize graphic context system. */
int gc_init(const float screen_width, const float screen_height);

/** Shutdown graphic context system. */
void gc_shutdown(void);

/** Set current graphic context. */
gc_t * gc_set(gc_t * gc);

/** Push (save) current graphic context. */
int gc_push(void);

/** Pop (restore) current graphic context. */
int gc_pop(void);

/**@}*/

#endif /* #define _GC_H_ */
