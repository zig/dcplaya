/**
 * @file      dcar.c
 * @author    benjamin gerard <ben@sashipa.com>
 * @date      2002/09/21
 * @brief     dcplaya archive.
 *
 * $Id: dcar.c,v 1.7 2003-03-07 21:06:03 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysdebug.h"
#include "dcar.h"
#include "filetype.h"
#include "file_utils.h"
#include "zlib.h"

const int dcar_align = 4;

dcar_filter_e dcar_default_filter(const dirent_t *de, int level)
{
  dcar_filter_e code = DCAR_FILTER_ERROR;
  level = level;
  if (de) {
    int type = filetype_get(de->name, de->size);
    code = (type >= filetype_dir) ? DCAR_FILTER_ACCEPT : DCAR_FILTER_REJECT;
  }
  return code;
}

void dcar_default_option(dcar_option_t * opt)
{
  memset(opt, 0, sizeof(*opt));
  opt->in.filter   = dcar_default_filter;
  opt->in.compress = -1;
}

static int ftotal(const char *fname)
{
  return fu_size(fname);
}

static char * mk_pathend(const dcar_option_t * opt)
{
  char * pathend
    = opt->internal.path + strlen(opt->internal.path);
  if (pathend > opt->internal.path && pathend[-1] != '/') {
    *pathend++ = '/';
    *pathend = 0;
  }
  return pathend;
}

/* Alloc the global archive. */
static dcar_tree_t * tree_alloc(int n_entry, int n_bytes)
{
  dcar_tree_t * dt;
  int bytes;

  if (n_entry <= 0 || n_bytes < 0) {
    return 0;
  }

  bytes = 
    sizeof(*dt) + 
    (n_entry-1) * sizeof(dcar_tree_entry_t) +
    n_bytes;

  dt = (dcar_tree_t *)malloc(bytes);
  if (dt) {
    if (n_bytes) {
      dt->data = (char *) &dt->e[dt->n = n_entry];
    } else {
      memcpy(dt->magic,"DCAR", 4);
    }
    memset(dt->e, 0, n_entry * sizeof(dcar_tree_entry_t));
    dt->n = n_entry;
  }
  return dt;
}

/* Convert a directory entry to an archive entry. */
static dcar_filter_e de2te(dcar_tree_entry_t * te, const dirent_t * de)
{
  te->name[sizeof(te->name)-1] = 0;
  strncpy(te->name, de->name, sizeof(te->name));
  if (te->name[sizeof(te->name)-1]) {
    /* File name too long ! */
    return DCAR_FILTER_ERROR;
  }
  te->attr.end = 0;
  if (de->size == -1) {
    te->attr.dir  = 1;
    te->attr.size = 0;
  } else {
    te->attr.dir  = 0;
    te->attr.size = de->size;
    if (te->attr.size != de->size) {
      return DCAR_FILTER_ERROR;
    }
  }
  return DCAR_FILTER_ACCEPT;
}

/* Reserve a linked-list  */
static void * reverse(void * t2)
{
  struct _any_list_t {
    struct _any_list_t * nxt;
  } * s, *p, *t;

  t = (struct _any_list_t *)t2;

  for (s=p=0; t; p=t, t=t->nxt) {
    if (p) {
      p->nxt = s;
    }
    s = p;
  }
  if (p) {
    p->nxt = s;
  }
  return p;
}

/*
 * s | . . A B C 
 * p | . A B C D
 * t | A B C D .
 * n | . A B C 
 */

/** Add entry. */
typedef struct _aentry_s {
  struct _aentry_s *nxt;
  struct _aentry_s *son;
  dcar_tree_entry_t te;
} aentry_t;

static int free_aentries(aentry_t *e)
{
  int count = 0;

  if (e) {
    count += free_aentries(e->son);
    count += free_aentries(e->nxt);
    free(e);
    ++count;
  }
  return count;
}

