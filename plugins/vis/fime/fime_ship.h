/** @ingroup dcplaya_fime_vis_plugin_devel
 *  @file    fime_ship.h
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME space ship
 *
 *  $Id: fime_ship.h,v 1.2 2003-03-19 05:16:16 ben Exp $
 */ 

#ifndef _FIME_SHIP_H_
#define _FIME_SHIP_H_

/** @defgroup  dcplaya_fime_vis_plugin_devel  FIME visual plugin
 *  @ingroup   dcplaya_vis_plugin_devel
 *  @brief     FIME, Fly Into a Musical Environment.
 *
 *    This plugin is in development. It is far from what it intends to do.
 *
 *  @author    benjamin gerard
 */

#include "draw/viewport.h"
#include "matrix.h"
#include "obj3d.h"

int fime_ship_init();
void fime_ship_shutdown();

int fime_ship_render(viewport_t *vp,
		     matrix_t camera, matrix_t proj,
		     vtx_t * ambient, 
		     vtx_t * diffuse,
		     vtx_t * light,
		     int opaque);


matrix_t * fime_ship_update(const float seconds);
matrix_t * fime_ship_matrix(void);

#endif /* #define _FIME_SHIP_H_ */
