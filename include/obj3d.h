/**
 * @file      obj3d.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      2001/10/20
 * @brief     Simple 3D objects defintions for 3D-object plugins
 * @version   $Id: obj3d.h,v 1.6 2003-01-21 02:38:16 ben Exp $
 */

/* Copyright (C) 1998-2001 Ben(jamin) Gerard */

#ifndef _OBJ3D_H_
#define _OBJ3D_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "vtx.h"
/* #include "any_driver.h" */


/** Triangle */
typedef struct {
  int a;      /**< Index of 1st vertex */
  int b;      /**< Index of 2nd vertex */
  int c;      /**< Index of 3td vertex */
  int flags;  /**< flags */
} tri_t;

typedef struct {
  int a;      /**< Index of [a b] linked face */
  int b;      /**< Index of [b c] linked face */
  int c;      /**< Index of [c a] linked face */
  int flags;
} tlk_t;

/** 3D object */
typedef struct {
  const char *name; /**< Object name */

  int flags;        /**< Flags @warning Used for texture-id  */
  int	nbv;        /**< Number of vertex */
  int	nbf;        /**< Number of faces */

  int static_nbv;   /**< ? */
  int static_nbf;   /**< ? */

  vtx_t *vtx;       /**< Pointer to vertex buffer */
  tri_t *tri;       /**< Pointer to face buffer */
  tlk_t *tlk;       /**< Pointer to linked face buffer */
  vtx_t *nvx;       /**< Pointer to face normal */
} obj_t;

/** @name Simple 3d Object
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Build and alloc if neccessary face normals. */
int obj3d_build_normals(obj_t *o);

/** @} */

DCPLAYA_EXTERN_C_END



#endif
