/** @ingroup dcplaya_vis_driver
 *  @file    fime_ship.h
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME : spac ship
 *  $Id: fime_ship.h,v 1.1 2003-01-18 14:22:17 ben Exp $
 */ 

#ifndef _FIME_SHIP_H_
#define _FIME_SHIP_H_

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
