/**
 * @file     draw_object.h
 * @author   benjamin gerard <ben@sashipa.com>
 * @author   vincent penne
 * @brief    Simple 3D api
 *
 * @version  $Id: draw_object.h,v 1.6 2003-01-22 19:12:56 ben Exp $
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
			  obj_t *o, const vtx_t *color);

int DrawObjectLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
		      vtx_t *ambient, vtx_t *light, vtx_t *diffuse);

int DrawObjectFrontLighted(viewport_t * vp, matrix_t local, matrix_t proj,
			   obj_t *o,
			   const vtx_t *ambient, const vtx_t *diffuse);

int DrawObjectPrelighted(viewport_t * vp, matrix_t local, matrix_t proj,
			 obj_t *o);

int DrawObject(viewport_t * vp, matrix_t local, matrix_t proj,
	       obj_t *o,
	       const vtx_t * ambient, 
	       const vtx_t * diffuse,
	       const vtx_t * light);


DCPLAYA_EXTERN_C_END

#endif /* #ifndef _DRAW_OBJECT_H_ */
