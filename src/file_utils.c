/**
 * @file    file_utils.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/30
 * @brief   File manipulation utilities.
 *
 * $Id: file_utils.c,v 1.2 2002-10-03 02:37:26 benjihan Exp $
 */

/* #include "sysdebug.h" */

#include "file_utils.h"
#include <kos/fs.h>
#include <string.h>
#include <stdlib.h>

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
  case FU_ALLOC_ERROR:  return "alloc error";
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

typedef struct _fu_linkeddirentry_s
{
  struct _fu_linkeddirentry_s *nxt;
  fu_dirent_t entry;
} fu_linkeddirent_t;

static int r_readir(int fd, fu_filter_f filter, fu_linkeddirent_t *elist,
		    fu_dirent_t ** res) 
{
  dirent_t *dir;
  fu_linkeddirent_t local, *e;
  int count;

  while (dir=fs_readdir(fd), dir) {
    /* Copy direntry */
/*     SDDEBUG("entry [%s] [%d]\n", dir->name, dir->size); */

    memset(&local,0,sizeof(local));
    strncpy(local.entry.name,dir->name,sizeof(local.entry.name)-1);
    local.entry.size = dir->size;
    /* Filter */
    if (filter(&local.entry)) {
      continue;
    }
/*     SDDEBUG("-->Accepted [%s] [%d]\n", local.entry.name, local.entry.size); */

    local.nxt = elist;
    /* Continue ... */
    return r_readir(fd, filter, &local, res);
  }
  *res = 0;
      
/*   SDDEBUG("count:\n"); */
  /* Count entries */
  for (e=elist, count=0; e; ++count, e=e->nxt)
    ;
/*   SDDEBUG("-->%d\n", count); */

  if (count) {
    fu_dirent_t * result;

    result = (fu_dirent_t *)malloc(count * sizeof(fu_dirent_t));
    if (result) {
      int i;
      for (i=count-1, e=elist; e; e=e->nxt, --i) {
	result[i].size = e->entry.size;
	memcpy(result[i].name, e->entry.name, sizeof(e->entry.name));
      }
      *res = result;
    } else {
      count = FU_ALLOC_ERROR;
    }
  }
  return count;
}

/* Default readdir filter, accept all except null pointer */
static int readir_default_filter(const fu_dirent_t *dir)
{
  return dir ? 0 : -1;
}

int fu_read_dir(const char *dirname, fu_dirent_t **res, fu_filter_f filter)
{
  int fd, count;

/*   SDDEBUG("[%s] : [%s]\n", __FUNCTION__, dirname); */

  if (!dirname || !dirname[0] || !res) {
    return FU_INVALID_PARM;
  }

  if (!filter) {
    filter = readir_default_filter;
  }

  *res = 0;
  fd = fs_open(dirname, O_RDONLY | O_DIR);
  if (!fd) {
    return FU_OPEN_ERROR;
  }
  count = r_readir(fd, filter, 0, res);
  fs_close(fd);

/*   SDDEBUG("[%s] : [%s] := [%d]\n", __FUNCTION__, dirname, count); */

  return count;
}

int fu_sortdir_by_name_dirfirst(const fu_dirent_t *a, const fu_dirent_t *b)
{
  if (a==b) {
    return 0;
  }
  if (!a) {
    return -1;
  }
  if (!b) {
    return 1;
  }

  if ( (a->size ^ b->size) < 0) {
    /* Comparing dir / regular */
    return a->size - b->size;
/*     return (((unsigned int)a->size < (unsigned int) b->size) << 1) - 1; */
  } else {
    /* Comparing same type */
    return strnicmp(a->name, b->name, sizeof(a->name));
  }
}

int fu_sortdir_by_name(const fu_dirent_t *a, const fu_dirent_t *b)
{
  if (a==b) {
    return 0;
  }
  if (!a) {
    return -1;
  }
  if (!b) {
    return 1;
  }
  return strnicmp(a->name, b->name, sizeof(a->name));
}

int fu_sortdir_by_descending_size(const fu_dirent_t *a, const fu_dirent_t *b)
{
  const int mask = ~(1 << ((sizeof(int)<<3)-1));
  if (a==b) {
    return 0;
  }
  if (!a) {
    return -1;
  }
  if (!b) {
    return 1;
  }
  return (b->size & mask) - (a->size & mask);
}

int fu_sortdir_by_ascending_size(const fu_dirent_t *a, const fu_dirent_t *b)
{
  if (a==b) {
    return 0;
  }
  if (!a) {
    return -1;
  }
  if (!b) {
    return 1;
  }
  return a->size - b->size;
}

typedef int (*fu_qsort_f)(const void *, const void *);

int fu_sort_dir(fu_dirent_t *dir, int entries, fu_sortdir_f sortdir)
{
  if (!dir || entries<0) {
    return FU_INVALID_PARM;
  }

  if (!sortdir) {
    sortdir = fu_sortdir_by_name_dirfirst;
  }

  qsort(dir, entries, sizeof(*dir), (fu_qsort_f)sortdir);
  return FU_OK;
}
