/**
 * @file    filename.c
 * @author  benjaimn gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   filename utilities.
 *
 * $Id: filename.c,v 1.1 2002-09-30 20:03:09 benjihan Exp $
 */

#include <string.h>
#include "filename.h"

const char *fn_ext(const char *pathname)
{
  const char * e , * p;
  if (!pathname) {
    return 0;
  }
  e = strrchr(pathname,'.');
  p = strrchr(pathname,'/');
  return (e>p) ? e : pathname + strlen(pathname);
}

const char *fn_basename(const char *pathname)
{
  const char * s;

  if (!pathname) {
    return 0;
  }
  s = strrchr(pathname, '/');
  return s ? s+1 : pathname;
}

const char * fn_leafname(const char * pathname)
{
  return fn_basename(pathname);
}

int fn_is_absolute(const char *pathname)
{
  return pathname && pathname[0] == '/';
}

int fn_is_relative(const char * pathname)
{
  return pathname && pathname[0] != '/';
}

char * fn_get_path(char *path, const char *pathname, int max, int * isslash)
{
  int len, slash = 0, c;

  if (!path || !pathname) {
    return 0;
  }

  len = strlen(pathname);
  if (len > 0 && pathname[len-1] == '/') {
    slash = 1;
    --len;
  }
  if (len > --max) {
    return 0;
  }

  memcpy(path, pathname, len);
  path[len] = 0;
  if (isslash) {
    *isslash = slash;
  }

  while (c = *path, c) {
    if (c == '\\') {
      *path = '/';
    }
    ++path;
  }
  return path;
}

char * fn_add_path(char *path, char *pathend, const char *leafname, int max)
{
  int len, c;

  if (!path || !leafname) {
    return 0;
  }
  if (!pathend) {
    pathend = path + strlen(path);
  }
  max -= pathend - path;
  len = strlen(leafname) + 1;
  if (len >= max) {
    return 0;
  }
  *pathend++ = '/';
  while (c = *leafname++, c) {
    if(c == '\\') {
      c = '/';
    }
    *pathend++ = c;
  }
  *pathend = c;
  return pathend;
}

