/**
 * @name     gzip.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @date     2002/09/20
 * @brief    Simple gzipped file access.
 *
 * $Id: gzip.c,v 1.1 2002-09-20 00:22:14 benjihan Exp $
 */

#include <kos/fs.h>
#include <string.h>
#include <stdlib.h>

#include "zlib.h"
#include "sysdebug.h"
#include "gzip.h"

static int is_gz(int fd, int len)
{
  static char gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */
  char magic[2];
  int inflate_len = -1;
  
  /* header + inflate len */
  if (len < 10+4) {
    goto error;
  }

  if (fs_read(fd, magic, 2) != 2 || memcmp(magic, gz_magic, 2)) {
    goto error;
  }

  /* Get uncompressed size at the end of file.
   * $$$ ben: architecture !!!
   */
  fs_seek(fd, -4, SEEK_END);
  if(fs_read(fd, &inflate_len, 4) != 4 || inflate_len <= 0) {
    goto error;
  }

 error:
  if (inflate_len < 0) {
    inflate_len = -1;
    SDDEBUG("Not gz file.\n");
  } else {
    SDDEBUG("gz file : inflate size=%d.\n", inflate_len);
  }
  fs_seek(fd,0,SEEK_SET);
  return inflate_len;
}

void *gzip_load(const char *fname, int *ptr_ulen)
{
  int fd = 0;
  gzFile f = 0;
  int len, ulen = 0;
  void * uncompr = 0;

  SDDEBUG("%s(%s)\n", __FUNCTION__, fname);
  SDINDENT;

  fd = fs_open(fname, O_RDONLY);
  if (!fd) {
    goto error;
  }
  len = fs_total(fd);
  ulen = is_gz(fd, len);
  if (ulen < 0) {
    /* Not a gzip file : get total file size. */
    ulen = len;
  }

  f = gzdopen(fd, "rb");
  if (!f) {
    SDERROR("gzopen failed\n");
    goto error;
  }
  fd = 0; /* $$$ Closed by gzclose(). Verify fdopen() rules. */

  uncompr = calloc(1, ulen);
  if (!uncompr) {
    SDERROR("malloc error\n");
    goto error;
  }
  len = gzread(f, uncompr, ulen);
  if (len != ulen) {
    int err;
    SDERROR("gzread error: %s\n", gzerror(f, &err));
    goto error;
  }
  goto end;
  
 error:
  if (uncompr) {
    free(uncompr);
    uncompr = 0;
    ulen = 0;
  }

 end:
  if (fd) {
    fs_close(fd);
  }
  if (f) {
    gzclose(f);
  }
  if (ptr_ulen) {
    *ptr_ulen = ulen;
  }
      
  if (!uncompr) {
    SDERROR("Failed\n");
  } else {
    SDERROR("Success\n");
  }
  SDUNINDENT;
  return uncompr;
}
