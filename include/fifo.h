/* $Id: fifo.h,v 1.2 2002-09-06 23:16:09 ben Exp $ */

#ifndef _FIFO_H_
#define _FIFO_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


void fifo_init();
int fifo_start();
void fifo_stop();

void fifo_read_lock(int *i1, int *n1, int *i2, int *n2);
void fifo_write_lock(int *i1, int *n1, int *i2, int *n2);
void fifo_unlock();

int fifo_free();
int fifo_size();
int fifo_used();

void fifo_state(int *r, int *w, int *b);
int fifo_fill();
int fifo_read(int *buf, int n);
int fifo_write(const int *buf, int n);
int fifo_write_mono(const short *buf, int n);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FIFO_H_ */
