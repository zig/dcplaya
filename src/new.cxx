/**
 * @name    new.cxx
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   new ans delete operators
 * @date    2002/09/27
 *
 * $Id: new.cxx,v 1.1 2002-09-27 08:16:00 benjihan Exp $
 */

extern "C" {
#include <malloc.h>
}

void * operator new (unsigned int bytes) {
  return malloc(bytes);
}

void operator delete (void *addr) {
  free (addr);
}

void * operator new[] (unsigned int bytes) {
  return malloc(bytes);
}

void operator delete[] (void *addr) {
  free (addr);
}