static int make_dir_index(aentry_t * aentry, int base)
{
  aentry_t *e;

  if (!aentry) {
    // Empty sub directory special case
    return 0;
  }

  /* Count entries in this level */
  //  SDDEBUG("-------\n");
  for (e=aentry; e; e=e->nxt, ++base) {
    //    SDDEBUG("#%03d [%s%s]\n", base, e->te.name, e->te.attr.dir ? "/":"");
  }
  
  /* Apply index to this level */
  for (e=aentry; e; e=e->nxt) {
    if (e->te.attr.dir) {
      int newbase;
      newbase = make_dir_index(e->son, base);
      if (newbase) {
	e->te.attr.size = base;
	base = newbase;
      } else {
	e->te.attr.size = 0;
      }
    }
  }
  return base;
}

static int r_gzwrite_aentries(gzFile zf, aentry_t * aentry)
{
  aentry_t *e;

  if (!aentry) {
    return 0;
  }

  /* 1st : This level */
  for (e=aentry; e; e=e->nxt) {
    if (gzwrite(zf, &e->te, sizeof(e->te)) <= 0) {
      /*       SDERROR("Writing directory entry [%s]\n", e->te.name); */
      return -1;
    }
  }

  /* 2nd : Recurse sub-dir */
  for (e=aentry; e; e=e->nxt) {
    if (e->te.attr.dir) {
      if ( r_gzwrite_aentries(zf, e->son) < 0) {
	return -1;
      }
    }
  }

  return 0;
}

/*
  static unsigned int CRC(unsigned int crc, char *b, int n)
  {
  while (n--) {
  crc += *(unsigned char *)b++;
  }
  return crc;
  }
*/

/* Copy `opt->internal.path' into gzfile. Increments opt->out.bytes
 *
 * @retval >=0    Number of bytes copied (including pading)
 * @retval -1     Open error
 * @retval -2     Read error
 * @retval -3     Write error
 * @retval other  Internal unexecpected errors
 */
static int gzcopy(gzFile zf, dcar_option_t * opt)
{
  int fd, n, r, w, len, z;

  fd = fs_open(opt->internal.path, O_RDONLY);
  if (!fd) {
    n = -1;
    goto error;
  }
  len = fs_total(fd);
  if (len < 0) {
    len = -1;
    /*     SDWARNING("[%s] : Could not get total, trying to copy anyway\n", */
    /* 			  opt->internal.path); */
  }

  r = w = 0;
  do {
    n = fs_read(fd, opt->internal.tmp, opt->internal.max);
    if (n < 0) {
      opt->errstr = "read error";
      n = -2;
      goto error;
    } else if (n > 0) {
      r += n;
      if (gzwrite(zf, opt->internal.tmp, n) != n) {
	opt->errstr = "gzip write error";
	n = -3;
	goto error;
      }
      w += n;
      opt->out.bytes += n;
    }
  } while (n>0);

  /* Check ... */
  if (r != w || (len != -1 && r != len)) {
    n = -4;
    opt->errstr = "check error";
    goto error;
  }

  n = r;
  len = (len + dcar_align - 1) & ~(dcar_align - 1);
  z = len - n;

  // Zero pad for alignment requirement.
  if (z >= dcar_align) {
    n = -5;
    opt->errstr = "alignment error";
    goto error;
  }

  if (z) {
    memset(opt->internal.tmp, 0, z);
    if (gzwrite(zf, opt->internal.tmp, z) != z) {
      opt->errstr = "gzip padding write error";
      n = -3;
      goto error;
    }
    n += z;
  }

 error:
  if (fd) {
    fs_close(fd);
  }
  
  return n;
}

