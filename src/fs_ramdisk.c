/**
 * @file    fs_ramdisk.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   RAM disk for KOS file system
 * 
 * $Id: fs_ramdisk.c,v 1.10 2002-09-30 20:06:50 benjihan Exp $
 */

#ifdef VPSPECIAL
/* VP : added this to remove debug messages and test :) */
# undef DEBUG
# undef DEBUG_LOG
#endif

#include <arch/types.h>
#include <kos/thread.h>
#include <arch/spinlock.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "sysdebug.h"
#include "fs_ramdisk.h"

/* Allocation block size */
#define ALLOC_BLOCK_SIZE 1024

/* Opening mode bit. */
#define READ_MODE       1
#define WRITE_MODE 	2
#define DIR_MODE 	4

#define MAX_RD_FILES 	32            /**< Must not excess 32 */
#define INVALID_FH  	MAX_RD_FILES

/* Macro to work with node open field. */
#define UNLINKED        (1<<31)
#define IS_UNLINKED(V)  ((V)->open & UNLINKED)
#define OPEN_COUNT(V)   ((V)->open & ~UNLINKED)

/** RAM disk i-node */
typedef struct _node_s {
  struct _node_s   * big_bros;       /**< Previous node same level   */
  struct _node_s   * little_bros;    /**< Next node same level       */
  struct _node_s   * father;         /**< Previous level node        */
  struct _node_s   * son;            /**< Next level node (dir only) */

  dirent_t           entry;          /**< Kos directory entry.       */
  int                max;            /**< Data allocated size.       */
  uint8            * data;           /**< File data                  */
  int                open;           /**< Open count                 */
} node_t;

typedef struct
{
  node_t *node;	                    /**< Node for this file.         */
  int mode;                         /**< Opening mode mask.          */
  union {
    int pos;                        /**< Current position.           */
    struct _node_s * readdir;       /**< Current next readdir entry  */
  };
  int alloc_size;                   /**< Alloc policy (unused)       */
} openfile_t;


static node_t root_node;
static node_t * root = 0;       
static int fh_mask;       
static openfile_t fh[32];


/* File data alignment */
const int align = 16;

/* Mutex for file handles */
static spinlock_t fh_mutex;

/* Used to compare 2 names. We could easily change case sensitivity */
static int namecmp(const char *n1, const char *n2)
{
  return strcmp(n1,n2);
}

static const char * modestr(int mode)
{
  const char * str[12] =
    {
      "N/A", "R", "W", "R/W",
      "D", "D/R", "D/W", "D/R/W",
      "?/D", "?/D/R", "?/D/W", "?/D/R/W"
    };
  return str[(mode&7) + ((!!(mode&-8)) << 3)];
}

static const char * whatstr(int what)
{
  switch (what) {
  case 1:  return "file";
  case 2:  return "dir";
  case 3:  return "file/dir";
  default: return "nothing";
  }
}

static int valid_fh(uint32 fd)
{
  if (--fd >= MAX_RD_FILES || !(fh_mask & (1<<fd)) || !fh[fd].node>1) {
    SDERROR("[%d] : invalid file handle.\n", fd+1);
    return INVALID_FH;
  }
  return fd;
}

static int valid_file(uint32 fd, int mode)
{
  mode &= (READ_MODE|WRITE_MODE);
  if (fd = valid_fh(fd), fd != INVALID_FH) {
    if (!(fh[fd].mode & mode)) {
      SDERROR("[%d] : not an [%s] opened handle.\n", fd+1, modestr(mode));
      fd = INVALID_FH;
    }
  }
  return fd;
}

static int is_dir(node_t * node)
{
  return node && node->entry.size == -1;
}

static int valid_regular(uint32 fd, int mode)
{
  if (fd = valid_file(fd, mode), fd != INVALID_FH) {
    if (is_dir(fh[fd].node) || (fh[fd].mode & DIR_MODE)) {
      SDERROR("[%d] : not regular file handle.\n", fd+1);
      fd = INVALID_FH;
    }
  }
  return fd;
}

