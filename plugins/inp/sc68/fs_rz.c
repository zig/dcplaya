/**
 * @file    fs_rz.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   Gzipped compressed rom disk for KOS file system
 * 
 * $Id: fs_rz.c,v 1.5 2004-07-16 07:35:21 vincentp Exp $
 */

#include <arch/types.h>
#include <kos/thread.h>
#include <arch/spinlock.h>
#include <kos/fs.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "zlib.h"

#include "dcplaya/config.h"
#include "sysdebug.h"
#include "fs_rz.h"

/** RAM disk i-node */
typedef struct {
  const char *name;
  const uint8 *cdata;
  uint8 * udata;
  int   ulen;
  int   clen;
  int   uptr;
  int   cptr;
} rdfh_t;

typedef struct {
  int cnt;
  const uint8 *e;
  dirent_t dir;
} rddh_t;

static rddh_t mydh, * cur_dh = 0;

static const uint8 * root = 0;
static rdfh_t myrd, * cur_rd = 0;
static int root_entries;
static uint8 * cached_addr;
static const uint8 * cached_shadow;

/* Used to compare 2 names. We could easily change case sensitivity */
static int namecmp(const char *n1, const char *n2)
{
  return strcmp(n1,n2);
}

static int do_inflate(Byte * uncompr, int uncomprLen,
		      const Byte * compr, int comprLen)
{
  int err;
  z_stream d_stream;

  if (!comprLen || !uncomprLen) {
    return 0;
  }

  d_stream.zalloc = (alloc_func)0;
  d_stream.zfree = (free_func)0;
  d_stream.opaque = (voidpf)0;

  d_stream.next_in  = (Byte *)compr;
  d_stream.avail_in = 0;
  d_stream.next_out = uncompr;
  d_stream.avail_out = 0;

  err = inflateInit(&d_stream);
  if (err != Z_OK) {
    SDERROR("[rz] : inflate init failed := [%d]\n",err);
    return -1;
  }

  d_stream.next_in  = (Byte *)compr;
  d_stream.avail_in = comprLen;
  d_stream.next_out = uncompr;
  d_stream.avail_out = uncomprLen;

  err = inflate(&d_stream, Z_FINISH);

/*   SDDEBUG("[rz] : inflate code : %d\n", err); */
/*   SDDEBUG("[rz] : total  in    : %d\n", d_stream.total_in); */
/*   SDDEBUG("[rz] : expected in  : %d\n", comprLen); */
/*   SDDEBUG("[rz] : avail in     : %d\n", d_stream.avail_in); */
/*   SDDEBUG("[rz] : total  out   : %d\n", d_stream.total_out); */
/*   SDDEBUG("[rz] : expected out : %d\n", uncomprLen); */
/*   SDDEBUG("[rz] : avail out    : %d\n", d_stream.avail_out); */

  if (err == Z_STREAM_END) {
    err = 0;
/*     SDDEBUG("[rz] : inflate stream end\n"); */
    if (d_stream.total_out != uncomprLen) {
      SDERROR("[rz] : inflate invalid out total [%d != %d]\n",
	      d_stream.total_out, uncomprLen);
      err = -1;
    }
    if (d_stream.total_in != comprLen) {
      SDERROR("[rz] : inflate invalid in total [%d != %d]\n",
	      d_stream.total_in, uncomprLen);
      err = -1;
    }
  } else {
    err = -1;
  }
  inflateEnd(&d_stream);

  return err;
}

