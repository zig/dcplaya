/**
 * @ingroup  dcplaya_el_exe_plugin_devel
 * @file     entrylist_loader.h
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/24
 * @brief    entry-list loader thread.
 * 
 * $Id: entrylist_loader.h,v 1.5 2003-03-19 05:16:16 ben Exp $
 */

#ifndef _ENTRY_LOADER_H_
#define _ENTRY_LOADER_H_

#include "entrylist.h"

int el_loader_init(void);
void el_loader_shutdown(void);
int el_loader_loaddir(el_list_t * el, const char * path, int filter);

#endif /* #define _ENTRY_LOADER_H_ */

