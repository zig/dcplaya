/* 2002/02/21 */

#ifndef _REMANENS_H_
#define _REMANENS_H_

#include "matrix.h"
#include "obj3d.h"

typedef struct remanens_s
{
  unsigned int  frame;
  matrix_t      mtx;
  obj_t         *o;
} remanens_t;

void remanens_clean(void);
int remanens_setup(void);
void remanens_push(obj_t *o, matrix_t mtx, unsigned int frame);
void remanens_remove_old(unsigned int threshold_frame);
int remanens_nb();
remanens_t *remanens_get(int id);

#endif /* #define _REMANENS_H_ */