static ssize_t read(file_t fd, void * buffer, size_t size)
{
  rdfh_t * rd = (rdfh_t *)fd;
  int err;

  if (!size) {
    return 0;
  }
  if (!buffer) {
    return -1;
  }

  if (!rd || rd != cur_rd) {
    return -1;
  }

  if (!rd->udata) {
    if (rd->cdata == cached_shadow) {
/*       SDDEBUG("[rz] : Using cached buffer @%p shadow:%p]\n", */
/* 	      cached_addr,cached_shadow); */
      rd->udata = cached_addr;
    } else {
      rd->udata = malloc(rd->ulen);
      if (!rd->udata) {
	SDERROR("[rz] : malloc error (%d bytes)\n", rd->ulen);
	return -1;
      }
      if (cached_addr) {
/* 	SDDEBUG("[rz] : Free old cached buffer @%p shadow:%p]\n", */
/* 		cached_addr,cached_shadow); */
	free (cached_addr);
      }
      cached_addr = rd->udata;
      cached_shadow = rd->cdata;
/*       SDDEBUG("[rz] : New cached buffer @%p shadow:%p]\n", */
/* 	      cached_addr,cached_shadow); */
      err = do_inflate(rd->udata, rd->ulen, rd->cdata, rd->clen);
      if (err) {
	free(rd->udata);
	if (rd->udata == cached_addr) {
	  cached_addr = 0;
	  cached_shadow = 0;
	}
	rd->udata = 0;
	return -1;
      }
    }
  }

  err = rd->ulen - rd->uptr;
  if (err > size) {
    err = size;
  }
  memcpy(buffer, rd->udata + rd->uptr, err);
  rd->uptr += err;

/*   SDDEBUG("[rz] : read [%p,%p,%d,%d]\n", rd, buffer,size,err); */
  return err;
}


static dirent_t * readdir(file_t fd)
{
  rddh_t *dh = (rddh_t *)fd;
  int nlen,clen,ulen;
  const char * name;

  if (!dh || dh != cur_dh) {
    return 0;
  }

  if (dh->cnt >= root_entries) {
    return 0;
  }

  nlen = dh->e[0];
  ulen = dh->e[1] + (dh->e[2]<<8) + (dh->e[3]<<16);
  clen = dh->e[4] + (dh->e[5]<<8) + (dh->e[6]<<16);
  name = dh->e + 7;
  dh->e += 7 + nlen + clen;
  ++dh->cnt;

  /* Fill dir */
  strcpy(dh->dir.name, name);
  dh->dir.size = ulen;
  dh->dir.attr = 0;

  return &dh->dir;
}


/* Open a file or directory */
static file_t open(vfs_handler_t * vfs, const char *fn, int mode)
{
  const char * name;

  if (!fn) {
    SDERROR("[rz] : invalid filename [%s]\n",fn);
    return 0;
  }

  if (!fn[0] || (fn[0] == '.' && !fn[1])) {
    if (mode != (O_RDONLY | O_DIR)) {
      SDERROR("[rz] : invalid open mode for root\n");
      return 0;
    }
    if (cur_dh) {
      SDERROR("[rz] : too many open dir\n");
      return 0;
    }
    cur_dh = (rddh_t *)1;
    mydh.cnt = 0;
    mydh.e   = root + 8;
    memset(&mydh.dir,0,sizeof(mydh.dir));
    return (uint32) (cur_dh = &mydh);
  }

  name = strrchr(fn,'/');
  name = name ? name+1 : fn;

  if (cur_rd) {
    SDERROR("[rz] : too many open file\n");
    return 0;
  }

  // Alloc fh
  cur_rd = (rdfh_t *)1;

  /* Checking open mode. */
  if (mode != O_RDONLY) {
    SDERROR("[rz] : invalid open mode\n");
    cur_rd = 0;
    return 0;
  }

  {
    int i;
    const uint8 * entry;
    entry = root + 8;

/*     SDDEBUG("[rz] : scanning %d entries for [%s]\n",root_entries, name); */
    for (i=0; i<root_entries; ++i) {
      int namelen,ulen,clen;

      namelen = *entry++;
      ulen = entry[0] + (entry[1]<<8) + (entry[2]<<16);
      clen = entry[3] + (entry[4]<<8) + (entry[5]<<16);
      entry += 6;
      //SDDEBUG("[rz] : scan file [%s(%d) %d %d]\n",entry,namelen,ulen,clen);
      if (!namecmp(name,entry)) {
	myrd.name = entry;
	myrd.cdata = entry + namelen;
	myrd.udata = 0;
	myrd.ulen = ulen;
	myrd.clen = clen;
	myrd.uptr = 0;
	myrd.cptr = 0;
	break;
      }
      entry += namelen + clen;
    }
    if (i >= root_entries) {
      SDERROR("[rz] : file not found [%s]",fn);
      cur_rd = 0;
      return 0;
    }
  }
   
  return (uint32) (cur_rd = &myrd);
}

