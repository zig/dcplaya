/**
 * @ingroup dcplaya_ramdisk_devel
 * @file    fs_ramdisk.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   RAM disk for KOS file system
 * 
 * $Id: fs_ramdisk.h,v 1.3 2003-03-22 00:35:27 ben Exp $
 */

/** @defgroup dcplaya_ramdisk_devel Ramdisk filesystem
 *  @ingroup dcplaya_devel
 *  @brief   Ramdisk filesystem
 *
 * @author  benjamin gerard <ben@sashipa.com>
 */

#ifndef _FS_RAMDISK_H_
#define _FS_RAMDISK_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#include <arch/types.h>
#include <kos/fs.h>

/** @name Initialization functions
 *  @ingroup dcplaya_ramdisk_devel
 *  @{
 */

/** Initialize the RAMdisk filesystem.
 *
 *  @param max_size Maximum size of ramdisk (0:default) (NOT USED!!)
 *  @return error-code
 *  @retval 0 success
 *  @retval -1 error
 */
int fs_ramdisk_init(int max_size);

/** Shutdown the RAMdisk filesystem.
 */
int fs_ramdisk_shutdown(void);

/**@}*/

/** @name Mofication notication functions
 *  @ingroup dcplaya_ramdisk_devel
 *  @{
 */

/** Read and clear the modification status. */
int fs_ramdisk_modified(void);

/** Set the notification path.
 *
 *    The fs_ramdisk_notify_path() function set the path from which
 *    modification will be notified. The path must be a valid directory
 *    of the ramdisk.
 *
 *  @param  path  new notification path (0 for none) with or without heading
 *                "/ram". 
 *
 *  @return error-code
 *  @retval 0 success
 *  @retval -1 error (directory does not exist)
 */
int fs_ramdisk_notify_path(const char *path);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FS_RAMDISK_H_ */
