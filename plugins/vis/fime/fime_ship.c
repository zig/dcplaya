/** @ingroup dcplaya_vis_driver
 *  @file    fime_ship.c
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME : spac ship
 *  $Id: fime_ship.c,v 1.2 2003-01-20 14:23:09 ben Exp $
 */ 

#include <stdlib.h>

#include "fime_ship.h"

#include "draw/texture.h"
#include "draw/vertex.h"

#include "driver_list.h"
#include "obj_driver.h"
#include "sysdebug.h"
#include "draw_object.h"

static obj_driver_t * ship_obj;
static texid_t texid;
static matrix_t mtx;
static vtx_t angle;

int fime_ship_init(void)
{
  int err;
  ship_obj = 0;
  texid = -1;

  MtxIdentity(mtx);
  memset(&angle, 0, sizeof(angle));

  ship_obj = (obj_driver_t *)driver_list_search(&obj_drivers,"bebop");
  if (!ship_obj) {
    ship_obj = (obj_driver_t *)driver_list_search(&obj_drivers,"spaceship1");
  }
  err = !ship_obj;
  if (err) {
    SDDEBUG("[fime] : No ship object\n");
  } 
  texid = texture_get("fime_bordertile");
  if (texid < 0) {
    SDDEBUG("[fime] : No bordertex\n");
    err = 1;
  }

  return -(!!err);
}

void fime_ship_shutdown(void)
{
  driver_dereference(&ship_obj->common);
  ship_obj = 0;
}

int fime_ship_render(viewport_t *vp,
		     matrix_t camera, matrix_t proj,
		     vtx_t * ambient, 
		     vtx_t * diffuse,
		     vtx_t * light,
		     int opaque)
{
  int err = -1;
  matrix_t tmp;
  if (!ship_obj || texid < 0) {
    return -1;
  }

  MtxMult3(tmp, mtx, camera);

  ship_obj->obj.flags = 0
    | DRAW_NO_FILTER
    | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (texid << DRAW_TEXTURE_BIT);

  if (light) {
    // Draw lighted object

  } else if (ambient && diffuse) {
    err = DrawObjectFrontLighted(vp, tmp, proj,
				 &ship_obj->obj,
				 ambient, diffuse);
  } else {
    static vtx_t white = { 1,1,1,1 };
    vtx_t * color;

    color = ambient ? ambient : diffuse;
    color = color ? color : &white;
    err =  DrawObjectSingleColor(vp, tmp, proj,
				 &ship_obj->obj, color);


    MtxIdentity(tmp);
    tmp[3][2] = mtx[3][2];
    MtxMult(tmp, camera);
    err =  DrawObjectSingleColor(vp, tmp, proj,
				 &ship_obj->obj, color);
  }

  return err;
}

matrix_t * fime_ship_update(const float seconds)
{
  static float ax;
  static float ay;
  matrix_t tmp;
  float z;

  MtxIdentity(tmp);
  MtxRotateX(tmp, ax += 0.01f );
  MtxRotateY(tmp, ay += 0.031f);

  //  angle.z += 0.041f;

  z = mtx[3][2];

  MtxIdentity(mtx);
  MtxRotateZ(mtx, angle.z);
  MtxRotateX(mtx, angle.x);
  MtxRotateY(mtx, angle.y);

  mtx[3][0] = tmp[0][0] * 1.4;
  mtx[3][1] = tmp[1][0] * 1.0;
  mtx[3][2] = z;// + 0.1;
  
  return &mtx;
}

matrix_t * fime_ship_matrix(void)
{
  return &mtx;
}
