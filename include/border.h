/**
 * @file    border.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/02/16
 * @brief   border rendering
 */

#ifndef _BORDER_H_
#define _BORDER_H_

#include "draw/texture.h"

typedef struct {
  texid_t texid;
  uint32 align;
  struct {
    float u,v;
  } uv[3];
} borderuv_t;

int border_setup(void);

extern borderuv_t borderuv[];
extern texid_t bordertex[];
extern const unsigned int border_max;

int border_setup(void);

#endif /* #define _BORDER_H_ */