static int valid_dir(uint32 fd, int mode)
{
  if (fd = valid_file(fd, mode), fd != INVALID_FH) {
    if (!is_dir(fh[fd].node) || !(fh[fd].mode & DIR_MODE)) {
      SDERROR("[%d] : not directory handle.\n", fd+1);
      fd = INVALID_FH;
    }
  }
  return fd;
}

static int valid_name(const char * name, int accept_root) {
  node_t *node;
  int valid =
    ! (
       !name ||
       (!accept_root && !name[0]) ||
       (name[0] == '.' && !name[1]) ||
       (name[0] == '.' && name[1] == '.' && !name[2]) ||
       strlen(name) >= sizeof(node->entry.name)
       );
  if (!valid) {
    SDERROR("[%s] : invalid filename.\n", name ? name : "<null>");
  }
  return valid;
}

static node_t * youngest_bros(node_t * node)
{
  if (!node) {
    return 0;
  }
  for(;node->little_bros; node = node->little_bros)
    ;
  return node;
}

static void attach_node(node_t * father, node_t * node)
{
  /*
  SDDEBUG("%s [%s] to [%s]\n",__FUNCTION__,
	  node ? node->entry.name : "<null>",
	  father ? father->entry.name : "<null>");
  */
  node->father = father;
  node->little_bros = 0;
  if ((node->big_bros = youngest_bros(father->son)) != 0) {
    node->big_bros->little_bros = node;
  } else {
    father->son = node;
  }
}

static void detach_node(node_t * node)
{
  int i;

  //  SDDEBUG("%s [%s]\n",__FUNCTION__, node ? node->entry.name : "<null>");
  if (!node) {
    return;
  }
  /* Remove this node from directory reading. */
  for (i=0; i<MAX_RD_FILES; ++i) {
    if (fh[i].readdir == node) {
      fh[i].readdir = node->little_bros;
    }
  }
  if (node->father && node->father->son == node) {
      node->father->son = node->little_bros;
  }
  if (node->big_bros) {
    node->big_bros->little_bros = node->little_bros;
  }
  if (node->little_bros) {
    node->little_bros->big_bros = node->big_bros;
  }
}

static void release_node(node_t * node)
{
  //  SDDEBUG( "%s [%s]\n", __FUNCTION__, node ? node->entry.name : "<null>"); 
  if (node) {
    if (node->data) {
      free(node->data);
    }
    free(node);
  }
}

static void release_nodes(node_t * node)
{
  SDDEBUG("%s [%s]\n",__FUNCTION__ , node ? node->entry.name : "<null>"); 
  if (!node) {
    return;
  }
  SDINDENT;
  release_nodes(node->son);
  release_nodes(node->little_bros);
  release_node(node);
  SDUNINDENT;
}

static void dump_node(node_t * node)
{
  SDINDENT;
  if (node) {
    SDDEBUG("Name:   '%s'\n", node->entry.name);
    SDDEBUG("size:   %d\n", node->entry.size);
    SDDEBUG("alloc:  %d\n", node->max);
    SDDEBUG("open:   %d\n", OPEN_COUNT(node));
    SDDEBUG("unlink: %s\n", IS_UNLINKED(node)?"yes":"no");
    SDDEBUG("lbros:  '%s'\n", !node->little_bros ?
	    "<null>":node->little_bros->entry.name);
    SDDEBUG("bbros:  '%s'\n", !node->big_bros ?
	    "<null>":node->big_bros->entry.name);
    SDDEBUG("father: '%s'\n", !node->father ?
	    "<null>":node->father->entry.name);
    SDDEBUG("son:    '%s'\n", !node->son ?
	    "<null>":node->son->entry.name);
  } else {
    SDDEBUG("Name:   '<null>'\n");
  }
  SDUNINDENT;
}
 

