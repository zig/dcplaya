/**
 * dreammp3 - integer math functions
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard
 *
 * $Id: math_int.c,v 1.1 2002-08-26 14:15:05 ben Exp $
 */

#include "math_int.h"

/** Small Bijection for calculate a square root */
unsigned int int_sqrt (unsigned int x)
{
  unsigned int r = 0, t;

  for (t = 0x40000000; t; t >>= 2) {
    unsigned int s = r + t;
    if (s <= x) {
     x -= s;
     r = s + t;
  	}
    r >>= 1;
  }
  return r;
}

