/**
 * @ingroup dcplaya_draw
 * @file    draw/draw.h
 * @author  benjamin gerard
 * @date    2002/11/22
 * @brief   drawing system
 *
 * $Id: draw.h,v 1.5 2003-03-23 23:54:54 ben Exp $
 */

#ifndef _DRAW_H_
#define _DRAW_H_

#include "matrix.h"
#include "draw/viewport.h"

/** @defgroup  dcplaya_draw  Drawing system.
 *  @ingroup   dcplaya_devel
 *  @brief     Drawing system.
 *
 *  dcplaya drawing system API provides functions for drawing text and
 *  geometrical primitives towards tile accelerator.
 *
 *  dcplaya drawing system provides functions for :
 *  - 2D text primitives
 *  - 2D clipping
 *  - Boxes drawing
 *  - Texture manager
 *  - Triangle primitive
 *  - Strip primitive
 *
 *  @author    benjamin gerard
 *  @{
 */

/** @name  Drawing system globals.
 *  @{
 */

/** Current drawing system screen width. */
extern float draw_screen_width;

/** Current drawing system screen height. */
extern float draw_screen_height;

/** Current drawing system viewport. */
extern viewport_t draw_viewport; 

/** Current drawing system projection. */
extern matrix_t draw_projection;

/** Current draw projection Z near. */
extern float draw_znear;

/** Current draw projection 1/Z near. */
extern float draw_ooznear;

/**@}*/

/** @name Drawing system initialization functions.
 *  @{
 */

/** Initialize the drawing system.
 *
 *    The draw_init() function initializes all drawing system components :
 *      - Set a default viewport on the screen box.
 *      - Set a default projection matrix with an open angle of 70 degree
 *        a zFar at 250 and retrieves zNear.
 *      - Initialize the rendering engine with the draw_init_render() function.
 *      - Initialize the texture manager with the texture_init() function.
 *      - Initialize the graphic context with the gc_init() function
 *
 *  @param  screen_width   Screen width in pixel.
 *  @param  screen_height  Screen height in pixel.
 *
 * @return error-code
 * @retval 0  Success
 * @retval <0 Failure
 */
int draw_init(const float screen_width, const float screen_height);

/** Shutdown the drawing system. */
void draw_shutdown(void);

/**@}*/

/**@}*/

#endif /* #define _DRAW_H_ */