/*
static void remap_node(node_t * n0, node_t *n1)
{
  SDDEBUG("%s [%p,%s] -> [%p,%s]\n", __FUNCTION__,
	  n0, n0 ? n0->entry.name : "<null>",
	  n1, n1 ? n1->entry.name : "<null>");
  if (!n0 || !n1) {
    return;
  }
  if (n0 == n1) {
    return;
  }
 
  if ((n1->son = n0->son) != 0) {
    n0->son->father = n1;
  }
  if ((n1->father = n0->father) != 0 && (n0->father->son == n0)) {
    n0->father->son = n1;
  }
  if ((n1->little_bros = n0->little_bros) != 0) {
    n0->little_bros->big_bros = n1;
  }
  if ((n1->big_bros = n0->big_bros) != 0) {
    n0->big_bros->little_bros = n1;
  }
}
*/

// $$$ Ben: Could have smarter alloc policy depending of the current file size
static int realloc_node(node_t * node, int req_size)
{
  void * data;
  int missing = req_size - node->max;

/*   SDDEBUG("%s [%s] to %d, missing %d\n", __FUNCTION__, */
/* 	  node->entry.name, req_size, missing); */

  if (missing <= 0) {
    return 0;
  }

  missing = (missing + ALLOC_BLOCK_SIZE-1) & -ALLOC_BLOCK_SIZE;
  missing += node->max;

  data = realloc(node->data, missing);
  if (data) {
    node->data = data;
    node->max = missing;
    return 0;
  } else {
    SDERROR("Node data allocation failure\n");
    return -1;
  }
}

static void clean_openfile(openfile_t *f)
{
  memset(f,0,sizeof(*f));
}

static void clean_openfiles(void)
{
  int i;

  for (i=0; i<MAX_RD_FILES; ++i) {
    clean_openfile(fh+i);
  }
}

static void clean_node(node_t * node)
{
  memset(node,0,sizeof(*node));
}


static node_t * create_node(const char *name, int datasize)
{
  node_t * node = 0;

  //  SDDEBUG("%s [%s] %d\n", __FUNCTION__, name, datasize);

  /* Forbid empty name, and minus filesize. */
  if (!valid_name(name, 0) || datasize < 0) {
    goto error;
  }

  /* Create the  node itself. */
  node = (node_t *)malloc(sizeof (*node));
  if (!node) {
    goto error;
  }
  clean_node(node);

  /* Copy filename. */
  strcpy(node->entry.name, name);

  if (!node->entry.name) {
    goto error;
  }

  /* Create data area */
  if (datasize > 0) {
    node->data = (uint8 *)malloc(datasize);
    if (!node->data) {
      SDERROR("Data allocation failure.\n");
      goto error;
    }
  }
  node->max = datasize;

  //  SDDEBUG("-->%s success\n", __FUNCTION__);
  return node;

 error:
  release_node(node);
  //  SDDEBUG("-->%s failure\n", __FUNCTION__);
  return 0;
} 


static node_t * find_node_same_level(node_t *node, const char *fn, int what)
{
  /*
  SDDEBUG("%s [%s] [%s] in [%s]\n", __FUNCTION__,
	  whatstr(what),
	  fn ? fn : "<null>",
	  node ? node->entry.name : "<null>");
  */
  SDINDENT;
  if (!node || !fn) {
    SDERROR("Invalid parms [%p] [%p]\n", node, fn);
    goto error;
  }

  /* Get first born */
  while (node->big_bros) {
    node = node->big_bros;
  }

  /* Search ... */
  for (; node; node = node->little_bros) {
    //    SDDEBUG("SCAN [%s]%s\n", node->entry.name, is_dir(node)?"*":"");
    if (IS_UNLINKED(node)) {
      SDDEBUG("Skipping [%s] (scheduled for unlink)\n", node->entry.name);
      continue;
    }

    if (!namecmp(fn, node->entry.name)) {
      /* Found it, is it a kind we want to ? 1:regular 2Ldir 3:both */
      if ( (is_dir(node) + 1) & what) {
	break;
      } else {
	//	SDDEBUG("BAD TYPE !!\n");
	node = 0;
	break;
      }
    }
  }
 error:
  if (node) {
    //    SDDEBUG("-->[%s]\n",node->entry.name);
  } else {
    //    SDDEBUG("NOT FOUND OR BAD TYPE\n");
  }
  SDUNINDENT;
  return node;
}

