/**
 * @ingroup  dcplaya_draw_gc
 * @file     draw/gc.h
 * @author   benjamin gerard
 * @brief    Graphic context interface
 *
 * $Id: gc.h,v 1.7 2003-03-23 23:54:54 ben Exp $
 */

#ifndef _GC_H_
#define _GC_H_

/** @defgroup  dcplaya_draw_gc Graphic Context
 *  @ingroup   dcplaya_draw
 *  @brief     graphic context
 *  @author    benjamin gerard
 *  @{
 */

/* $$$ colors currently not used */
#define GC_NO_COLOR 1

#include "draw/color.h"
#include "draw/clipping.h"
#include "draw/text.h"

/** @name Graphic context restore flags.
 *  @{
 */
#define GC_RESTORE_ALL         -1 /**< Restore all graphic context. */

#define GC_RESTORE_TEXT_FONT   (1<<0) /**< Restore text font.       */
#define GC_RESTORE_TEXT_SIZE   (1<<1) /**< Restore text size.       */
#define GC_RESTORE_TEXT_COLOR  (1<<2) /**< Restore text color.      */
#define GC_RESTORE_TEXT_FILTER (1<<3) /**< Restore text filter.     */
#define GC_RESTORE_TEXT        (15<<0) /**< Restore text properties. */

#define GC_RESTORE_COLORS_0    (1<<4)  /**< Restore left/top color.     */
#define GC_RESTORE_COLORS_1    (1<<5)  /**< Restore right/top color.    */
#define GC_RESTORE_COLORS_2    (1<<6)  /**< Restore left/bottom color.  */
#define GC_RESTORE_COLORS_3    (1<<7)  /**< Restore right/bottom color. */
#define GC_RESTORE_COLORS      (15<<4) /**< Restore all colors.         */

#define GC_RESTORE_CLIPPING    (1<<8)  /**< Restore clipping box.    */

/**@}*/

/** Text graphic context structure.
 */
typedef struct {
  fontid_t fontid;     /**< font identifier.                */
  float size;          /**< text size.                      */
  float aspect;        /**< text aspect ratio (Y/X).        */
  draw_argb_t argb;    /**< text ARGB packed color.         */
  draw_color_t color;  /**< text floating point color.      */
  int escape;          /**< text escape char.               */
  int filter;          /**< text filter mode.               */
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
#ifndef GC_NO_COLOR
  draw_color_t colors[4];
#endif

} gc_t;

/** Pointer to current graphic context.
 */
extern gc_t * current_gc;

/** Default graphic context.
 */
extern gc_t default_gc;

/** Initialize graphic context system. */
int gc_init(void);

/** Shutdown graphic context system. */
void gc_shutdown(void);

/** Set current graphic context. */
gc_t * gc_set(gc_t * gc);

/** Set current graphic context with default value. */
void gc_default(void);

/** Set current graphic context with default value and clear gc stack. */
void gc_reset(void);

/** Push (save) current graphic context. */
int gc_push(int save_flags);

/** Pop (restore) current graphic context. */
int gc_pop(int restore_flags);

/** @name Color primitives.
 *  @{
 *  @deprecated  The GC colors are currently not used.
 */

/** Set a color in the color lookup table.
 */
void draw_set_color(int idx, const draw_color_t * color);

/** Set a color in the color lookup table.
 */
void draw_set_argb(int idx, const draw_argb_t argb);

/**@}*/

/**@}*/

#endif /* #define _GC_H_ */