static int r_gzwrite_data(gzFile zf, aentry_t * aentry, dcar_option_t * opt)
{
  aentry_t *e;
  char * pathend;

  if (!aentry) {
    return 0;
  }

  /* make path */
  pathend = mk_pathend(opt);

  /* 1st : This level */
  for (e=aentry; e; e=e->nxt) {
    if (opt->in.verbose) {
      printf("%s%s\n", opt->internal.path, e->te.name);
    }

    if (!e->te.attr.dir && e->te.attr.size) {
      int n;

      strcpy(pathend, e->te.name);
      n = gzcopy(zf, opt);
      *pathend = 0;
      if (n < 0) {
	switch(n) {
	case -1:
	  /* 		  SDERROR("[%s] : open error.\n", e->te.name); */
	  return -1;
	case -2:
	  /* 		  SDERROR("[%s] : read error.\n", e->te.name); */
	  return -1;
	case -3:
	  /* 		  SDERROR("Write error. Current out bytes [%d]\n", opt->out.bytes); */
	  return -1;
	default:
	  /* 		  SDERROR("[%s] : unexpected  error.\n", e->te.name); */
	  return -1;
	}
      }
    }
  }

  /* 2nd : Recurse sub-dir */
  for (e=aentry; e; e=e->nxt) {
    if (e->te.attr.dir) {
      int n;

      strcpy(pathend, e->te.name);
      n = r_gzwrite_data(zf, e->son, opt);
      *pathend = 0;
      if (n < 0) {
	return -1;
      }
    }
  }

  return 0;
}

/* Count number bytes including alignment */
static int count_bytes(aentry_t * aentry)
{
  aentry_t *t;
  int bytes = 0;

  for (t=aentry; t; t=t->nxt) {
    if (!t->te.attr.dir) {
      bytes += (t->te.attr.size + dcar_align - 1) & -dcar_align;
    } else {
      bytes += count_bytes(t->son);
    }
  }
  return bytes;
}

static void dump(aentry_t * aentry)
{
  aentry_t *e;

  if (!aentry) {
    SDDEBUG("<null>\n");
    return;
  }

  /* Count entries in this level */
  for (e=aentry; e; e=e->nxt) {
    SDDEBUG("[%s]%s%s [%d]\n",
	    e->te.name,
	    e->te.attr.dir ? "*":"",
	    e->te.attr.end ? ".":"",
	    e->te.attr.size);
    if (e->son && !e->te.attr.dir) {
      SDCRITICAL("Missing dir flag\n");
    }
    if ((e->nxt==0) != e->te.attr.end) {
      SDCRITICAL("Bad end flag\n");
    }

    if (e->te.attr.dir) {
      SDINDENT;
      dump(e->son);
      SDUNINDENT;
    }
  }
}

static int r_add(dcar_option_t *opt, aentry_t * parent)
{
  aentry_t * aentry = 0, * t;
  int fd;
  int count = 0;
  dirent_t *de;
  char *pathend;

  fd = fs_open(opt->internal.path, O_RDONLY|O_DIR);
  if (!fd) {
    opt->errstr = "open error";
    count = -1;
    goto error;
  }

  if (opt->out.level < opt->internal.level) {
    opt->out.level = opt->internal.level;
  }
  
  while (de = fs_readdir(fd), de) {

    /* Read nxt entry */
    switch (opt->in.filter(de, opt->internal.level)) {
    case DCAR_FILTER_ACCEPT: {
      aentry_t * aentry_here = calloc(1, sizeof(aentry_t));
      
      if (!aentry_here) {
	opt->errstr = "aentry_t malloc error";
	count = -1;
	goto error;
      }
      aentry_here->nxt = aentry;
      if (!aentry) {
	parent->son = aentry_here;
      }
      aentry = aentry_here;
      
      de2te(&aentry->te, de);
      ++count;
    } break;

    case DCAR_FILTER_REJECT:
      break;

    case DCAR_FILTER_BREAK:
      de = 0;
      break;

    case DCAR_FILTER_ERROR:
    default:
      opt->errstr = "filter error";
      count = -1;
      goto error;
    }

    if (!de) {
      break;
    }
  }
  if (aentry) {
    aentry->te.attr.end = 1;
  }
    
  fs_close(fd);
  fd = 0;

  pathend = mk_pathend(opt);

  for (t=reverse(aentry); t; t=t->nxt) {
    if (opt->in.verbose) {
      printf("%s%s\n", opt->internal.path,t->te.name);
    }
    ++opt->out.entries;
    opt->out.bytes += t->te.attr.size;

    if (t->te.attr.dir) {
      int more;
      strcpy(pathend, t->te.name);
      ++opt->internal.level;
      more = r_add(opt, t);
      --opt->internal.level;
      *pathend=0;
      if (more < 0) {
	count = more;
	break;
      } else {
	count += more;
      }
    }
  }
  
 error:
  if (fd) {
    fs_close(fd);
  }
  return count;
}