static node_t * find_node(node_t * node, char *fn, int what)
{
  char *fe;
  node_t * n = 0;
  int len, save_char;

/*   SDDEBUG("%s [%s] [%s] in [%s]\n", __FUNCTION__, */
/* 	  whatstr(what), */
/* 	  fn ? fn : "<null>", */
/* 	  node ? node->entry.name : "<null>"); */

  if (!node || !fn) {
    SDERROR("Find node invalid parameters [%p] [%p]\n", node, fn);
    return 0;
  }

  /* Scan for end of level name */
  for (fe=fn; *fe && *fe != '/'; ++fe)
    ;

  len = fe - fn;
  save_char = *fe;

  if (!save_char) {
    node_t *n;
    //    SDDEBUG("REACH LEAF LEVEL:\n");
    /* Reach end of path : search target (dir/file) */
    n = find_node_same_level(node, fn, what);
    if (!n) {
      SDERROR("NOT FOUND [%s] [%s]\n", fn, whatstr(what));
    }
    //    SDDEBUG("FOUND [%s] [%s]\n", fn, whatstr(what));
    return n;
  }

  /* Search a dir */
  *fe = 0;
  n = find_node_same_level(node, fn, 2);
  *fe = save_char;
  if (!n) {
    SDERROR("SUBDIR [%s] Not found\n", fn);
    return 0;
  }
  return find_node(n->son, fe+1, what);
}


static int get_openfile(void)
{
  int i;
		
  for (i=0; i<32; ++i) {
    if (!(fh_mask & (1<<i)) && !fh[i].node) {
      fh[i].node = (node_t *)1;  /* atomic : reserve this entry */
      fh_mask |= 1<<i; /* This is only for quick scan */
      return i;
    }
  }
  SDERROR("No free file handle.\n");

  return INVALID_FH;
}		

static ssize_t read(file_t fd, void * buffer, size_t size)
{
  uint8 * d = buffer;
  openfile_t * of;
  int end_pos, n;

  //  SDDEBUG("%s (%d, %p, %d)\n", __FUNCTION__, fd, buffer, size);
  
  fd = valid_regular(fd, READ_MODE);
  if (fd == INVALID_FH) {
    return -1;
  }
  if (size < 0) {
    SDERROR("minus size (%d)\n", size);
    return -1;
  }
  of = fh + fd;
	
  if (!size || of->pos >= of->node->entry.size) {
    return 0;
  }
		
  end_pos = of->pos + size;
  if (end_pos > of->node->entry.size) {
    end_pos = of->node->entry.size;
  }
  n = end_pos - of->pos;
  memcpy(d, of->node->data + of->pos, n);
  of->pos = end_pos;
	
  return n;
}


static ssize_t write(file_t fd, const void * buffer, size_t size)
{
  const uint8 * d = buffer;
  openfile_t * of;
  int end_pos, n;

  //  SDDEBUG("%s (%d, %p, %d)\n", __FUNCTION__, fd, buffer, size);

  fd = valid_file(fd, WRITE_MODE);
  if (fd == INVALID_FH) {
    return -1;
  }
  if (size < 0) {
    SDERROR("minus size:%d\n", size);
    return -1;
  }
  if (!size) {
    return 0;
  }
	
  of = fh + fd;
  end_pos = of->pos + size;

  if (end_pos > of->node->max) {
    realloc_node(of->node, end_pos);
    if (end_pos > of->node->max) {
/*       SDWARNING("[%s] : Realloc failed [end:%d] [max:%d]\n", */
/* 		__FUNCTION__, end_pos, of->node->max); */
      end_pos = of->node->max;
    }
  }
	
  n = end_pos - of->pos;
  memcpy(of->node->data + of->pos, d, n);
  //  SDDEBUG("%s copied [%p+%d %p %d]\n", __FUNCTION__,
  //	  of->node->data, of->pos, d, n);
  of->pos += n;
  if (of->pos > of->node->entry.size) {
    of->node->entry.size = of->pos;
  }
	
  //SDDEBUG("--> %d bytes\n", n);
  return n;
}

