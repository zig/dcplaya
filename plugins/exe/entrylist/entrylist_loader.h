/**
 * @ingroup  exe_plugin
 * @file     entrylist_loader.h
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/24
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_loader.h,v 1.3 2002-11-14 23:40:27 benjihan Exp $
 */

#ifndef _ENTRY_LOADER_H_
#define _ENTRY_LOADER_H_

#include "entrylist.h"

/* typedef int (*el_filter_f)(el_entry_t * e); */

typedef union {
  int all;
  struct {
	int dir      : 1;
	int regular  : 1;
	int playlist : 1;
	int plugins  : 1;
	int playable : 1;
  };
} el_filter_t;


int el_loader_init(void);
void el_loader_shutdown(void);
int el_loader_loaddir(el_list_t * el, const char * path, el_filter_t filter);

#endif /* #define _ENTRY_LOADER_H_ */

