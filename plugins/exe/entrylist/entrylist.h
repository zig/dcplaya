/**
 * @ingroup  exe_plugin
 * @file     entrylist.h
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist.h,v 1.3 2002-11-04 22:41:53 benjihan Exp $
 */

#ifndef _ENTRYLIST_H_
#define _ENTRYLIST_H_

#include "iarray.h"
#include "entrylist_path.h"

/** Entrylist standard entries as provided by entries allocator. */
typedef struct {
  int  type;         /**< File Type.                                 */
  int  size;         /**< Size of entry in bytes (-1 for dir).       */
  int  iname;        /**< Name (displayed). Points into buffer.      */
  int  ifile;        /**< Filename (leaf only).  Points into buffer. */
  el_path_t * path ; /**< Entry path.                                */
  char buffer[128];  /**< String buffer.                             */ 
} el_entry_t;

/** Entrylist type. */
typedef struct {
  iarray_t a;         /**< Holds entrylist elements.                */
  el_path_t * path ;  /**< The path used to fill this entry list.   */
  int loading;        /**< Set if currently loading.                */ 
} el_list_t;

el_list_t * entrylist_create(void);
void entrylist_destroy(el_list_t * el);

void entrylist_lock(el_list_t * el);
void entrylist_unlock(el_list_t * el);
int entrylist_trylock(el_list_t * el);
int entrylist_lockcount(el_list_t * el);

int entrylist_clear(el_list_t * el);
int entrylist_remove(el_list_t * el, int idx);
int entrylist_set(el_list_t * el, int idx, el_entry_t * e, int eltsize);
int entrylist_insert(el_list_t * el, int idx, el_entry_t * e, int eltsize);

#endif /* #define _ENTRYLIST_H_ */
