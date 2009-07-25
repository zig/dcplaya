
#ifdef SHA123_DEBUG

#include <stdio.h>
#include <stdarg.h>
#include "sha123/debug.h"

void sha123_debug(const char * fmt, ...)
{
  va_list list;

  va_start(list, fmt);
  fprintf(stderr, "[sha123] : ");
  vfprintf(stderr, fmt, list);
  va_end(list);
}

#endif /* #ifdef SHA123_DEBUG */

