/** @ingroup dcplaya_vis_driver
 *  @file    fime_bees.h
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME : bees 
 *  $Id: fime_bees.h,v 1.2 2003-01-21 02:38:16 ben Exp $
 */ 

#ifndef _FIME_BEES_H_
#define _FIME_BEES_H_

#include "vtx.h"
#include "matrix.h"
#include "draw/viewport.h"

typedef struct fime_bee_s fime_bee_t;

struct fime_bee_s {
  fime_bee_t * leader;
  fime_bee_t * buddy;
  fime_bee_t * soldier;

  vtx_t rel_pos; /* Position relative to */

  vtx_t pos;      /**< Current position */
  vtx_t prev_pos; /**< Previous position */

  float spd;

  matrix_t mtx;
};


int fime_bees_init(void);
void fime_bees_shutdown(void);

int fime_bees_update(void);

int fime_bees_render(viewport_t *vp,
		     matrix_t camera, matrix_t proj);

#endif /* #define _FIME_BEES_H_ */

