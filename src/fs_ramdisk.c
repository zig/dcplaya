/**
 * @file    fs_ramdisk.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   RAM disk for KOS file system
 * 
 * $Id: fs_ramdisk.c,v 1.18 2004-07-31 22:55:19 vincentp Exp $
 */

/** @TODO add lock to file-handle. It is not thread-safe right now !!! */
/** I'll try to use the node-mutex as a fix. It should be ok. */

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

#include "dcplaya/config.h"
#include "sysdebug.h"
#include "fs_ramdisk.h"

/* #define RAMDISK_TEST */

/* Allocation block size */
#define ALLOC_BLOCK_SIZE 1024

/* Opening mode bit. */
#define READ_MODE       1
#define WRITE_MODE 	2
#define DIR_MODE 	4

#define MAX_RD_FILES 	32            /**< Must not excess 32 */
#define INVALID_FH  	MAX_RD_FILES

/** RAM disk i-node */
typedef struct _node_s {
  struct _node_s   * big_bros;       /**< Previous node same level   */
  struct _node_s   * little_bros;    /**< Next node same level       */
  struct _node_s   * father;         /**< Previous level node        */
  struct _node_s   * son;            /**< Next level node (dir only) */

  dirent_t           entry;          /**< Kos directory entry.       */
  int                max;            /**< Data allocated size.       */
  uint8            * data;           /**< File data                  */
  struct {
    unsigned int open:29;            /**< Open count                 */
    unsigned int unlinked:1;         /**< Node requested for unlink  */
    unsigned int notify:1;           /**< Node send notification     */
    unsigned int modified:1;         /**< Node modified              */
  } flags;                           /**< Node flags.                */
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

static struct vfs_handler vh;

/* Notification :
 *  This mechanism is use to notify ramdisk modification.
 */
/* Set when ramdisk is modified a notification not read. */
static int modify; 

/* File data alignment */
const int align = 16;

/* Mutex for file handles */
static spinlock_t fh_mutex;

/* Mutex for node modification (modify notify) */
static spinlock_t node_mutex;

#ifdef DEBUG
#define LOCK_NODE() \
if (node_mutex) {\
  SDCRITICAL("[%s] : already locked\n",__FUNCTION__);\
} else {\
  spinlock_lock(&node_mutex);\
}

#define UNLOCK_NODE() \
if (!node_mutex) {\
  SDCRITICAL("[%s] : already unlocked\n",__FUNCTION__);\
} else {\
  spinlock_unlock(&node_mutex);\
}

#else
# define LOCK_NODE() if (1) {spinlock_lock(&node_mutex);} else
# define UNLOCK_NODE() if (1) {spinlock_unlock(&node_mutex);} else
#endif

static void dump_node(node_t * node, const char * label);

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
/*   father->flags.modified = 1; /\* Attach modify father ! *\/ */
  node->flags.notify = father->flags.notify;
}

