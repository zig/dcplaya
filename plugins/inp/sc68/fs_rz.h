/**
 * @file    fs_rz.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2003/03/07
 * @brief   Gzipped compressed rom disk for KOS file system
 * 
 * $Id: fs_rz.h,v 1.1 2003-03-08 18:25:40 ben Exp $
 */

#ifndef _FS_RZ_H_
# define _FS_RZ_H_

/* Initialize the file system */
int fs_rz_init(const unsigned char * romdisk);

/* De-init the file system */
int fs_rz_shutdown(void);

#endif /* #define _FS_RZ_H_ */