/* Create a valid filename, return leaf pointer. */
static char * valid_filename(char * fname, int max,
			     const char *fn, int *endslash, int accept_root)
{
  const char * save_fn = fn;
  int i, c, oc, leaf;

  /* Check filename */
  //  SDDEBUG("%s [%s]\n", __FUNCTION__, fn);
  if (!fn) {
    SDERROR("[<null>] : invalid filename.\n");
    return 0;
  }
/*   if (fn[0] != '/') { */
/*     SDERROR("Missing starting '/'\n"); */
/*     return 0; */
/*   } */

  /* Remove starting '/' */
  //  while (c = *fn++, c =='/')
  //    ;
  //SDDEBUG("remove starting '/' [%c%s]\n", c, fn);

  /* Copy string, remove repeated '/' and get last '/' position. */
  for (i=oc=leaf=0, c=*fn++; i<max && c; c=*fn++) {
    if (oc == '/') {
      if (c == '/') {
	continue;
      }
      leaf = i;
    }
    fname[i++] = oc = c;
  }
  if (c) {
    SDERROR("[%s] : filename too long.\n", save_fn);
    return 0;
  }
  /* Discard last char if it is a '/' */
  if (!!(*endslash = (oc == '/'))) {
    --i;
  }
  fname[i] = 0;
  //  SDDEBUG("copied string [%s] (slashended:%d)\n", fname, *endslash);

  /* If leaf separator has been found, to first char of leafname. */
  if (fname[leaf] == '/') {
    ++leaf;
  }
  //  SDDEBUG("leaf [%s]\n", fname+leaf);

  if (!valid_name(fname+leaf, accept_root)) {
    return 0;
  }

  return fname + leaf;
}

/* Open a file or directory */
static file_t open(const char *fn, int mode)
{
  uint32 fd = INVALID_FH;
  int omode=0;
  node_t * node, * created_node = 0, * father = 0;
  char fname[1024], *leaf;
  int endslash;

  leaf = valid_filename(fname, sizeof(fname), fn, &endslash, 1);
  if (!leaf) {
    goto error;
  }

  /* Checking open mode. */
  switch (mode & O_MODE_MASK) {
  case O_RDONLY:
    omode = READ_MODE;
    break;
  case O_RDWR:
    omode = READ_MODE | WRITE_MODE;
    break;
  case O_APPEND:
  case O_WRONLY:
    omode = WRITE_MODE;
    break;
  default:
    SDERROR("Unknown/unsupported open mode\n");
    goto error;
  }
  if (mode & O_DIR) {
    omode |= DIR_MODE;
  } else {
    /* Only root directory could have a empty name. 
       Only directory name could be '/' ended. */
    if (!leaf[0] || endslash)  {
      SDERROR("Invalid filename.\n");
      goto error;
    }
  }
	
  /* Get a free handle. */
  if (fd = get_openfile(), fd == INVALID_FH) {
    goto error;               
  }

  //  SDDEBUG("open('%s') in [%s]\n", fn , modestr(omode));

  //  SDDEBUG("Find [%s] [%s] node:\n", fn, whatstr(3));
  node = find_node(root, fname, 3);
  //  SDDEBUG("FOUND-NODE:\n");
  //  dump_node(node);

  if (node) {
    /* Node already exist. Perform some checks. */
    if ((omode ^ -is_dir(node)) & DIR_MODE) {
      SDERROR("Incompatible file/dir mode\n");
      goto error;
    }

    if (omode == (DIR_MODE | WRITE_MODE)) {
      SDERROR("Directory already exist\n");
      goto error;
    }

  } else /* if (!node) */ {
    if ( ! (omode & WRITE_MODE) ) {
      SDERROR("Not found and not created.\n");
      goto error;
    }

    //    SDDEBUG("Find father node\n");
    /* Node created, must be attached to father ... */
    if (leaf == fname) {
      father = root;
    } else {
      leaf[-1] = 0;
      father = find_node(root, fname, 2);
    }

    //    SDDEBUG("FATHER:\n");
    //    dump_node(father);

    if (!father) {
      SDERROR("Path not found.\n");
      goto error;
    }

    //    SDDEBUG("Create a new node [%s] for [%s]\n", leaf, modestr(omode));
    created_node = node = create_node(leaf, 0);
    //    SDDEBUG("CREATED-NODE:\n");
    //    dump_node(node);

    if (!node) {
      goto error;
    }
    node->entry.size = -!!(omode & DIR_MODE);

    attach_node(father, node);
  }

  /* Fill the fh structure */
  ++node->open;

  if (omode & DIR_MODE) {
    fh[fd].readdir = (omode & READ_MODE) ? node->son : 0;
  } else {
    fh[fd].pos = (mode & O_MODE_MASK) == O_APPEND ? node->entry.size : 0;
  }
  fh[fd].mode = omode;
  fh[fd].node = node; /* $$$ Last to be set, atomic op that valided
			 opened file */

  //  SDDEBUG("FINAL-NODE:\n");
  //  dump_node(node);
	
  //  SDDEBUG("[%s] open in [%s] := [%d]\n", fn, modestr(omode), fd+1);
  return fd + 1;
	
 error:
  if (fd != INVALID_FH) {		
    fh[fd].node = 0;
    fh_mask &= ~(1<<fd);
  }
  release_node(created_node);

  //  SDDEBUG("FAILED OPEN [%s] in [%s]\n", fn, modestr(omode));
  return 0;
}