static dcar_option_t * setup_option(dcar_option_t *opt, dcar_option_t *opt2)
{
  if (!opt) {
    dcar_default_option(opt = opt2);
  } else {
    memset(&opt->out, 0, (char*)&opt[1] - (char*)&opt->out);
  }
  if (!opt->in.filter) {
    opt->in.filter = dcar_default_filter;
  }
  return opt;
}

int dcar_archive(const char *name, const char *path, dcar_option_t *opt)
{
  aentry_t root;
  char mode [8];
  char cpath[512];
  char tmp[512];
  int count;
  dcar_tree_t dt;
  int fd = 0, fd2;
  gzFile zf=0;
  dcar_option_t option;
  volatile int save_verbose;

  /* Setup option */
  opt = setup_option(opt, &option);
  opt->internal.tmp = tmp;
  opt->internal.max = sizeof(tmp);
  save_verbose = opt->in.verbose;

  if (!name || !path) {
    opt->errstr = "invalid parameter";
    return -1;
  }

  /* Make gzip open mode. */
  count = 0;
  mode [count++] = 'w';
  mode [count++] = 'b';
  if (opt->in.compress >= 0 && opt->in.compress <= 9) {
    mode [count++] = '0' + opt->in.compress;
  }
  mode [count] = 0;

  /* Build tree */
  strncpy(cpath, path, sizeof(cpath)-1);
  cpath[sizeof(cpath)-1] = 0;
  opt->internal.path = cpath;
  memset(&root, 0, sizeof(root));
  root.te.attr.dir = root.te.attr.end = 1;
  opt->in.verbose = 0;
  count = r_add(opt, &root);
  opt->in.verbose = save_verbose;
  opt->out.bytes = 0;
  if (count < 0) {
    goto error;
  }
  make_dir_index(root.son, 0);
  
  /* Write archive */
  // $$$ ben : Could unlink empty-dir !!!
  if (!opt->in.skip) {
    fs_unlink(name);
    fd = fs_open(name, O_WRONLY);
  } else {
    fd = fs_open(name, O_APPEND);
    opt->in.skip = fs_tell(fd);
    /* 	SDDEBUG("[%s] : skipping %d bytes\n",__FUNCTION__, opt->in.skip); */
  }
  fd2 = fd;
  if (!fd) {
    opt->errstr = "create error";
    count = -1;
    goto error;
  }

  zf = gzdopen(fd, mode);
  if (!zf) {
    opt->errstr = "gzip reopen error";
    count = -1;
    goto error;
  }
  fd = 0;

  memcpy(&dt.magic, "DCAR",4);
  dt.n = count;

  /* Write archive headers */
  if (gzwrite(zf, &dt, 8) <= 0) {
    opt->errstr = "gzip write archive header error";
    count = -1;
    goto error;
  }

  /* Write directory */
  if (r_gzwrite_aentries(zf, root.son) < 0) {
    opt->errstr = "gzip write dir entries error";
    count = -1;
    goto error;
  }

  /* Write file data */
  if (r_gzwrite_data(zf, root.son, opt) < 0) {
    count = -1;
    goto error;
  }
  gzflush(zf, Z_SYNC_FLUSH);
  opt->out.ubytes = gztell(zf);
  /* add checksum + filesize - skip at start */
  opt->out.cbytes = fs_tell(fd2) - opt->in.skip + 8;

 error:
  free_aentries(root.son);
  if (fd) {
    fs_close(fd);
  } else if (zf) {
    gzclose(zf);
    if (count < 0) {
      fs_unlink(name);
    }
  }

  return count;
}


int dcar_simulate(const char *path, dcar_option_t * opt)
{
  char cpath[1024];
  aentry_t root;
  dcar_option_t option;
  int count;

  /* Setup options. */
  opt = setup_option(opt, &option);

  if (!path) {
    opt->errstr = "invalid parameter";
    return -1;
  }
  strncpy(cpath, path, sizeof(cpath)-1);
  cpath[sizeof(cpath)-1] = 0;
  opt->internal.path = cpath;

  root.nxt = root.son = 0;
  count = r_add(opt, &root);
  free_aentries(root.son);

  return count;
}