static void detach_node(node_t * node)
{
  int i;

  //  SDDEBUG("%s [%s]\n",__FUNCTION__, node ? node->entry.name : "<null>");
  if (!node) {
    return;
  }

#if DEBUG
  if (node == root) {
    SDCRITICAL("[%s] : Detaching root node\n",__FUNCTION__);
    dump_node(node,"Detaching root node");
  }
#endif

  /* Remove this node from directory reading. */
  for (i=0; i<MAX_RD_FILES; ++i) {
    if (fh[i].readdir == node) {
      fh[i].readdir = node->little_bros;
    }
  }
  /* This node is the first son of its father,
     them its little bros become the first son.
     In that case the node should not have big brother !!
  */
  if (node->father && node->father->son == node) {
      node->father->son = node->little_bros;
#if DEBUG
      if (node->big_bros) {
	SDCRITICAL("[%s] : node have an unexpected big bros\n",__FUNCTION__);
	dump_node(node,"unexpected big bros");
      }
#endif
  }
  /* Correct little/big brother hierarchy */
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
    if (node != &root_node) {
      free(node);
    }
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

static void dump_node(node_t * node, const char * label)
{
  SDINDENT;
  SDDEBUG("Dump node [%s]\n", label);
  SDINDENT;
  if (node) {
    SDDEBUG("Name:   '%s'\n", node->entry.name);
    SDDEBUG("size:   %d\n", node->entry.size);
    SDDEBUG("alloc:  %d\n", node->max);
    SDDEBUG("open:   %d\n", node->flags.open);
    SDDEBUG("notify: %d\n", node->flags.notify);
    SDDEBUG("modify: %d\n", node->flags.modified);
    SDDEBUG("unlink: %d\n", node->flags.unlinked);
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
  /* created node is always modified ;) becauze it modify the directory 
   * structure.  
   */
  node->flags.modified = 1;

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
    if (node->flags.unlinked) {
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
/*     SDERROR("Find node invalid parameters [%p] [%p]\n", node, fn); */
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

  if (!size) {
    return 0;
  }
		
  LOCK_NODE();
  end_pos = of->pos + size;
  if (end_pos > of->node->entry.size) {
    end_pos = of->node->entry.size;
  }
  n = end_pos - of->pos;
  memcpy(d, of->node->data + of->pos, n);
  of->pos = end_pos;
  UNLOCK_NODE();
	
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
  LOCK_NODE();
  end_pos = of->pos + size;
  if (end_pos > of->node->max) {
    realloc_node(of->node, end_pos);
    if (end_pos > of->node->max) {
      SDWARNING("[%s] : Realloc failed [end:%d] [max:%d]\n",
		__FUNCTION__, end_pos, of->node->max);
      end_pos = of->node->max;
    }
  }

  n = end_pos - of->pos;
#if DEBUG
  if (n < 0) {
    SDCRITICAL("[%s] : n=%d\n",__FUNCTION__,n);
    n = 0;
  }
#endif
  
  memcpy(of->node->data + of->pos, d, n);
  //  SDDEBUG("%s copied [%p+%d %p %d]\n", __FUNCTION__,
  //	  of->node->data, of->pos, d, n);
  of->pos += n;
  if (of->pos > of->node->entry.size) {
    of->node->entry.size = of->pos;
  }

  if (n>0) {
    /* Writing some data modify node */
    of->node->flags.modified = 1;
  }

  UNLOCK_NODE();
	
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
static file_t open(vfs_handler_t * vfs, const char *fn, int mode)
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
    SDERROR("Unknown/unsupported open mode %d\n", mode & O_MODE_MASK);
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
  LOCK_NODE();
  node = find_node(root, fname, 3);

  //  SDDEBUG("FOUND-NODE:\n");
  //  dump_node(node);

  if (node) {
    /* Node already exist. Perform some checks. */
    if ((omode ^ -is_dir(node)) & DIR_MODE) {
      SDERROR("Incompatible file/dir mode\n");
      UNLOCK_NODE();
      goto error;
    }

    if (omode == (DIR_MODE | WRITE_MODE)) {
      SDERROR("Directory already exist\n");
      UNLOCK_NODE();
      goto error;
    }
  } else /* if (!node) */ {
    if ( ! (omode & WRITE_MODE) ) {
/*       SDERROR("Not found and not created.\n"); */
      UNLOCK_NODE();
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
      UNLOCK_NODE();
      goto error;
    }

    //    SDDEBUG("Create a new node [%s] for [%s]\n", leaf, modestr(omode));
    created_node = node = create_node(leaf, 0);
    //    SDDEBUG("CREATED-NODE:\n");
    //    dump_node(node);

    if (!node) {
      UNLOCK_NODE();
      goto error;
    }

    /* Node inherit notification prperties from its father. */
    node->entry.size = -!!(omode & DIR_MODE);
    attach_node(father, node);
  }
  ++node->flags.open;

  /* Fill the fh structure */
  if (omode & DIR_MODE) {
    fh[fd].readdir = (omode & READ_MODE) ? node->son : 0;
#if DEBUG
/*     dump_node(fh[fd].readdir,"OPEN-DIR"); */
#endif
  } else {
    fh[fd].pos = (mode & O_MODE_MASK) == O_APPEND ? node->entry.size : 0;
  }
  fh[fd].mode = omode;
  fh[fd].node = node; /* $$$ Last to be set, atomic op that valided
			 opened file */

  // dump_node(node, fn);
  UNLOCK_NODE();

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

static void really_unlink(vfs_handler_t * vfs, node_t * node);

/* Close a file or directory */
static void close(uint32 fd)
{
  //  SDDEBUG("%s [%d]\n",__FUNCTION__, fd);

  /* Check that the fd is valid */
  if (fd = valid_file(fd, -1), fd != INVALID_FH) {
    node_t * node = fh[fd].node;
    int open;
    LOCK_NODE();
    
    if (open = node->flags.open, !open) {
      SDERROR("-->Invalid open count\n");
      UNLOCK_NODE();
      return;
    }
    node->flags.open = --open;
    if(!open && node->flags.notify && node->flags.modified) {
      modify = 1;
#if DEBUG
/*       dump_node(node,"CLOSE-MODIFY"); */
#endif
    }
    node->flags.modified = 0;

    if (node->flags.unlinked && !open) {
      really_unlink(&vh, fh[fd].node);
    }
    UNLOCK_NODE();

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
  dirent_t *e;

  //  SDDEBUG("Readdir [%u]\n", fd);

  if (fd = valid_dir(fd, READ_MODE), fd == INVALID_FH) {
    return 0;
  }

  LOCK_NODE();
  /* Don't need to check mode, readdir only set for READ_MODE */
  for (rd = fh[fd].readdir; rd && rd->flags.unlinked; rd = rd->little_bros) {
#if DEBUG
/*     dump_node(rd,"Skip-Dir"); */
#endif
  }
    ;
  if (rd) {
    /* Found one, set pointer to next */
    e = &rd->entry;
/*     dump_node(rd,"Read-Dir"); */
    rd = rd->little_bros;
  } else {
    e = 0;
  }
  fh[fd].readdir = rd; 

  UNLOCK_NODE();

  //  SDDEBUG("-->end of dir\n");
  return e;
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

static void really_unlink(vfs_handler_t * vfs, node_t * node)
{
  //  SDDEBUG("%s [%s]\n",__FUNCTION__,node ? node->entry.name : "<null>");
  SDINDENT;

  if (node && node->son) {
    SDCRITICAL("Trying to unlink non-empty dir\n");
    return;
  }

  //  SDDEBUG("UNLINKING [%s]\n", node->entry.name); 

  detach_node(node);
  if (node->flags.notify) {
/*     dump_node(node,"MODIFY BY UNLINK"); */
    modify = 1;
  }
  release_node(node);

  SDUNINDENT;
  return;
}

static int unlink(vfs_handler_t * vfs, const char *fn)
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

  LOCK_NODE();

  if (node->flags.open) {
    /* Node is open... Just flag it for unlinking. */
    SDDEBUG("-->Scheduled for unlink\n");
    node->flags.unlinked = 1;
  } else {
    /* Node is closed... */
    really_unlink(vfs, node);
  }
  UNLOCK_NODE();

  return 0;
}

/* Assume node will be modified ! */
static void * mmap(file_t fd)
{
  if (fd = valid_regular(fd,-1), fd == INVALID_FH) {
    return 0;
  }
  LOCK_NODE();
  fh[fd].node->flags.modified = 1;
  UNLOCK_NODE();

  return fh[fd].node->data;
}

/* Put everything together */
static struct vfs_handler vh = {
  {
    { "/ram" },            /* name */
    0, 
    0x00010000,		/* Version 1.0 */
    0,			/* flags */
    NMMGR_TYPE_VFS,	/* VFS handler */
    NMMGR_LIST_INIT	/* list */
  },
  0, NULL,		/* In-kernel, no cacheing, next */
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

/* Initialize the file system */
int dcpfs_ramdisk_init(int max_size)
{
  SDDEBUG("[%s] : [max_size:%d]\n", __FUNCTION__, max_size);

  /* Could not create 2 ramdisks. */
  if (root) {
    SDERROR("Sorry, only one RAM disk is available.\n");
    return -1;
  }
  
  clean_node(&root_node);
  root_node.entry.size = -1;
  root_node.flags.notify = 1;
  
  if (max_size) {
    SDNOTICE("[%s] : max_size [%d] ignored (not implemented\n",
	     __FUNCTION__, max_size);
  }

  /* Set max size to max if 0 is requested. $$$ BEN: Not used  */ 
  max_size -= !max_size;

  clean_openfiles();
	
  /* Init thread mutexes */
  spinlock_init(&fh_mutex);
  spinlock_init(&node_mutex);

  /* Register with VFS */
  if (nmmgr_handler_add(&vh)) {
    SDERROR("[%s] : fs_handler_add failed\n", __FUNCTION__);
    return -1;
  }
  fh_mask = 0;
  root = &root_node;
  modify = 0;

  return 0;
}

/* De-init the file system */
int dcpfs_ramdisk_shutdown(void)
{
  SDDEBUG("%s\n", __FUNCTION__);

  if (root) {
    spinlock_lock(&fh_mutex);
    LOCK_NODE();
    fh_mask = 0;
    clean_openfiles();
    release_nodes(root);
    root = 0;
  }
  modify = 0;

  return nmmgr_handler_remove(&vh);
}

/* Check modification flag */
int fs_ramdisk_modified(void)
{
  int ret;
  LOCK_NODE();
  ret = modify;
  modify = 0;
  UNLOCK_NODE();
  return ret;
}

static void notify_node(node_t * node, int notify)
{
  if (node) {
    node->flags.notify = notify;
/*     dump_node(node, "Notify"); */
  }
}

static void notify_nodes(node_t * node, node_t * except, int notify)
{
  if (!node || node == except) {
    return;
  }
  notify_nodes(node->son, except, notify);
  notify_nodes(node->little_bros, except, notify);
  notify_node(node, notify);
}

int fs_ramdisk_notify_path(const char *path)
{
  node_t * node;
  char fname[1024];
  const char * leaf;
  int slash_end;

  if (!path) {
    /* Desactive all notifications */
    LOCK_NODE();
    notify_nodes(root, 0, 0);
    UNLOCK_NODE();
    return 0;
  }

  if (strstr(path,"/ram/") == path) {
    path += 4;
  }

  leaf = valid_filename(fname, sizeof(fname), path, &slash_end, 0);
  if (!leaf) {
    SDERROR("[fs_ramdisk_notify_path] : [%s] no leaf\n", path);
    return -1;
  }

  LOCK_NODE();
  node = find_node(root, fname, 2);
  if (!node) {
    UNLOCK_NODE();
    SDERROR("[fs_ramdisk_notify_path] : [%s] not found\n", fname);
    return -1;
  }

  notify_nodes(root, node, 0);
  notify_nodes(node, 0, 1);

  UNLOCK_NODE();
  return 0;
}