static void really_unlink(node_t * node);

/* Close a file or directory */
static void close(uint32 fd)
{
  //  SDDEBUG("%s [%d]\n",__FUNCTION__, fd);

  /* Check that the fd is valid */
  if (fd = valid_file(fd, -1), fd != INVALID_FH) {
    if (!OPEN_COUNT(fh[fd].node)) {
      SDERROR("-->Invalid open count\n");
      return; 
    }
    --fh[fd].node->open;

    if (IS_UNLINKED(fh[fd].node) &&
	!OPEN_COUNT(fh[fd].node)) {
      really_unlink(fh[fd].node);
    }

    spinlock_lock(&fh_mutex);
    fh_mask &= ~(1 << fd);
    //    SDDEBUG("-->closed [%s]\n", fh[fd].node->entry.name);
    fh[fd].node = 0;
    fh[fd].mode = 0;
    spinlock_unlock(&fh_mutex);
  }
}

/* Seek elsewhere in a file */
static off_t seek(uint32 fd, off_t offset, int whence)
{
  //  SDDEBUG("Seek [%u] [%d] [%d]\n", fd, offset, whence);

  if (fd = valid_regular(fd, -1), fd == INVALID_FH) {	
    return -1;
  }

  /* Update current position according to arguments */
  switch (whence) {
  case SEEK_SET:
    fh[fd].pos = offset;
    break;
  case SEEK_CUR:
    fh[fd].pos += offset;
    break;
  case SEEK_END:
    fh[fd].pos = fh[fd].node->entry.size + offset;
    break;
  default:
    return -1;
  }
	
  /* Check bounds */
  if (fh[fd].pos < 0) {
    fh[fd].pos = 0;
  } 
  /* $$$ Ben: we could write outside file bound, that's the way to
   * to create big file with a single alloc !
   */ 
  /*else if (fh[fd].pos > fh[fd].node->entry.size) {
    fh[fd].pos = fh[fd].node->entry.size;
    }*/
	
  //  SDDEBUG("--> %s success (%u)\n", __FUNCTION__, fh[fd].pos);
  return fh[fd].pos;
}


/* Tell where in the file we are */
static off_t tell(uint32 fd)
{
  //  SDDEBUG("%s [%u]\n",__FUNCTION__, fd);
  if (fd = valid_regular(fd, -1), fd == INVALID_FH) {	
    return -1;
  }
  //  SDDEBUG("-->%s success [%u]\n",__FUNCTION__, fh[fd].pos);
  return fh[fd].pos;
}

