/**
 * @date 2003/02/03
 */

#ifndef _FIME_GROUND_H_
#define _FIME_GROUND_H_

#include "matrix.h"
#include "draw/viewport.h"


int fime_ground_init(float w, float h, int nw, int nh);
void fime_ground_shutdown(void);
int fime_ground_update(const float seconds);
int fime_ground_render(viewport_t *vp, matrix_t camera, matrix_t proj,
		       int opaque);

#endif /* #define _FIME_GROUND_H_ */
