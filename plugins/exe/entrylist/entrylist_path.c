/**
 * @ingroup  exe_plugin
 * @file     entrylist_path.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/31
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_path.c,v 1.1 2002-11-05 08:50:52 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "entrylist_path.h"
#include "mutex.h"

static int init;

static mutex_t mutex;
el_path_t * pathes;

int elpath_init(void)
{
  if (init) {
	return 0;
  }
  pathes = 0;
  mutex_init(&mutex,0);
  init = 1;
  return 0;
}

void elpath_shutdown(void)
{
  el_path_t * e, * n;
  if (!init) {
	return;
  }
  for (e=pathes; e; e=n) {
	n = e->next;
	free(e);
  }
  pathes = 0;
  init = 0;
}

el_path_t * elpath_addref(el_path_t *path)
{
  if (path) {
	mutex_lock(&mutex);
	++path->refcount;
	mutex_unlock(&mutex);
  }
  return path;
}

el_path_t * elpath_add(const char * path)
{
  el_path_t *e;

  if (!path) {
	return 0;
  }
  mutex_lock(&mutex);

/*   printf("elpath_add(%s)\n", path); */
  for (e=pathes; e && strcmp(path,e->path); e=e->next)
	;
  if (!e) {
	int len, size;

	len  = strlen(path)+1;
	size = sizeof(*e) - sizeof(e->path) + len;
	e = malloc(size);
	if (e) {

/* 	  printf("Create a new path : (%p) '%s'\n", e, path); */

	  e->refcount = 1;
	  memcpy(e->path, path, len);
	  e->prev = 0;
	  e->next = pathes;
	  pathes = e;
	}
  } else {
	++e->refcount;
/* 	printf("Reference path : (%p) '%s' -> %d\n", e, e->path, e->refcount); */
  }
  mutex_unlock(&mutex);
  return e;
}

void elpath_del(el_path_t *path)
{
  if (path) {
	mutex_lock(&mutex);
/* 	printf("Dereference path : (%p) '%s' -> %d\n", path, path->path, */
/* 		   path->refcount-1); */

	if (--path->refcount <= 0) {
	  el_path_t * n, * p;

/* 	  printf("Remove path : (%p)\n", path); */

	  n = path->next;
	  p = path->prev;
	  if (n) {
		n->prev = p;
	  }
	  if (p) {
		p->next = n;
	  } else {
		pathes = n;
	  }
	}
	mutex_unlock(&mutex);
  }
}
