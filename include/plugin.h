/**
 * dreammp3 - Plugin loader
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: plugin.h,v 1.2 2002-09-06 23:16:09 ben Exp $
 */

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "any_driver.h"

any_driver_t * plugin_load(const char *fname);
int plugin_path_load(const char *path, int max_recurse);

DCPLAYA_EXTERN_C_END

#endif
