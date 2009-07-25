#include <stdlib.h>

#include "sha123/alloc.h"

void * sha123_alloc(unsigned int n)
{
  return malloc(n);
}

void sha123_free(void * data)
{
  if (data) {
    free(data);
  }
}
