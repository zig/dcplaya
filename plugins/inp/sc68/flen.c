
#include <kos/fs.h>
#include <stdio.h>

int flen(FILE *stream)
{
  file_t fd = (file_t)stream;
  
  fd = fs_total(fd);
  return fd < 0 ? -1 : fd;
}

