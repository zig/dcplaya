/**
 * dreammp3 - Plugin loader
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: plugin.h,v 1.1.1.1 2002-08-26 14:15:00 ben Exp $
 */

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "any_driver.h"

any_driver_t * plugin_load(const char *fname);
int plugin_path_load(const char *path, int max_recurse);

#endif