/* Tell how big the file is */
static size_t total(uint32 fd)
{
  //  SDDEBUG("%s [%u]\n",__FUNCTION__, fd);

  if (fd = valid_fh(fd), fd == INVALID_FH) {	
    return -1;
  }
  //  SDDEBUG("-->%s success [%u]\n",__FUNCTION__, fh[fd].node->entry.size);
  return fh[fd].node->entry.size;
}

static dirent_t * readdir(file_t fd)
{
  node_t *rd;

  //  SDDEBUG("Readdir [%u]\n", fd);

  if (fd = valid_dir(fd, READ_MODE), fd == INVALID_FH) {
    return 0;
  }

  /* Don't need to check mode, readdir only set for READ_MODE */ 
  rd = fh[fd].readdir;
  if (rd) {
    fh[fd].readdir = rd->little_bros;
    /*
    SDDEBUG("-->%s success [%s] [%d]\n", __FUNCTION__,
	    rd->entry.name, rd->entry.size);
    */
    return &rd->entry;
  }

  //  SDDEBUG("-->end of dir\n");
  return 0;
}


static int ioctl(file_t fd, void *data, size_t size)
{
  SDDEBUG("%s [%u] [%p] [%u]\n", __FUNCTION__, fd, data, size);
  SDERROR("Unsupported\n");
  return -1;
}

// $$$ Ben: not surefor erro-code convention
static int rename(const char *fn1, const char *fn2)
{
  node_t * node1,  * node2;
  char fname1[1024], *leaf1;
  char fname2[1024], *leaf2;
  int slash_end;

  SDDEBUG("%s [%s] [%s]\n",__FUNCTION__, fn1, fn2);

  if (!fn1 || !fn2 || !fn1[0] || !fn2[0]) {
    SDERROR("Invalid parameter\n");
    return -1;
  }

  leaf1 = valid_filename(fname1, sizeof(fname1), fn1, &slash_end, 0);
  leaf2 = valid_filename(fname2, sizeof(fname2), fn2, &slash_end, 0);

  if (!leaf1 || !leaf2) {
    return -1;
  }

  if (!namecmp(fname1, fname2)) {
    SDDEBUG("-->Nothing to do\n");
    return 0;
  }

  /* Find the files */
  node1 = find_node(root, fname1, 3);
  if (!node1) {
    return -1;
  }

  /* Check if a file already exist with new name */
  node2 =  find_node(root, fname2, 3);
  if (node2) {
    return -1;
  }

  /* $$$ unfinished */
  SDERROR("Not implemented !\n");
  return -1;


  SDDEBUG("-->%s, success\n", __FUNCTION__);
  return 0;
}

static void really_unlink(node_t * node)
{
  //  SDDEBUG("%s [%s]\n",__FUNCTION__,node ? node->entry.name : "<null>");
  SDINDENT;

  if (node && node->son) {
    SDCRITICAL("Trying to unlink non-empty dir\n");
    return;
  }

  //  SDDEBUG("UNLINKING [%s]\n", node->entry.name); 

  detach_node(node);
  release_node(node);

  SDUNINDENT;
  return;
}

static int unlink(const char *fn)
{
  node_t *node;
  char fname[1024], *leaf;
  int slash_end;

  //  SDDEBUG("%s [%s]\n",__FUNCTION__, fn);

  leaf = valid_filename(fname, sizeof(fname), fn, &slash_end, 0);
  if (!leaf) {
    return -1;
  }

  /* Find a regular file or a directory */
  node = find_node(root, fname, 3 - slash_end);
  if (!node) {
    return -1;
  }

  if (is_dir(node) && node->son) {
    /* Can't unlink non empty dir. */
    SDERROR("Directory not empty.\n");
    return -1;
  }

  if (OPEN_COUNT(node)) {
    /* Node is open... Just flag it for unlinking. */
    SDDEBUG("-->Scheduled for unlink\n");
    node->open |= UNLINKED;
  } else {
    /* Node is closed... */
    really_unlink(node);
  }
  return 0;
}

