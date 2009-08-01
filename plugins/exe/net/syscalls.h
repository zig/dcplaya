/* 
 * This file is part of the dcload Dreamcast ethernet loader
 *
 * Copyright (C) 2001 Andrew Kieschnick <andrewk@austin.rr.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

#define CMD_EXIT     "DC00"
#define CMD_FSTAT    "DC01"
#define CMD_WRITE    "DD02"
#define CMD_READ     "DC03"
#define CMD_OPEN     "DC04"
#define CMD_CLOSE    "DC05"
#define CMD_CREAT    "DC06"
#define CMD_LINK     "DC07"
#define CMD_UNLINK   "DC08"
#define CMD_CHDIR    "DC09"
#define CMD_CHMOD    "DC10"
#define CMD_LSEEK    "DC11"
#define CMD_TIME     "DC12"
#define CMD_STAT     "DC13"
#define CMD_UTIME    "DC14"
#define CMD_BAD      "DC15"
#define CMD_OPENDIR  "DC16"
#define CMD_CLOSEDIR "DC17"
#define CMD_READDIR  "DC18"
#define CMD_CDFSREAD "DC19"

static unsigned int syscall_retval;

typedef struct {
  unsigned char id[4] __attribute__ ((packed));
  unsigned int value0 __attribute__ ((packed));
  unsigned int value1 __attribute__ ((packed));
  unsigned int value2 __attribute__ ((packed));
} command_3int_t;

typedef struct {
  unsigned char id[4] __attribute__ ((packed));
  unsigned int value0 __attribute__ ((packed));
  unsigned int value1 __attribute__ ((packed));
  unsigned char string[1] __attribute__ ((packed));
} command_2int_string_t;

typedef struct {
  unsigned char id[4] __attribute__ ((packed));
  unsigned int value0 __attribute__ ((packed));
} command_int_t;

typedef struct {
  unsigned char id[4] __attribute__ ((packed));
  unsigned int value0 __attribute__ ((packed));
  unsigned char string[1] __attribute__ ((packed));
} command_int_string_t;

typedef struct {
  unsigned char id[4] __attribute__ ((packed));
  unsigned char string[1] __attribute__ ((packed));
} command_string_t;

typedef struct {
  unsigned char id[4] __attribute__ ((packed));
  unsigned int value0 __attribute__ ((packed));
  unsigned int value1 __attribute__ ((packed));
  unsigned int value2 __attribute__ ((packed));
  unsigned char string[1] __attribute__ ((packed));
} command_3int_string_t;

//void build_send_packet(int command_len);


struct dcload_dirent {
  long            d_ino;  /* inode number */
  off_t           d_off;  /* offset to the next dirent */
  unsigned short  d_reclen;/* length of this record */
  unsigned char   d_type;         /* type of file */
  char            d_name[256];    /* filename */
};

typedef struct dcload_dirent dcload_dirent_t;

#define DIR dcload_dirent_t

/* dcload stat */

struct  dcload_stat { 
  unsigned short st_dev;
  unsigned short st_ino;
  int st_mode;
  unsigned short st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
  unsigned short st_rdev;
  long st_size;
  long st_atime;
  long st_spare1;
  long st_mtime;
  long st_spare2;
  long st_ctime;
  long st_spare3;
  long st_blksize;
  long st_blocks;
  long st_spare4[2];
};

typedef struct dcload_stat dcload_stat_t;


#endif
