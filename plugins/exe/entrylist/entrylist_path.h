/**
 * @ingroup  dcplaya_el_exe_plugin_devel
 * @file     entrylist_path.h
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list pathes
 * 
 * $Id: entrylist_path.h,v 1.2 2003-03-19 05:16:16 ben Exp $
 */

#ifndef _ENTRYLIST_PATH_H_
#define _ENTRYLIST_PATH_H_

#include "iarray.h"

/** entry-list path.
 * @ingroup  dcplaya_el_exe_plugin_devel
 */
typedef struct _el_path_s {
  struct _el_path_s * next; /**< Next path.        */
  struct _el_path_s * prev; /**< Previous path.    */
  int refcount;             /**< Reference count.  */
  char path[4];             /**< Path name buffer. */
} el_path_t;

/** entry-list pathes list head.
 *  @ingroup  dcplaya_el_exe_plugin_devel
 */
extern el_path_t * pathes;

int elpath_init(void);
void elpath_shutdown(void);
el_path_t * elpath_add(const char * path);
void elpath_del(el_path_t *elpath);
el_path_t * elpath_addref(el_path_t *path);


#endif /* #define _ENTRYLIST_PATH_H_ */
