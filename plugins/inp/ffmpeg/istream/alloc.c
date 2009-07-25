#include <stdlib.h>

#include "istream/alloc.h"

void * istream_alloc(unsigned int n)
{
  return malloc(n);
}

void istream_free(void * data)
{
  if (data) {
    free(data);
  }
}
