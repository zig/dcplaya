/**
 * @file      dcar.c
 * @author    benjamin gerard <ben@sashipa.com>
 * @date      2002/09/21
 * @brief     dcplaya archive.
 *
 * $Id: dcar.c,v 1.1 2002-09-23 03:23:24 benjihan Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sysdebug.h"
#include "dcar.h"
#include "filetype.h"
#include "zlib.h"

const int dcar_align = 4;

dcar_filter_e dcar_default_filter(const dirent_t *de, int level)
{
  dcar_filter_e code = DCAR_FILTER_ERROR;

  level = level;
  if (de) {
    int type = filetype_get(de->name, de->size);

    code = (type >= FILETYPE_DIR) ? DCAR_FILTER_ACCEPT : DCAR_FILTER_REJECT;
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
  int fd, n;
  fd = fs_open(fname, O_RDONLY);
  if (!fd) {
    return -1;
  }
  n = fs_total(fd);
  fs_close(fd);
  return n;
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
    return base;
  }

  /* Count entries in this level */
  //  SDDEBUG("-------\n");
  for (e=aentry; e; e=e->nxt, ++base) {
    //    SDDEBUG("#%03d [%s%s]\n", base, e->te.name, e->te.attr.dir ? "/":"");
  }
  
  /* Apply index to this level */
  for (e=aentry; e; e=e->nxt) {
    if (e->te.attr.dir) {
      e->te.attr.size = base;
      base = make_dir_index(e->son, base);
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
      SDERROR("Writing directory entry [%s]\n", e->te.name);
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

static int gzcopy(gzFile zf, dcar_option_t * opt)
{
  int fd, n, r, w;

  fd = fs_open(opt->internal.path, O_RDONLY);
  if (!fd) {
    return -1;
  }

  r = w = 0;
  do {
    n = fs_read(fd, opt->internal.tmp, opt->internal.max);
    if (n <= 0) {
      break;
    }
    r += n;

    n = gzwrite(zf, opt->internal.tmp, n);
    if (n <= 0) {
      n = -1;
      break;
    }
    w += n;
  } while (1);

  if (n != 0 || r != w) {
    n = -1;
  } else {
    n = r;
  }

  fs_close(fd);
  
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
  pathend = opt->internal.path+strlen(opt->internal.path);
  if (pathend > opt->internal.path && pathend[-1] != '/') {
    *pathend++ = '/';
    *pathend = 0;
  }

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
      if (n <= 0) {
	SDERROR("Writing file data [%s]\n", e->te.name);
	return -1;
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
    //    SDDEBUG("count:[%s] : %d\n", t->te.name, t->te.attr.size);
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

static int r_add(char * path, dcar_option_t *opt, aentry_t * parent)
{
  aentry_t * aentry = 0, * t;
  int fd;
  int count = 0;
  dirent_t *de;
  char *pathend;

  fd = fs_open(path, O_RDONLY|O_DIR);
  if (!fd) {
    SDERROR("Open error [%s]\n", path);
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
	SDERROR("aentry_t malloc failed.\n");
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
      SDERROR("Filter error\n");
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

  pathend = path+strlen(path);
  if (pathend > path && pathend[-1] != '/') {
    *pathend++ = '/';
    *pathend = 0;
  }
  for (t=reverse(aentry); t; t=t->nxt) {
    if (opt->in.verbose) {
      printf("%s%s\n", path,t->te.name);
    }
    ++opt->out.entries;
    opt->out.bytes += t->te.attr.size;

    if (t->te.attr.dir) {
      int more;
      strcpy(pathend, t->te.name);
      ++opt->internal.level;
      more = r_add(path, opt, t);
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
  gzFile zf=0;
  dcar_option_t option;
  volatile int save_verbose;

  /* Setup option */
  opt = setup_option(opt, &option);
  opt->internal.tmp = tmp;
  opt->internal.max = sizeof(tmp);
  save_verbose = opt->in.verbose;

  if (!name || !path) {
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
  count = r_add(cpath, opt, &root);
  opt->in.verbose = save_verbose;
  if (count < 0) {
    goto error;
  }
  make_dir_index(root.son, 0);
  
  /* Write archive */
  fs_unlink(name);
  zf = gzopen(name, mode);
  if (!zf) {
    goto error;
  }
  memcpy(&dt.magic, "DCAR",4);
  dt.n = count;

  /* Write archive headers */
  if (gzwrite(zf, &dt, 8) <= 0) {
    SDERROR("Writing archive header.\n");
    goto error;
  }

  /* Write directory */
  if (r_gzwrite_aentries(zf, root.son) < 0) {
    SDERROR("Writing archive directory entries.\n");
    goto error;
  }

  /* Write file data */
  if (r_gzwrite_data(zf, root.son, opt) < 0) {
    SDERROR("Writing archive file data.\n");
    goto error;
  }
  gzclose(zf);
  zf = 0;

  opt->out.ubytes  = gztell(zf);
  opt->out.cbytes  = ftotal(name);

 error:
  free_aentries(root.son);
  if (zf) {
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
    return -1;
  }
  strncpy(cpath, path, sizeof(cpath)-1);
  cpath[sizeof(cpath)-1] = 0;
  opt->internal.path = cpath;

  root.nxt = root.son = 0;
  count = r_add(cpath, opt, &root);
  free_aentries(root.son);

  return count;
}

int dcar_test(const char *name, dcar_option_t * opt)
{
  return -1;
}

int dcar_extract(const char *name, const char *path, dcar_option_t *opt)
{
  char header[8];
  dcar_option_t option;
  dcar_tree_t * dt = 0;
  int fd, n;
  gzFile zf = 0;

  opt = setup_option(opt, &option);

  fd = fs_open(name, O_RDONLY);
  if (!fd) {
    SDERROR("Open error.\n");
    goto error;
  }
  opt->out.cbytes = fs_total(fd);

  zf = gzdopen(fd, "rb");
  if (!zf) {
    SDERROR("Open error.\n");
    goto error;
  }
  fd = 0;

  if (gzread(zf, header, 8) != 8) {
    SDERROR("Header read error.\n");
    goto error;
  }

  if (memcmp(header, "DCAR", 4)) {
    SDERROR("Invalid archive header");
    goto error;
  }

  dt = tree_alloc(*(int*)&header[4], 0);
  if (!dt) {
    goto error;
  }

  /* Read directory entries */
  n = dt->n * sizeof(*dt->e);
  if (gzread(zf, dt->e, n) != n) {
    SDERROR("Dir entries read error.\n");
    goto error;
  }

  for (n=0; n<dt->n; ++n) {
    printf("#%3d [%s]%s%s : %d\n",
	   n,
	   dt->e[n].name,
	   dt->e[n].attr.dir ? "*":"",
	   dt->e[n].attr.end ? ".":"",
	   dt->e[n].attr.size);
    opt->out.bytes += dt->e[n].attr.dir ? 0 : dt->e[n].attr.size;
  }

  opt->out.entries = dt->n;

 error:
  if (dt) {
    free(dt);
  }
  if (fd) {
    fs_close(fd);
  }
  if (zf) {
    gzclose(zf);
  }

  return -1;
}

