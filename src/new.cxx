/**
 * @name    new.cxx
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   new ans delete operators
 * @date    2002/09/27
 *
 * $Id: new.cxx,v 1.3 2003-03-10 22:55:35 ben Exp $
 */

extern "C" {
#include <malloc.h>
#include "dcplaya/config.h"
#include "sysdebug.h"
}

void * operator new (unsigned int bytes) {
//   SDDEBUG("[%s] : %u\n", __FUNCTION__, bytes);
  return malloc(bytes);
}

void operator delete (void *addr) {
//   SDDEBUG("[%s] : %p\n", __FUNCTION__, addr);
  free (addr);
}

void * operator new[] (unsigned int bytes) {
//   SDDEBUG("[%s] : %u\n", __FUNCTION__, bytes);
  return malloc(bytes);
}

void operator delete[] (void *addr) {
//   SDDEBUG("[%s] : %p\n", __FUNCTION__, addr);
  free (addr);
}

extern "C" void __cxa_pure_virtual (void)
{
  SDCRITICAL("[%s]\n", __FUNCTION__);
  BREAKPOINT(0xDEAD9456);
}