/* Close a file or directory */
static void close(uint32 fd)
{
  rdfh_t * rd;
  rddh_t * dh;

  if (fd) {
    rd = (rdfh_t *)fd;
    dh = (rddh_t *)fd;

    if (rd == cur_rd) {
      /* do not free : keep cached data !!! */
      memset(rd,0,sizeof(*rd));
      cur_rd = 0;
    } else if (dh == cur_dh) {
      memset(dh,0,sizeof(*dh));
      cur_dh = 0;
    }
  }
}

/* Seek elsewhere in a file */
static off_t seek(uint32 fd, off_t offset, int whence)
{
  rdfh_t * rd = (rdfh_t *)fd;

  if (!rd || rd != cur_rd) {
    return -1;
  }

  switch(whence) {
  case SEEK_SET:
    break;
  case SEEK_CUR:
    offset += rd->uptr;
    break;
  case SEEK_END:
    offset = rd->ulen - offset;
    break;
  default:
    SDERROR("[rz] : invalid seek whence\n");
    return -1;
  }

  if (offset < 0 || offset > rd->ulen) {
    SDERROR("[rz] : invalid seek address\n");
    return -1;
  }
  rd->uptr = offset;
  return rd->uptr;
}

/* Tell where in the file we are */
static off_t tell(uint32 fd)
{
  rdfh_t * rd = (rdfh_t *)fd;

  if (rd && rd == cur_rd) {
    return rd->uptr;
  }
  return -1;
}

/* Tell how big the file is */
static size_t total(uint32 fd)
{
  rdfh_t * rd = (rdfh_t *)fd;

  if (rd && rd == cur_rd) {
    return rd->ulen;
  }
  return -1;
}

/* Put everything together */
static vfs_handler_t vh = {
  {
    "/rz",              /* name */
    0, 
    0x00010000,		/* Version 1.0 */
    0,			/* flags */
    NMMGR_TYPE_VFS,	/* VFS handler */
    NMMGR_LIST_INIT	/* list */
  },
  0, 0,		        /* In-kernel, no cacheing */
  open,
  close,
  read,
  0,    //write
  seek, //only for total !!
  tell,
  total,
  readdir,
  0,
  0,
  0,
  0
};

/* Initialize the file system */
int fs_rz_init(const unsigned char * romdisk)
{
  SDDEBUG("[%s] [%p]\n", __FUNCTION__, romdisk);

  /* Could not create 2 ramdisks. */
  if (root) {
    SDERROR("[%s] : only one ROM disk is available.\n", __FUNCTION__);
    return -1;
  }
  if (!romdisk) {
    SDERROR("[%s] : bad pointer.\n", __FUNCTION__);
    return -1;
  }

  if (memcmp(romdisk,"RD-BEN",6)) {
    SDERROR("[%s] : invalid romdisk.\n", __FUNCTION__);
    return -1;
  }
      
  root_entries = * (uint16 *) (romdisk + 6);
  SDDEBUG("[%s] : %d entries\n",__FUNCTION__, root_entries);

  /* Register with VFS */
  if (nmmgr_handler_add(&vh)) {
    SDERROR("-->fs_handler_add failed\n");
    return -1;
  }

  root = romdisk;
  cur_rd = 0;
  cached_addr = 0;
  cached_shadow = 0;
  memset(&myrd,0,sizeof(myrd));
  memset(&mydh,0,sizeof(mydh));
  cur_dh = 0;

  return 0;
}

/* De-init the file system */
int fs_rz_shutdown(void)
{
  SDDEBUG("%s\n", __FUNCTION__);
  root = 0;
  if (cached_addr) {
    free(cached_addr);
    cached_addr = 0;
  }
  cur_rd = 0;
  cached_addr = 0;
  cached_shadow = 0;
  memset(&myrd,0,sizeof(myrd));
  memset(&mydh,0,sizeof(mydh));
  cur_dh = 0;

  return nmmgr_handler_remove(&vh);
}
