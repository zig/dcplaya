/**
 * @file    file_utils.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   File manipulation utilities.
 *
 * $Id: file_utils.c,v 1.1 2002-09-30 20:03:09 benjihan Exp $
 */

#include "file_utils.h"
#include <kos/fs.h>

const char * fu_strerr(int err)
{
  switch (err) {
  case FU_INVALID_PARM: return "invalid parm";
  case FU_OPEN_ERROR:   return "open error";
  case FU_DIR_EXISTS:   return "dir exists";
  case FU_FILE_EXISTS:  return "file exists";
  case FU_READ_ERROR:   return "read error";
  case FU_WRITE_ERROR:  return "write error";
  case FU_CREATE_ERROR: return "create error";
  case FU_UNLINK_ERROR: return "unlink error";
  case FU_OK:           return "success";
  case FU_ERROR:        return "general error";
  }
  return "unknown error";
}

int fu_open_test(const char *fname, int mode)
{
  int fd, ret;
  
  fd = (fname) ? fs_open(fname, mode) : 0;
  ret = fd != 0;
  if (fd) {
    fs_close(fd);
  }
  return ret;
}

int fu_is_regular(const char *fname)
{
  return fu_open_test(fname, O_RDONLY);
}

int fu_is_dir(const char *fname)
{
  return fu_open_test(fname, O_DIR | O_RDONLY);
}

int fu_exist(const char *fname)
{
  return fu_is_regular(fname) || fu_is_dir(fname);
}

int fu_remove(const char *fname)
{
  if (!fname || !fname[0]) {
    return FU_INVALID_PARM;
  }
  return fs_unlink(fname) ? FU_UNLINK_ERROR : 0;
}

int fu_unlink(const char *fname)
{
  return fu_remove(fname);
}

static int filecopy(const char * dstname, const char * srcname,
		    int force, int unlink)
{
  char buf[2048];
  int fds = 0, fdd = 0;
  int err = 0, cnt = 0, n;

  /* Test parameters */
  if (!dstname || !srcname || !dstname[0] || !srcname[0]) {
    err = FU_INVALID_PARM;
    goto error;
  }

  /* Test if target is a directory */
  if (fu_is_dir(dstname) ) {
    err = FU_DIR_EXISTS;
    goto error;
  }

  /* Test if target exist (It should be a file) */
  if (fu_exist(dstname)) {
    if (!force) {
      err = FU_FILE_EXISTS;
      goto error;
    } else {
      /* Force mode : remove existing target file */
      err = fu_remove(dstname);
      if (err < 0) {
	goto error;
      }
    }
  }

  /* Open source. */
  fds = fs_open(srcname, O_RDONLY);
  if (!fds) {
    err = FU_OPEN_ERROR;
    goto error;
  }

  /* Open destination. */
  fdd = fs_open(dstname, O_WRONLY);
  if (!fdd) {
    err = FU_CREATE_ERROR;
    goto error;
  }

  do {
    n = fs_read(fds, buf, sizeof(buf));
    if (n < 0) {
      err = FU_READ_ERROR;
    } else if (n > 0) {
      if (fs_write(fdd,buf,n) != n) {
	n = -1;
	err = FU_WRITE_ERROR;
      }
      cnt += n;
    }
  } while (n > 0);

 error:
  if (fds) {
    fs_close(fds);
  }
  if (fdd) {
    fs_close(fdd);
    if (err < 0) {
      /* On error, remove corrupt destination file. */
      fu_remove(dstname);
    }
  }
  /* No error and unlink mode, remove source file. */
  if (!err && unlink) {
    err = fu_remove(srcname);
  }

  return err ? err : cnt;
}

int fu_copy(const char * dstname, const char * srcname, int force)
{
  return filecopy(dstname, srcname, force, 0);
}

int fu_move(const char * dstname, const char * srcname, int force)
{
  return filecopy(dstname, srcname, force, 1);
}

int fu_create_dir(const char *dirname)
{
  file_t fd;

  if (!dirname || !dirname[0]) {
    return FU_INVALID_PARM;
  }

  fd  = fs_open(dirname, O_WRONLY | O_DIR);
  if (fd) {
    fs_close(fd);
  } else {
    return FU_CREATE_ERROR;
  }
  return fd ? 0 : FU_CREATE_ERROR;
}
