/**
 * @file    plugin.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   plugin loader
 *
 * $Id: plugin.h,v 1.4 2002-09-14 04:30:55 ben Exp $
 */

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#include "any_driver.h"

/** Load a plugin file.
 *
 *    The plugin_load() function loads an lef plugin file, add a reference 
 *    count for all valid drivers found in it and returns a list containing
 *    all of them.
 *
 * @param  fname  Name of lef file to load.
 *
 * @return  First driver of linked list. A lef file driver could have more
 *          than one driver.
 * @retval  0:failure, no suitable driver found or invalid lef file.
 *
 */
any_driver_t * plugin_load(const char *fname);

/** Load and register driver of a lef pluginfile.
 *
 *    The plugin_load_and_register() function call the plugin_load() function.
 *    Each of the driver form the returned list is initialized and registred
 *    into the suitable driver list with driver_list_register() functions.
 *    Existing old driver will be properly removed. The number of driver
 *    properly registred is returned.
 *
 * @param  fname  Name of lef file to load.
 *
 * @return  number of driver registered.
 * @retval  0 No driver has been registered. The causes may be multiple : 
 *            invalid file, no driver has been successfully initialized or
 *            registred ...
 */
int plugin_load_and_register(const char *fname);

/** Load and register all drivers in all .lef recursively from a path.
 *
 *    The plugin_path_load() function scans directory with a maximum depth
 *    of max_recurse. Each *.lef matching file is loaded by the
 *    plugin_load_and_register() functions. A zero max_recurse causes no
 *    depth limitation.
 *
 * @param  path         Path of directory to load.
 * @param  max_recurse  Maximum depth of sub-directory recursion.
 *                      Set to 0 for no limit.
 *
 * @return  number of driver registered.
 * @retval  0 No driver has been registered. The causes may be multiple : 
 *            invalid path, no valid file found, no driver has been
 *            successfully initialized or registred ...
 *
 */
int plugin_path_load(const char *path, int max_recurse);

DCPLAYA_EXTERN_C_END

#endif
