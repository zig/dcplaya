/**
 * @ingroup  dcplaya_draw_ta
 * @file     draw/ta.h
 * @author   benjamin gerard
 * @date     2002/11/22
 * @brief    draw tile accelerator interface.
 *
 * $Id: ta.h,v 1.5 2004-07-31 22:55:18 vincentp Exp $
 */

#ifndef _DRAW_TA_H_
#define _DRAW_TA_H_

#include <arch/types.h>
#include "dc/ta.h"

#include "draw/ta_defines.h"

/** @defgroup  dcplaya_draw_ta Tile Accelerator (TA)
 *  @ingroup   dcplaya_draw
 *  @brief     tile accelerator (TA) interface
 *  @author    benjamin gerard
 *  @{
 */


/** @name TA internal command strutures.
 *  @{
 */
typedef struct {
  volatile uint32 word1;
  volatile uint32 word2;
  volatile uint32 word3;
  volatile uint32 word4;
  volatile uint32 words[4];
} ta_hw_poly_t;

typedef struct {
  volatile uint32 flags;
  volatile float x;
  volatile float y;
  volatile float z;
  volatile float a;
  volatile float r;
  volatile float g;
  volatile float b;
} ta_hw_col_vtx_t;

typedef struct {
  volatile uint32 flags;
  volatile float x;
  volatile float y;
  volatile float z;
  volatile float u;
  volatile float v;
  volatile uint32 col;
  volatile uint32 addcol;
} ta_hw_tex_vtx_t;

/** @} */

/** @name Tile Accelarator hardware registers.
 *  @{
 */

/** Tile accelarator hardware color (no texture) vertex. */
#define HW_COL_VTX ((ta_hw_col_vtx_t *)(0xe0<<24))

/** Tile accelarator hardware textured vertex. */
#define HW_TEX_VTX ((ta_hw_tex_vtx_t *)(0xe0<<24))

/** Tile accelarator hardware polygon. */
#define HW_POLY    ((ta_hw_poly_t    *)(0xe0<<24))

/** @} */

/** @name Render session function.
 *
 *    To process redering the following @b must be do in this order in
 *    the main loop after the rendering as been initialized properly with
 *    draw_init_render() function.
 *      - Opening render in opaque mode with the draw_open_render() function.
 *      - Process all opaque primitives.
 *      - Opening render in translucent with the draw_translucent_render()
 *        function.
 *      - Process all translucent primitives.
 *      - Close rendering with the draw_close_render() function.
 *
 *  @warming Anyway all these functions should do all proper things if it is
 *   not done, but this has not been really tested. The really important thing
 *   to understand is that you can't mix opaque and translucent rendering
 *   and that you need to process all opaque primitives before any
 *   translucent.
 *
 *  @{
 */

/** Initialize rendering system. */
int draw_init_render(void);

/** Open render.
 *
 *     Calling the draw_open_render() function begins a new render frame.
 *     After this call the TA is in opaque rendering mode.
 *
 *  @return number of frame elapsed since previous render.
 */
unsigned int draw_open_render(void);

/** Open translucent rendering mode.
 *
 *     Calling the draw_translucent_render() function begins the translucent
 *     rendering mode.
 */
void draw_translucent_render(void);

/** Close the render session. */
void draw_close_render(void);

/** @} */

/** Set current drawing mode. */
void draw_set_flags(int flags);

/** Current drawing flags (mode) (read-only).
 *
 * @warning Do not write this value ! Use draw_set_flags() or the fast
 * DRAW_SET_FLAGS instead.
 */

/* $$$ ben hacks : see ta.c */
extern
#ifndef _IN_TA_C_
const
#endif
int draw_current_flags;

/** Current frame counter. */
extern unsigned int draw_frame_counter;

/** Macro to speed up draw_set_flags() access when the value does not change
 *  from the current. Since the draw_current_flags to DRAW_INVALID_FLAGS
 *  by the draw_open_render() this macro should be safe enougth cross
 *  frames.
 */
#define DRAW_SET_FLAGS(F) if (draw_current_flags != (F)) { draw_set_flags(F); } else

/** You must call this before initiating any dma */
void draw_lock();

/** You must call this before initiating any dma */
int draw_trylock();

/** You must call this when you're done with dma */
void draw_unlock();

/**@}*/

#endif /* #define _DRAW_TA_H_ */
