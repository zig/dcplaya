/**
 * dreammp3 - Plugin loader
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: plugin.c,v 1.2 2002-09-04 18:54:11 ben Exp $
 */
#include <stdio.h>
#include <string.h>
#include <kos/fs.h>

#include "any_driver.h"
#include "driver_list.h"
#include "lef.h"
#include "filetype.h"

any_driver_t * plugin_load(const char *fname)
{
  int fd;
  lef_prog_t * prog = 0;
  any_driver_t *d = 0;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%s)\n", fname);

  fd = fs_open(fname, O_RDONLY);
  if (!fd) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__ 
	   " : Plugin file not found [%s]\n", fname);
    goto error;
  }

  d = 0;
  /* $$$ After this call, ffd is closed by lef_load !!! */
  prog = lef_load(fd);
  fd = 0;

  if (!prog) {
    dbglog(DBG_ERROR, "!! " __FUNCTION__
	   " : Plugin [%s] load error\n", fname);
    return 0;
  }
  dbglog(DBG_DEBUG,"** " __FUNCTION__ 
	 " : Plugin [%s] load success.\n", fname);
  d = (any_driver_t *)prog->ko_main(0, 0);
  if (!d) {
    dbglog(DBG_ERROR,"** " __FUNCTION__ 
	 " No driver found in plugin [%s].\n", fname);
    goto error;
  }
  
  switch (d->type) {
  case INP_DRIVER:
  case EXE_DRIVER:
  case OBJ_DRIVER:
  case VIS_DRIVER:
    {
      any_driver_t * d2;

      dbglog(DBG_DEBUG, "** " __FUNCTION__
	     " : Driver list in [%s]\n", fname);
      for (d2=d; d2; d2=d2->nxt) {
	d2->dll = prog;
	++prog->ref_count;
	dbglog(DBG_DEBUG, " + [%s] [%s]\n", (char *)&d2->type, d2->name);
      }
    } break;
  default:
    dbglog(DBG_ERROR, "!! " __FUNCTION__ 
	   "Bad driver type %08x [%c%c%c%c]\n",
	   d->type,
	   (d->type&255), (d->type>>8)&255,
	   (d->type>>16)&255,  (d->type>>24)&255);
    d = 0;
  }


error:
  if (!d && prog) {
    lef_free(prog);
  }
  dbglog(DBG_DEBUG, "<< " __FUNCTION__
    " : %s\n", d ? "OK" : "FAILED");
  return d;
}

static int r_plugin_path_load(char *path, unsigned int level)
{
  dirent_t *de;
  int count = 0;
  int fd = 0;
  char *path_end = 0;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%2d,[%s])\n", level, path);

  if (level == 0) {
    goto error;
  }


  fd = fs_open(path, O_RDONLY | O_DIR);
  if (!fd) {
    count = -1;
    goto error;
  }

  path_end = path + strlen(path);
  path_end[0] = '/';
  path_end[1] = 0;

  while (de = fs_readdir(fd), de) {
    any_driver_t *d, *driver, *nxt;
    int type;

    type = filetype_get(de->name, de->size);

/*     dbglog(DBG_DEBUG, "** size=%08x [%s] type=%08x\n", */
/* 	   de->size, de->name, type); */

    if (type == FILETYPE_DIR) {
      int cnt;
      strcpy(path_end+1, de->name);
      cnt = r_plugin_path_load(path, level-1);
      count += (cnt > 0) ? cnt : 0;
      continue;
    }

    if (type != FILETYPE_LEF) {
      continue;
    }

    strcpy(path_end+1, de->name);
    driver = plugin_load(path);
    if (!driver) {
      continue;
    }

    for (d=driver; d; d = nxt) {
      driver_list_t *dl = 0;
      
      /* Save nxt here, It will be crashed by driver_list */
      nxt = d->nxt;
      switch (d->type) {
      case OBJ_DRIVER:
	dl = &obj_drivers;
	break;
      case VIS_DRIVER:
	dl = &vis_drivers;
	break;
      case INP_DRIVER:
	dl = &inp_drivers;
	break;
      case EXE_DRIVER:
	dl = &exe_drivers;
	break;
      default:
	dbglog(DBG_ERROR, "!! " __FUNCTION__
	       " : Unexecpected plugin type !\n");
      }

      if (!dl) {
	/* Error !! Let's run away !! */
	break;
      }

      if (d->init(d) < 0 || driver_list_register(dl, d) < 0) {

	dbglog(DBG_DEBUG, "++ [%s] INIT OR REGISTRATION FAILED\n", d->name);
	/* If the registration failed, let's free the dll. */
	lef_free((lef_prog_t *)d->dll);
      } else {
	dbglog(DBG_DEBUG, "++ [%s] added to [%s]\n", d->name, dl->name);
	++count;
      }
    }
  }

 error:
  if (fd) {
    fs_close(fd);
  }
  if (path_end) {
    *path_end = 0;
  }
  dbglog(DBG_DEBUG, "<< " __FUNCTION__ "(%2d,[%s]) = %d\n",
	 level, path, count);
  return count;
}

int plugin_path_load(const char *path, int max_recurse)
{
  char rpath[2048];

  max_recurse -= !max_recurse;
  strcpy(rpath, path);
  return r_plugin_path_load(rpath, max_recurse);
}
