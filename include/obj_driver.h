#ifndef _OBJ_DRIVER_H_
#define _OBJ_DRIVER_H_

#include "any_driver.h"
#include "obj3d.h"

/** 3D-Object driver */
typedef struct {
  any_driver_t common; /**< All driver common structure */
  obj_t        obj;    /**< 3d object definition */
} obj_driver_t;

/* This fonctions are implemented in obj3d.c */
extern int obj3d_init(any_driver_t *);
extern int obj3d_shutdown(any_driver_t *);
extern driver_option_t * obj3d_options(any_driver_t * driver, int idx,
                                       driver_option_t * opt);


#endif
