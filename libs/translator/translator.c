#include "translator.h"

SHAwrapperImage_t * LoadImageFile(const char * fname)
{
  return SHAwrapperLoadFile(fname);
}

SHAwrapperImage_t * LoadImageMemory(void *buffer, int size)
{
  return SHAwrapperLoadMemory(buffer, size);
}
