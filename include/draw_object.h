/**
 * @file     draw_object.h
 * @author   benjamin gerard <ben@sashipa.com>
 * @author   vincent penne
 * @brief    Simple 3D api
 *
 * @version  $Id: draw_object.h,v 1.5 2002-11-25 16:51:05 ben Exp $
 */

#ifndef _DRAW_OBJECT_H_
#define _DRAW_OBJECT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#include "draw/viewport.h"
#include "obj3d.h"
#include "matrix.h"

/** 3D pipeline post-process.
 *
 *     The DrawObjectPostProcess() functions includes vertrices transformation
 *     and back face calculation.
 *
 * @param  vp     Rendering viewport.
 * @param  local  Object transfomation matrix (e.g rotation, translation).
 * @param  proj   Projection matrix as setted by MtxProjection().
 * @param  o      3D object.
 *
 * @return Error code
 * @retval  0 : success
 * @retval <0 : failure
 */
int DrawObjectPostProcess(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o);

int DrawObjectSingleColor(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o, vtx_t *color);

int DrawObjectLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
		      vtx_t *ambient, vtx_t *light, vtx_t *diffuse);

int DrawObjectFrontLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
			   vtx_t *ambient, vtx_t *diffuse);

int DrawObjectPrelighted(viewport_t * vp, matrix_t local, matrix_t proj,
			 obj_t *o);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _DRAW_OBJECT_H_ */
