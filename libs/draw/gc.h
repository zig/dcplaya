/**
 * @ingroup  dcplaya_draw
 * @file     gc.h
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    Graphic context interface
 *
 * $Id: gc.h,v 1.2 2002-11-27 09:58:09 ben Exp $
 */

#ifndef _GC_H_
#define _GC_H_

#include "draw/color.h"
#include "draw/clipping.h"
#include "draw/text.h"

/** @name Graphic context restore flags.
 *  @ingroup dcplaya_draw
 *  @{
 */
#define GC_RESTORE_ALL         -1 /**< Restore all graphic context. */

#define GC_RESTORE_TEXT_FONT   (1<<0) /**< Restore text font.       */
#define GC_RESTORE_TEXT_SIZE   (1<<1) /**< Restore text size.       */
#define GC_RESTORE_TEXT_COLOR  (1<<2) /**< Restore text color.      */
#define GC_RESTORE_TEXT        (7<<0) /**< Restore text properties. */

#define GC_RESTORE_COLORS_0    (1<<3)  /**< Restore left/top color.     */
#define GC_RESTORE_COLORS_1    (1<<4)  /**< Restore right/top color.    */
#define GC_RESTORE_COLORS_2    (1<<5)  /**< Restore left/bottom color.  */
#define GC_RESTORE_COLORS_3    (1<<6)  /**< Restore right/bottom color. */
#define GC_RESTORE_COLORS      (16<<3) /**< Restore all colors.         */

#define GC_RESTORE_CLIPPING_X1 (1<<7)  /**< Restore left clipping.    */
#define GC_RESTORE_CLIPPING_Y1 (1<<8)  /**< Restore top clipping.     */
#define GC_RESTORE_CLIPPING_X2 (1<<9)  /**< Restore right clipping.   */
#define GC_RESTORE_CLIPPING_Y2 (1<<10)  /**< Restore bottom clipping. */
#define GC_RESTORE_CLIPPING    (16<<7)  /**< Restore clipping box.    */

/**@}*/

/** Text graphic context structure.
 *  @ingroup dcplaya_draw
 */
typedef struct {
  fontid_t fontid;     /**< font identifier.                */
  float size;          /**< text size.                      */
  float aspect;        /**< text aspect ratio (Y/X).        */
  //  float xscale;        /**< text X scale (depends on size). */
  //  float yscale;        /**< text Y scale (depends on size). */
  draw_argb_t argb;    /**< text ARGB packed color.         */
  draw_color_t color;  /**< text floating point color.      */
  int escape;          /**< text escape char.               */
} gc_text_t;

/** Graphic context structure.
 *  @ingroup dcplaya_draw
 */
typedef struct {

  /** Text graphic context. */
  gc_text_t text;

  /** Graphic context clipping box. */
  draw_clipbox_t clipbox;

  /** Graphic context back ground colors [LT/RT/LB/RB] */
  draw_color_t colors[4];

} gc_t;

/** Pointer to current graphic context.
 * @ingroup  dcplaya_draw
 */
extern gc_t * current_gc;

/** Default graphic context.
 *  @ingroup dcplaya_draw
 */
extern gc_t default_gc;

/** @name Graphic context functions.
 *  @ingroup dcplaya_draw
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
int gc_pop(int restore_flags);

/**@}*/

/** @name Color primitives.
 *  @ingroup dcplay_draw
 */

/** Set a color in the color lookup table. */
void draw_set_color(int idx, const draw_color_t * color);

/** Set a color in the color lookup table. */
void draw_set_argb(int idx, const draw_argb_t argb);

/**@}*/



#endif /* #define _GC_H_ */