int dcar_test(const char *name, dcar_option_t * opt)
{
  if (opt) {
    opt->errstr = "not implemented";
  }
  return -1;
}

static int exist_dir(const char *fname)
{
  return fu_is_dir(fname);
}

/* Create a directory if not exist */ 
static int create_dir(const char *fname)
{
  if (!exist_dir(fname)) {
    return fu_create_dir(fname);
  }
  return 0;
}

static int gzextract(int fd, gzFile zf, dcar_option_t * opt, int len)
{
  int n, r, w, rem;
  //  unsigned int crc = 0;

  if (len < 0) {
    opt->errstr = "extract invalid file size";
    return -1;
  }

  r = w = 0;
  rem = len;
  while (rem > 0) {
    n = rem;
    if (n > opt->internal.max) {
      n = opt->internal.max;
    }
    rem -= n;

    if (gzread(zf, opt->internal.tmp, n) != n) {
      opt->errstr = "extract gzip read error";
      return -1;
    }
    r += n;

    if (fs_write(fd, opt->internal.tmp, n) != n) {
      opt->errstr = "extract write error";
      return -1;
    }
    w += n;
    //    crc = CRC(crc,  opt->internal.tmp, n);

    opt->out.bytes += n;
  }

  if (rem != 0 || r != w || w != len) {
    opt->errstr = "extract check error";
    n = -1;
  } else {
    int z;

    n = r;
    len = (len + dcar_align - 1) & ~(dcar_align - 1);
    z = len - n;

    // Zero pad for alignment requirement.
    if (z >= dcar_align) {
      opt->errstr = "extract alignment error";
      return -1;
    }
    if (z) {
      if (gzread(zf, opt->internal.tmp, z) != z) {
	opt->errstr = "extract gzip padding read error";
	return -1;
      }
    }
  }

  return n;
}


static int extract_file(gzFile zf, dcar_option_t * opt,
			int bytes)
{
  int fd;
  char * fname = opt->internal.path;

  if (bytes < 0) {
    return -1;
  }
  
  if (exist_dir(fname)) {
    opt->errstr = "extract file exists as a directory";
    return -1;
  }
  fs_unlink(fname);

  fd = fs_open(fname, O_WRONLY);
  if (!fd) {
    opt->errstr = "extract create error";
    return -1;
  }
  bytes = gzextract(fd, zf, opt, bytes);
  fs_close(fd);

  return bytes;
}

static int r_extract_dtree(gzFile zf, dcar_tree_t * dt, dcar_option_t * opt,
			   int n)
{
  char * pathend;
  int count;
  dcar_tree_entry_t *e, *save_e;

  if ((unsigned int)n >= (unsigned int)dt->n) {
    opt->errstr = "extract entry out of range";
    return -1;
  }

  pathend = mk_pathend(opt);

  /* Process files */
  for (count=0, save_e = e = dt->e+n; n < dt->n;  ++n, ++e) {
    strcpy(pathend, e->name);
    if (opt->in.verbose) {
      printf("%s\n", opt->internal.path);
    }
    ++count;
    
    if (e->attr.dir) {
      /* Create directory */
      if (create_dir(opt->internal.path) < 0) {
	opt->errstr = "extract directory creation error";
	goto error;
      }
    } else {
      /* Extract regular file */
      if (extract_file(zf, opt, e->attr.size) < 0) {
	goto error;
      }
    }

    if (e->attr.end) {
      *pathend = 0;
      break;
    }
  }

  if (n >= dt->n) {
    opt->errstr = "extract check entries error";
    goto error;
  }

  /* Process subdir */
  for (e = save_e; 1; ++e) {
    int cnt;

    if (e->attr.dir) {
      /* Extract sub-directory */

      strcpy(pathend, e->name);
      opt->internal.level++;
      if (opt->internal.level > opt->out.level) {
	opt->out.level = opt->internal.level;
      }
      cnt = 0;
      if (e->attr.size) {
	if (cnt = r_extract_dtree(zf, dt, opt, e->attr.size), cnt < 0) {
	  goto error;
	}
      }
      opt->internal.level--;
      count += cnt;
    }

    if (e->attr.end) {
      *pathend = 0;
      return count;
    }
  }

 error:
  *pathend = 0;
  opt->errstr = "reach end of tree without encounter end flag";
  return -1;
}

