/**
 * @ingroup  exe_plugin
 * @file     display_matrix.h
 * @author   Vincent Penne <ziggy@sashipa.com>
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, matrix interface
 * 
 * $Id: display_matrix.h,v 1.2 2002-10-22 10:35:47 benjihan Exp $
 */

#include "display_driver.h"

#ifndef _DISPLAY_MATRIX_H_
#define _DISPLAY_MATRIX_H_

extern int matrix_tag;

/** Call it at driver init. */
int display_matrix_init(void);

/** Call it at driver shutdown. */
int display_matrix_shutdown(void);

/** lua matrix definition type.*/
typedef struct
{
  int refcount;       /**< Matrix reference counter'.               */
  unsigned int l;     /**< Matrix number of line (vectors).         */
  unsigned int c;     /**< Matrix number of column.                 */
  unsigned int align; /**< 16 bit alignment.                        */
  float v[16];        /**< Matrix elements.                         */
} lua_matrix_def_t;

/** lua matrix type. */
typedef struct {
  lua_matrix_def_t *md; /**< Matrix definition attached to this matrix.   */
  float * li;           /**< 0:matrix, ~0:vector in md.                   */
} lua_matrix_t;

#define CHECK_MATRIX(i) \
  if (lua_tag(L, i) != matrix_tag) { \
    printf("%s : argument #%d is not a matrix\n", __FUNCTION__, i); \
    return 0; \
  }

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
