/**
 * @file    filename.c
 * @author  benjaimn gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   filename utilities.
 *
 * $Id: filename.c,v 1.3 2002-12-04 10:47:25 ben Exp $
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

const char *fn_secondary_ext(const char *pathname, const char *ext)
{
  const char * e , * p;
  if (!pathname) {
    return 0;
  }
  e = strrchr(pathname,'.');
  p = strrchr(pathname,'/');
  if (e>p && (!ext || !stricmp(e,ext))) {
	const char * e2;
	if (!p) {
	  p = pathname;
	}
	for (e2 = e-1; e2 >= p; --e2) {
	  if (*e2 == '.') {
		e = e2;
		break;
	  }
	}
  }
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

static int is_slash(int c)
{
  return c == '/' || c == '\\';
}

static const char * skip_slash(const char * s)
{
  int c;
  while (c=*s, is_slash(c)) {
	++s;
  }
  return s;
}

static int get_name_length(const char * s)
{
  const char * e = s;
  int c;

  while (c=*s, (c && !is_slash(c))) {
	++s;
  }
  return s-e;
}

static char * backward_slash(char *s, char *e)
{
  while (e > s && !is_slash(*--e))
	;
  return e;
}

static char * copy_path(char *dst, const char * src, int max)
{
  char * dst_start = dst, * dst_stop;
/*   int prev_slash = 0; */

  if (max <= 0) {
	return 0;
  }

  /* Beginning slash is unremovable */
  if (is_slash(*src)) {
	*dst++ = '/';
	--max;
	src = skip_slash(src);
  }
  dst_stop = dst;

  if (max <= 0) {
	return 0;
  }

  for ( ; ; ) {
	int c, len;
	const char * name;

	*dst = 0;
/* 	printf("[%s] [%s] [%d]\n", dst_start,src,prev_slash); */

	/* Get file. */
	len = get_name_length(src);
	if (!len) {
	  break;
	}

	name = src;
	src += len;

/* 	has_slash = is_slash(*src); */
	src = skip_slash(src);

	c = *name;
	if (len == 1 && c == '.') {
	  /* '.' */
	  continue;
	} else if (len == 2 && c == '.' && name[1] == '.') {
	  /* '..' */
	  char * d;
	  int len;
	  d = backward_slash(dst_stop, dst);
	  len = dst - d;
	  max += len;
	  dst = d;
/* 	  prev_slash = d > dst_stop; */
	  continue;
	}

	/* Copy slash */
	if (dst > dst_stop) {
	  if (1 >= max) {
		dst_start = 0;
		break;
	  } else {
		*dst++ = '/';
		--max;
	  }
	}

	if (len >= max) {
	  dst_start = 0;
	  break;
	}

	/* Copy name */
	memcpy(dst, name, len);
	max -= len;
	dst += len;
	
	/* store slash. */
/* 	prev_slash = has_slash; */
  }
  *dst = 0;
  return dst_start;
}

char * fn_canonical(char * dst, const char * name, int max)
{
  return copy_path(dst, name, max);
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

