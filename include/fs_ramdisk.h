/**
 * @file    fs_ramdisk.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   RAM disk for KOS file system
 * 
 * $Id: fs_ramdisk.h,v 1.1 2002-09-17 19:58:50 ben Exp $
 */

#ifndef _FS_RAMDISK_H_
#define _FS_RAMDISK_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#include <arch/types.h>
#include <kos/fs.h>

/** Initialize the RAMdisk file system.
 */
int fs_ramdisk_init(int max_size);

/** De-init the RAMdisk file system.
 */
int fs_ramdisk_shutdown(void);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _FS_RAMDISK_H_ */