static int extract_dtree(gzFile zf, dcar_tree_t * dt, dcar_option_t * opt)
{
  int count = -1;

  /*   SDDEBUG("%s('%s')\n", __FUNCTION__, opt->internal.path); */
  /*   SDINDENT; */

  /* Create path extract path. */
  if (opt->in.verbose) {
    printf("%s\n", opt->internal.path);
  }
  if (create_dir(opt->internal.path) < 0) {
    opt->errstr = "extract directory creation error";
    goto error;
  }
  count = r_extract_dtree(zf, dt, opt, 0);

 error:
  /*   SDUNINDENT; */
  /*   SDDEBUG("%s('%s') := [%d]\n", __FUNCTION__, opt->internal.path, count); */
  return count;
}


int dcar_extract(const char *name, const char *path, dcar_option_t *opt)
{
  char cpath[1024];
  char tmp[512];
  dcar_option_t option;
  dcar_tree_t * dt = 0;
  int fd, n;
  int err = -1;
  gzFile zf = 0;

  /*   SDDEBUG("%s('%s','%s',%p)\n", __FUNCTION__, name, path, opt); */
  /*   SDINDENT; */

  if (!name || !path)  {
    return -1;
  }
  opt = setup_option(opt, &option);
  opt->internal.tmp = tmp;
  opt->internal.max = sizeof(tmp);

  cpath[sizeof(cpath)-1] = 0;
  strncpy(cpath, path, sizeof(cpath)-1);
  if (cpath[sizeof(cpath)-1]) {
    opt->errstr = "path too long";
    return -1;
  }
  cpath[sizeof(cpath)-1] = 0;
  opt->internal.path = cpath;

  fd = fs_open(name, O_RDONLY);
  if (!fd) {
    opt->errstr = "open error";
    goto error;
  }
  if (opt->in.skip && fs_seek(fd, opt->in.skip, SEEK_SET) != opt->in.skip) {
    opt->errstr = "seek error";
    goto error;
  }
  opt->out.cbytes = fs_total(fd) - opt->in.skip;

  zf = gzdopen(fd, "rb");
  if (!zf) {
    opt->errstr = "reopen error";
    goto error;
  }
  fd = 0;

  if (gzread(zf, tmp, 8) != 8) {
    opt->errstr = "header read error";
    goto error;
  }

  if (memcmp(tmp, "DCAR", 4)) {
    opt->errstr = "invalid archive header";
    goto error;
  }

  dt = tree_alloc(*(int*)&tmp[4], 0);
  if (!dt) {
    opt->errstr = "tree alloc error";
    goto error;
  }

  /* Read directory entries */
  n = dt->n * sizeof(*dt->e);
  if (gzread(zf, dt->e, n) != n) {
    opt->errstr = "dir entries read error";
    goto error;
  }
  opt->out.entries = dt->n;

  n = extract_dtree(zf, dt, opt);
  opt->out.ubytes = gztell(zf);

  if (n != dt->n) {
    if (n >= 0) {
      opt->errstr = "bad number of entry";
    } else {
      /*       SDERROR("[%s] : extract failure [%d].\n", name, n); */
    }
    err = -1;
  } else {
    err = n;
  }

 error:
  if (err < 0) {
    if (!opt->errstr) {
      opt->errstr = "unexpected error";
    }
    SDERROR("[%s] : %s.\n", __FUNCTION__, opt->errstr);
  } else {
    opt->errstr = "success";
  }

  if (dt) {
    free(dt);
  }
  if (fd) {
    fs_close(fd);
  }
  if (zf) {
    gzclose(zf);
  }

  /*   SDUNINDENT; */
  /*   SDDEBUG("%s('%s','%s') := [%d,%s]\n", __FUNCTION__, */
  /* 		  name, path, err, opt->errstr); */

  return err;
}
