/**
 * @ingroup dcplaya_draw
 * @file    draw.h
 * @author  benjamin gerard <ben@sashipa.com> 
 * @date    2002/11/22
 * @brief   drawing system
 *
 * $Id: draw.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _DRAW_H_
#define _DRAW_H_

#include "matrix.h"
#include "draw/viewport.h"

/** @defgroup dcplaya_draw dcplaya drawing system API.
 *  @ingroup  dcplaya_devel
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
 */

/** @defgroup dcplaya_draw_3d dcplaya 3D system API.
 *  @ingroup   dcplaya_draw
 *
 *  dcplaya 3D system API provides fuctions for calculating and drawing 
 *  3D primitives.
 */

/** Current drawing system screen width. */
extern float draw_screen_width;

/** Current drawing system screen height. */
extern float draw_screen_height;

/** Current drawing system viewport. */
extern viewport_t draw_viewport; 

/** Current drawing system projection. */
extern matrix_t draw_projection;

/** @name Drawing system initialization funnctions.
 *  @ingroup   dcplaya_draw
 *  @{
 */

/** Initialize the drawing system.
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

#endif /* #define _DRAW_H_ */
