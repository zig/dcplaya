#include <stdio.h>

#include "kos/fs.h"
#include "file_wrapper.h"

static const int verbose = 0;

FILE *fopen(const char *name, const char *attr)
{
  file_t fd;
  
  if (verbose) dbglog(DBG_DEBUG, ">> " __FUNCTION__ "('%s','%s')\n", name, attr);
  fd = fs_open(name, O_RDONLY);
  if (verbose) dbglog(DBG_DEBUG, "<< " __FUNCTION__ " [fd=%p]\n", fd);
  return (FILE *) fd;
}

int fclose(FILE *stream)
{
  file_t fd = (file_t)stream;
  
  if (verbose) dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(fd=%p)\n", fd);
  fd = fd ? fs_close(fd),0 : -1;
  if (verbose) dbglog(DBG_DEBUG, "<< " __FUNCTION__ "(err=%d)\n", fd);
  
  return 0;
}

int fread(void *ptr,int size, int nmemb, FILE *stream)
{
  file_t fd = (file_t)stream;
  int read = 0, memb = 0, total = 0;
  
  total =  size*nmemb;
  if (verbose) dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(fd=%p,%d)\n", fd, total);
  
  if (total) {
    read = fs_read(fd, ptr, total);
    memb = read / size;
  }
  if (verbose) dbglog(DBG_DEBUG, "<< " __FUNCTION__ "(read=%d/%d, rem=%d)\n", read, total, memb);
  return memb;
}

int fseek(FILE *stream, long offset, int whence)
{
  file_t fd = (file_t)stream;
  
  if (verbose) dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%d,%d,%d)\n", fd, offset, whence);
  fd = fs_seek(fd, offset, whence);
  if (verbose) dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : [%d]\n", fd);
  return fd;
}

long ftell( FILE *stream)
{
  file_t fd = (file_t)stream;
  
  if (verbose) dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%d)\n", fd);
  fd = fs_tell(fd);
  if (verbose) dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : [%d]\n", fd);
  return fd;
}

int flen(FILE *stream)
{
  file_t fd = (file_t)stream;
  
  if (verbose) dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%d)\n", fd);
  fd = fs_total(fd);
  if (verbose) dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : [%d]\n", fd);
  return fd;
}

int fgetpos(FILE *stream, fpos_t *pos)
{
  *pos = ftell(stream); 
  return 0;
}

int rewind(FILE *stream)
{
  return fseek(stream, 0L, SEEK_SET);
}

int fsetpos(FILE *stream, fpos_t *pos)
{
  return fseek(stream, *pos, SEEK_SET);
}

void clearerr( FILE *stream)
{
}
