/**
 * @ingroup  exe_plugin
 * @file     entrylist_path.h
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_path.h,v 1.1 2002-11-05 08:50:52 benjihan Exp $
 */

#ifndef _ENTRYLIST_PATH_H_
#define _ENTRYLIST_PATH_H_

#include "iarray.h"


typedef struct _el_path_s {
  struct _el_path_s * next;
  struct _el_path_s * prev;
  int refcount;
  char path[4];
} el_path_t;

extern el_path_t * pathes;

int elpath_init(void);
void elpath_shutdown(void);
el_path_t * elpath_add(const char * path);
void elpath_del(el_path_t *elpath);
el_path_t * elpath_addref(el_path_t *path);


#endif /* #define _ENTRYLIST_PATH_H_ */
