/** @ingroup dcplaya_fime_vis_plugin_devel
 *  @file    fime_star.h
 *  @author  benjamin gerard 
 *  @date    2003/02/04
 *  @brief   FIME star field 
 *
 *  $Id: fime_star.h,v 1.2 2003-03-19 05:16:16 ben Exp $
 */ 

#ifndef _FIME_STAR_H_
#define _FIME_STAR_H_

#include <stdlib.h>

#include "vtx.h"
#include "matrix.h"
#include "obj_driver.h"
#include "draw/viewport.h"

/** fime star transform and anim.
 *  @ingroup dcplaya_fime_vis_plugin_devel
 */
typedef struct {
  matrix_t mtx;     /**< Transform matrix (without translation). */
  vtx_t scale;      /**< Scaling X,Y,Z.                          */
  vtx_t cur_scale;  /**< Current scaling.                        */
  vtx_t goal_scale; /**< Goal scaling.                           */
  vtx_t angle;      /**< Current rotation angles (X,Y,Z).        */
  vtx_t inc_angle;  /**< Rotation increment per second.          */
  float z_mov;
  float cur_z_mov;
} fime_star_trans_t;

/** fime star.
 *  @ingroup dcplaya_fime_vis_plugin_devel
 */
typedef struct {
  fime_star_trans_t * trans; /**< Reference transformation. */
  obj_t *obj;                /**< Reference object.         */
  vtx_t pos;                 /**< Star position.            */ 
} fime_star_t;

/** fime star field bounding box.
 *  @ingroup dcplaya_fime_vis_plugin_devel
 */
typedef struct {
  float xmin;
  float xmax;
  float ymin;
  float ymax;
  float zmin;
  float zmax;
} fime_star_box_t;

typedef struct fime_star_field_s fime_star_field_t;

/** star generator function.
 *  @ingroup dcplaya_fime_vis_plugin_devel
 */
typedef void (*fime_star_generator_t)(fime_star_field_t *, fime_star_t *);

/** star field.
 *  @ingroup dcplaya_fime_vis_plugin_devel
 */
struct fime_star_field_s {
  int nb_forms;
  obj_driver_t ** forms;

  int nb_trans;                    /**< Max transformations  */
  fime_star_trans_t * trans;       /**< Star transformations */

  int nb_stars;                    /**< Max stars            */
  fime_star_t * stars;             /**< Star array           */

  fime_star_box_t box;             /**< Star box             */
  fime_star_generator_t generator; /**< Star generator       */
  void * cookie;

} ;


fime_star_field_t * fime_star_init(int nb_stars,
				   const char ** forms,
				   const fime_star_box_t * box,
				   void * cookie);

void fime_star_shutdown(fime_star_field_t * sf);
int fime_star_update(fime_star_field_t * sf, const float seconds);

int fime_star_render(fime_star_field_t * sf,
		     viewport_t *vp,
		     matrix_t camera, matrix_t proj,
		     vtx_t * ambient,
		     vtx_t * diffuse,
		     vtx_t * light,
		     int opaque);

#endif /* #define _FIME_STAR_H_ */