static void * mmap(file_t fd)
{
  if (fd = valid_regular(fd,-1), fd == INVALID_FH) {	
    return 0;
  }
  return fh[fd].node->data;
}

/* Put everything together */
static vfs_handler vh = {
  "ramdisk",            /* name */
  0, 0, NULL,		/* In-kernel, no cacheing, next */
  open,
  close,
  read,
  write,
  seek,
  tell,
  total,
  readdir,
  ioctl,
  rename,
  unlink,
  mmap
};


static void test();

/* Initialize the file system */
int fs_ramdisk_init(int max_size)
{
  SDDEBUG("%s [%d]\n", __FUNCTION__, max_size);

  /* Could not create 2 ramdisks. */
  if (root) {
    SDERROR("Sorry, only one RAM disk is available.\n");
    return -1;
  }
  
  clean_node(&root_node);
  root_node.entry.size = -1;

  /* Set max size to max if 0 is requested. $$$ BEN: Note used  */ 
  max_size -= !max_size;

  clean_openfiles();
	
  /* Init thread mutexes */
  spinlock_init(&fh_mutex);

  /* Register with VFS */
  if (fs_handler_add("/ram", &vh)) {
    SDERROR("-->fs_handler_add failed\n");
    return -1;
  }
  fh_mask = 0;
  root = &root_node;

#if DEBUG && 0
  test();
#endif


  return 0;
}
/* De-init the file system */
int fs_ramdisk_shutdown(void)
{
  SDDEBUG("%s\n", __FUNCTION__);

  if (root) {
    spinlock_lock(&fh_mutex);
    fh_mask = 0;
    clean_openfiles();
    release_nodes(root);
    spinlock_unlock(&fh_mutex);
    root = 0;
  }

  return fs_handler_remove(&vh);
}


#if DEBUG

static int copyfile(const char *fn1, const char *fn2)
     /* $$$ Give it a try */
{
  file_t fd, fd2;

  SDDEBUG("%s [%s] <- [%s]\n", __FUNCTION__, fn1, fn2);
  SDINDENT;

  fd  = fs_open(fn1, O_WRONLY);
  fd2 = fs_open(fn2, O_RDONLY);

  if (fd && fd2) {
    char buf[256];
    int n;

    do {
      n = fs_read(fd2, buf, 256);
      if (n > 0) {
	fs_write(fd,buf,n);
      }
    } while (n > 0);
  }

  if (fd) fs_close(fd);
  if (fd2) fs_close(fd2);

  SDUNINDENT;
  return 0;
}

static int createdir(const char *fn)
{
  file_t fd;

  SDDEBUG("%s [%s]\n", __FUNCTION__, fn);
  SDINDENT;

  fd  = fs_open(fn, O_WRONLY|O_DIR);
  if (fd) {
    fs_close(fd);
  }
  SDUNINDENT;
  return 0;
}


static void test()
{
  SDDEBUG("Perform RAMdisk test suit...\n");
  SDINDENT;

  copyfile("/ram/toto.sid", "/pc/Shades.sid");
  copyfile("/ram/zorglub.sid", "/pc/Synapse.sid");
  copyfile("/ram//toto-bak.sid", "/ram/toto.sid");
  copyfile("//ram/toto.sid", "/pc/Synapse.sid");
  copyfile("/ram/invalid.sid/", "/pc/Synapse.sid");

  createdir("/ram/sid");
  createdir("/ram/sid2/sid3");
  createdir("/ram/sid2/");
  createdir("/ram/sid2/sid3");
  copyfile("/ram/sid/sid1.sid", "/ram/zorglub.sid");
  copyfile("/ram/sid/sid2.sid", "/ram/toto.sid");
  copyfile("/ram/sid2/sid1.sid", "/ram/zorglub.sid");
  copyfile("/ram/sid2/sid2.sid", "/ram/toto.sid");
  
  SDUNINDENT;
}

#endif
