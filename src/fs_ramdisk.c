/**
 * @file    fs_ramdisk.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   RAM disk for KOS file system
 * 
 * $Id: fs_ramdisk.c,v 1.2 2002-09-17 23:28:41 ben Exp $
 */

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

/* Opening mode bit. DIR_MODE is exclusive. */
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

static int valid_fh(uint32 fd)
{
  if (--fd >= MAX_RD_FILES || !(fh_mask & (1<<fd)) || !fh[fd].node>1) {
    return INVALID_FH;
  }
  return fd;
}

static int valid_file(uint32 fd)
{
  if (fd = valid_fh(fd), fd != INVALID_FH) {
    if (!(fh[fd].mode & (READ_MODE|WRITE_MODE))) {
      fd = INVALID_FH;
    }
  }
  return fd;
}

static int valid_dir(uint32 fd)
{
  if (fd = valid_fh(fd), fd != INVALID_FH) {
    if (!is_dir(fh[fd].node) || (fh[fd].mode != (READ_MODE|DIR_MODE))) {
      fd = INVALID_FH;
    }
  }
  return fd;
}

static void release_node(node_t * node)
{
  SDDEBUG( "%s [%s]\n", __FUNCTION__, node ? node->entry.name : "<null>"); 
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

  missing = (missing + ALLOC_BLOCK_SIZE-1) & ALLOC_BLOCK_SIZE;
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

static node_t * youngest_bros(node_t * node)
{
  if (!node) {
    return 0;
  }
  for(;node->little_bros; node = node->little_bros)
    ;
  return node;
}

static int is_dir(node_t * node)
{
  return node && node->entry.size == -1;
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

  SDDEBUG("%s [%s] %d\n", __FUNCTION__, name, datasize);

  /* Forbid empty name, and minus filesize. */
  if (!name || !name[0] || datasize < 0 ||
      strlen(name) >= sizeof(node->entry.name)) {
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
  if (datasize) {
    node->data = (uint8 *)malloc(datasize);
    if (!node->data) {
      SDERROR("Data allocation failure.\n");
      goto error;
    }
  }
  node->max = datasize;

  SDDEBUG("-->%s success\n", __FUNCTION__);
  return node;

 error:
  release_node(node);
  SDDEBUG("-->%s failure\n", __FUNCTION__);
  return 0;
} 


static node_t * find_node_same_level(node_t *node, const char *fn, int what)
{
  SDDEBUG("%s [%s] [%s] [%d]\n", __FUNCTION__,
	  node ? node->entry.name : "<null>", fn, what);
  if (!node || !fn) {
    SDERROR("Invalid parms [%p] [%p]\n", node, fn);
    return 0;
  }

  /* Get first born */
  while (node->big_bros) {
    node = node->big_bros;
  }

  /* Search */
  for (; node; node = node->little_bros) {
    if (IS_UNLINKED(node)) {
      SDDEBUG("Skipping [%s] (scheduled for unlink)\n", node->entry.name);
      continue;
    }

    if (!namecmp(fn, node->entry.name)) {
      /* Found it, is it a kind we want to ? 1:regular 2Ldir 3:both */
      if ((is_dir(node) + 1) & what) {
	SDDEBUG("-->%s success\n", __FUNCTION__);
	return node;
      } else {
	SDDEBUG("-->%s failure : bad node type\n", __FUNCTION__);
	return 0;
      }
    }
  }
  return 0;
}

static node_t * find_node(node_t * node, const char *fn, int what)
{
  const char *fe;
  char name[sizeof(node->entry.name)];
  node_t * n = 0;
  int len;

  SDDEBUG("%s [%s] [%s] [%d]\n", __FUNCTION__,
	  node ? node->entry.name : "<null>", fn ? fn : "<null>", what);

  if (!node || !fn) {
    SDERROR("Find node invalid parameters [%p] [%p]\n", node, fn);
    return 0;
  }

/*   while (*fn == '/') { */
/*     ++fn; */
/*   } */

  for (fe=fn; *fe && *fe != '/'; ++fe)
    ;

  len = fe - fn;

  /* len could be 0 for root node ! */
  if (len >= sizeof(name)) {
    SDERROR("Find node invalid filename length\n");
    return 0;
  }

  memcpy(name, fn, len);
  name[len] = 0;

  if (!*fe) {
    node_t *n;
    /* Search something */
    n = find_node_same_level(node, name, what);
    if (!n) {
      SDERROR("Find node [%s] mode=%d not found\n", name, what);
    }
    return n;
  }

  /* Search a dir */
  n = find_node_same_level(node, name, 2);
  if (!n) {
    SDERROR("Find node : dir [%s] not found\n", name);
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
  
  fd = valid_file(fd);
  if (fd == INVALID_FH || ! (fh[fd].mode & READ_MODE) || size < 0) {
    SDERROR("-->Invalid file or parameters\n");
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
  //  SDDEBUG("%s copied [%p %p+%d %d]\n", __FUNCTION__,
  //	  d, of->node->data, of->pos, n);
  of->pos += n;

  //  SDDEBUG("--> %d bytes\n", n);
	
  return n;
}


static ssize_t write(file_t fd, const void * buffer, size_t size)
{
  const uint8 * d = buffer;
  openfile_t * of;
  int end_pos, n;

  //  SDDEBUG("%s (%d, %p, %d)\n", __FUNCTION__, fd, buffer, size);

  fd = valid_file(fd);
  if (fd == INVALID_FH || ! (fh[fd].mode & WRITE_MODE) || size < 0) {
    SDERROR("-->Invalid file or parameters\n");
    return -1;
  }
  of = fh + fd;
  if (!size) {
    return 0;
  }
	
  end_pos = of->pos + size;
	
  if (end_pos > of->node->max) {
    realloc_node(of->node, end_pos);
    if (end_pos > of->node->max) {
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
	
  //  SDDEBUG("--> %d bytes\n", n);
  return n;
}

static file_t open_dir(const char *fn, int mode)
{
  file_t fd;
  node_t * node;

  SDDEBUG("open dir [%s] mode=%x\n", fn, mode);

  if ((mode & O_MODE_MASK) != O_RDONLY) {
    SDERROR("Invalid dir open mode (%x)\n", mode);
    return 0;
  }

  node = find_node(root, fn, 2);
  if (!node) {
    return 0;
  }

  fd = get_openfile();
  if (fd == INVALID_FH) {
    return 0;
  }

  /* Fill the fh structure */
  ++node->open;
  fh[fd].readdir = node->son;
  fh[fd].mode = READ_MODE|DIR_MODE;
  fh[fd].node = node;
  
  SDDEBUG("-->%s success (%d)\n", __FUNCTION__, fd + 1);
  return fd + 1;
}

/* Open a file or directory */
static file_t open(const char *fn, int mode)
{
  uint32 fd = INVALID_FH;
  int omode;
  node_t * node;

  SDDEBUG("open [%s] [%x]\n", fn, mode);
	
  /* Check for extended mode */
  if ((mode & ~O_MODE_MASK)) {
    if (mode & O_DIR) {
      return open_dir(fn, mode);
    } else {
      /* $$$ BEN: Don't know what to do with O_META and O_TRUNC
       * $$$ BEN: O_TRUNC means files is emptied at creation. Not supported
       *     by this version of KOS, but ok with latest.
       */
      SDERROR("Unknown/unsupported extended open mode\n");
      goto error;
    }
  }
	
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

  if (!fn || !fn[0]) {
    SDERROR("Invalid filename\n");
    goto error;
  }
	
  /* Get a free handle. */
  if (fd = get_openfile(), fd == INVALID_FH) {
    goto error;               
  }
	
  /* Look for the file */
  node = find_node(root, fn, 1);
  if (!node) {
      /* $$$ Ben: Does all write mode create file if not exist ? */
    if (omode & WRITE_MODE) {
      const char *leaf = strrchr(fn,'/');
      node_t * father;

      leaf = leaf ? leaf+1 : fn;
      SDDEBUG("Create a new file [%s]\n",leaf); 
      node = create_node(leaf, 0);
      if (!node) {
	goto error;
      }

      /* Node created, must be attached to father ... */
      if (leaf == fn) {
	father = root;
      } else {
	char * tmp = strdup(fn);
	if (!tmp) {
	  SDERROR("Temporary filename allocation failed\n");
	  release_node(node);
	  goto error;
	}
	tmp [leaf-1-fn] = 0;
	SDDEBUG("Find path [%s]\n", tmp);
	father = find_node(root, tmp, 2);
	free(tmp);
      }
      if (!father) {
	SDERROR("Path not found\n");
	release_node(node);
	goto error;
      }

      // $$$ Ben: dir list will be inversed 
      node->father = father;
      node->little_bros = 0;
      if ((node->big_bros = youngest_bros(father->son)) != 0) {
	node->big_bros->little_bros = node;
      } else {
	father->son = node;
      }

    } else {
      goto error;
    }

  }
	
  /* Fill the fh structure */
  ++node->open;
  fh[fd].pos = (mode & O_MODE_MASK) == O_APPEND ? node->entry.size : 0;
  fh[fd].mode = omode;
  fh[fd].node = node; /* $$$ Last to be set, atomic op that valided
			 opened file */
	
  SDDEBUG("-->%s success (%d)\n", __FUNCTION__, fd + 1);
  return fd + 1;
	
 error:
  if (fd != INVALID_FH) {		
    fh[fd].node = 0;
    fh_mask &= ~(1<<fd);
  }
  SDDEBUG("-->%s failure\n", __FUNCTION__);
  return 0;
}

static void really_unlink(node_t * node);

/* Close a file or directory */
static void close(uint32 fd)
{
  SDDEBUG("%s [%d]\n",__FUNCTION__, fd);

  /* Check that the fd is valid */
  if (fd = valid_fh(fd), fd != INVALID_FH) {
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
    SDDEBUG("-->closed [%s]\n", fh[fd].node->entry.name);
    fh[fd].node = 0;
    fh[fd].mode = 0;
    spinlock_unlock(&fh_mutex);
  }
}

/* Seek elsewhere in a file */
static off_t seek(uint32 fd, off_t offset, int whence)
{
  SDDEBUG("Seek [%u] [%d] [%d]\n", fd, offset, whence);

  if (fd = valid_file(fd), fd == INVALID_FH) {	
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
	
  SDDEBUG("--> %s success (%u)\n", __FUNCTION__, fh[fd].pos);
  return fh[fd].pos;
}


/* Tell where in the file we are */
static off_t tell(uint32 fd)
{
  SDDEBUG("%s [%u]\n",__FUNCTION__, fd);
  if (fd = valid_file(fd), fd == INVALID_FH) {	
    return -1;
  }
  SDDEBUG("-->%s success [%u]\n",__FUNCTION__, fh[fd].pos);
  return fh[fd].pos;
}

/* Tell how big the file is */
static size_t total(uint32 fd)
{
  SDDEBUG("%s [%u]\n",__FUNCTION__, fd);

  if (fd = valid_fh(fd), fd == INVALID_FH) {	
    return -1;
  }
  SDDEBUG("-->%s success [%u]\n",__FUNCTION__, fh[fd].node->entry.size);
  return fh[fd].node->entry.size;
}

static dirent_t * readdir(file_t fd)
{
  node_t *rd;

  SDDEBUG("Readdir [%u]\n", fd);

  if (fd = valid_dir(fd), fd == INVALID_FH) {
    return 0;
  }
  
  rd = fh[fd].readdir;
  if (rd) {
    fh[fd].readdir = rd->little_bros;
    SDDEBUG("-->%s success [%s] [%d]\n", __FUNCTION__,
	    rd->entry.name, rd->entry.size);
    return &rd->entry;
  }

  SDDEBUG("-->end of dir\n");
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
  node_t * node;

  SDDEBUG("%s [%s] [%s]\n",__FUNCTION__, fn1, fn2);

  if (!fn1 || !fn2 || !fn1[0] || !fn2[0] ||
      strlen(fn2) >= sizeof(node->entry.name)) {
    SDERROR("Invalid parameter\n");
    return -1;
  }

  if (!namecmp(fn1, fn2)) {
    SDDEBUG("-->Nothing to do\n");
    return 0;
  }

  /* Find the file */
  node = find_node(root, fn1, 3);
  if (!node) {
    return -1;
  }

  /* Check if a file already exist with new name */
  if (find_node_same_level(node, fn2, 3)) {
    return -1;
  }

  strcpy(node->entry.name, fn2);
  SDDEBUG("-->%s, success\n", __FUNCTION__);
  return 0;
}

static void really_unlink(node_t * node)
{
  SDDEBUG("%s [%s]\n",__FUNCTION__,node ? node->entry.name : "<null>");

  if (!node || node->son) {
    return;
  }

  SDINDENT;
  if (node->father) {
    node->father->son = 0;
  }
  if (node->big_bros) {
    node->big_bros = node->little_bros;
  }
  if (node->little_bros) {
    node->little_bros = node->big_bros;
  }
  release_node(node);
  SDUNINDENT;

  return;
}

static int unlink(const char *fn)
{
  node_t *node;

  SDDEBUG("%s [%s]\n",__FUNCTION__, fn);

  /* Find a regular file or a directory */
  node = find_node(root, fn, 3);
  if (!node) {
    return -1;
  }

  if (is_dir(node) && node->son) {
    /* Can't unlink non empty dir. */
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
  if (fd = valid_file(fd), fd == INVALID_FH) {	
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

#if DEBUG
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

int copyfile(const char *fn1, const char *fn2)
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

static void test()
{
  SDDEBUG("Perform RAMdisk test suit...\n");
  SDINDENT;

  copyfile("/ram/toto.sid", "/pc/Shades.sid");
  copyfile("/ram/zorglub.sid", "/pc/Synapse.sid");
  copyfile("/ram/toto-bak.sid", "/ram/toto.sid");
  copyfile("/ram/toto.sid", "/pc/Synapse.sid");
  
  SDUNINDENT;
}

#endif
