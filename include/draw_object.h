/** $Id: draw_object.h,v 1.1 2002-09-02 19:11:01 ben Exp $ */

#ifndef _DRAW_OBJECT_H_
#define  _DRAW_OBJECT_H_

#include "viewport.h"
#include "obj3d.h"
#include "matrix.h"

int DrawObjectPostProcess(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o);
int DrawObjectSingleColor(viewport_t * vp, matrix_t local, matrix_t proj,
			  obj_t *o, vtx_t *color);
int DrawObjectLighted(viewport_t * vp, matrix_t local, matrix_t proj,
		      obj_t *o,
		      vtx_t *ambient, vtx_t *light, vtx_t *diffuse);
int DrawObjectPrelighted(viewport_t * vp, matrix_t local, matrix_t proj,
			 obj_t *o);

#endif
