/**
 * @ingroup  dcplaya_display_exe_plugin_devel
 * @file     display_matrix.h
 * @author   Vincent Penne <ziggy@sashipa.com>
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    LUA matrix extensions.
 * 
 * $Id: display_matrix.h,v 1.3 2003-03-19 05:16:16 ben Exp $
 */

#include "display_driver.h"

#ifndef _DISPLAY_MATRIX_H_
#define _DISPLAY_MATRIX_H_

/** @defgroup dcplaya_display_mtx_exe_plugin_devel  LUA matrix extensions
 *  @ingroup  dcplaya_display_exe_plugin_devel
 *  @author   Vincent Penne <ziggy@sashipa.com>
 *  @author   Benjamin Gerard <ben@sashipa.com>
 *  @brief    Fast matrix for LUA.
 *  @see      dcplaya_matrix_devel
 */

/** LUA tag value for matrix type.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
extern int matrix_tag;

/** Call it at driver init.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
int display_matrix_init(void);

/** Call it at driver shutdown.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
int display_matrix_shutdown(void);

/** lua matrix definition type.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
typedef struct
{
  int refcount;       /**< Matrix reference counter'.               */
  unsigned int l;     /**< Matrix number of line (vectors).         */
  unsigned int c;     /**< Matrix number of column.                 */
  unsigned int align; /**< 16 bit alignment.                        */
  float v[16];        /**< Matrix elements.                         */
} lua_matrix_def_t;

/** lua matrix type.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
typedef struct {
  lua_matrix_def_t *md; /**< Matrix definition attached to this matrix.   */
  float * li;           /**< 0:matrix, ~0:vector in md.                   */
} lua_matrix_t;

/** Check if LUA stack argurment is a matrix or exit.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
#define CHECK_MATRIX(i) \
  if (lua_tag(L, i) != matrix_tag) { \
    printf("%s : argument #%d is not a matrix\n", __FUNCTION__, i); \
    return 0; \
  }

/** Get a matrix and its definition from LUA stack with optionnal dimension
 *  checking.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
#define GET_MATRIX(M,MD,i,j,k) \
    (MD) = 0;  /* Avoid warning */ \
    (M) = (lua_matrix_t *)lua_touserdata(L, i); \
    if (!(M) || ((MD) = (M)->md), !(MD)) { \
      printf("%s : null pointer\n", __FUNCTION__);\
      return 0; \
    } \
    if ( (M)->li ) { \
      printf("%s : matrix is a vector\n", __FUNCTION__); \
      return 0; \
    } \
    if (j && j != (MD)->l) { \
      printf("%s : invalid number of line. %d differs from %d\n", \
             __FUNCTION__, (MD)->l, j); \
      return 0; \
    } \
    if (k && k != (MD)->c) { \
      printf("%s : invalid number of column. %d differs from %d\n", \
             __FUNCTION__, (MD)->c, k); \
      return 0; \
    }

/** Get a vector (matrix line) and its definition from LUA stack with optionnal
 *  dimension checking.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
#define GET_VECTOR(M,MD,i,k) \
    (MD) = 0; /* Avoid warning */ \
    (M) = (lua_matrix_t *)lua_touserdata(L, i); \
    if (!(M) || ((MD) = (M)->md), !(MD)) { \
      printf("%s : null pointer\n", __FUNCTION__);\
      return 0; \
    } \
    if ( !(M)->li ) { \
      printf("%s : not a vector\n", __FUNCTION__); \
      return 0; \
    } \
    if (k && k != (MD)->c) { \
      printf("%s : invalid number of column. %d differs from %d\n", \
             __FUNCTION__, (MD)->c, k); \
      return 0; \
    }

/** Get either a matrix or a vector (matrix line) and its definition from LUA
 *  stack with optionnal dimension checking.
 *  @ingroup dcplaya_display_mtx_exe_plugin_devel
 */
#define GET_MATRIX_OR_VECTOR(M,MD,i,k) \
    (MD) = 0;  /* Avoid warning */ \
    (M) = (lua_matrix_t *)lua_touserdata(L, i); \
    if (!(M) || ((MD) = (M)->md), !(MD)) { \
      printf("%s : null pointer\n", __FUNCTION__);\
      return 0; \
    } \
    if (k && k != (MD)->c) { \
      printf("%s : invalid number of column. %d differs from %d\n", \
             __FUNCTION__, (MD)->c, k); \
      return 0; \
    }


#endif /* #define _DISPLAY_MATRIX_H_ */
