/**
 * dreammp3 - Plugin loader
 *
 * (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: plugin.c,v 1.11 2002-12-15 16:15:03 ben Exp $
 */
#include <stdio.h>
#include <string.h>
#include <kos/fs.h>

#include "plugin.h"

#include "any_driver.h"
#include "driver_list.h"
#include "lef.h"
#include "filetype.h"
#include "sysdebug.h"

any_driver_t * plugin_load(const char *fname)
{
  lef_prog_t * prog = 0;
  any_driver_t *d = 0, *d2 = 0, *pd = 0, *d1 = 0;

  SDDEBUG(">> %s(%s)\n", __FUNCTION__, fname);
  SDINDENT;

  d = 0;
  prog = lef_load(fname);

  if (!prog) {
    SDERROR("Plugin [%s] load error\n", fname);
    goto error;
  }

  SDDEBUG("Plugin load success [%s].\n", fname);
  d = (any_driver_t *)prog->main(0, 0);
  if (!d) {
    SDWARNING("No driver found in plugin [%s].\n", fname);
    goto error;
  }
  /* Attach dll to driver. */
  d->dll = prog;

  SDDEBUG("Driver list in [%s]\n", fname);

  sysdbg_indent(1, 0);
  for (d1 = pd = 0, d2 = d; d2; d2 = d2->nxt) {
    driver_list_t *dl;

    dl = driver_list_which(d2);
    if (dl) {
      /* Have a valid driver. */
      if (pd) {
		/* Link it to previous valid driver. */
		pd->nxt = d2;
      } else {
		/* Or set first valid. */
		d1 = d2;
      }
      pd = d2;
      /* Add a ref count to lef. */
      /*      ++prog->ref_count;*/
      SDDEBUG("+ [%s] [%s] -> [%s]\n", (char *)&d2->type, d2->name, dl->name);
    } else {
      SDERROR("Bad driver type %08x [%c%c%c%c]\n",
			  d->type,
			  (d->type&255), (d->type>>8)&255,
			  (d->type>>16)&255,  (d->type>>24)&255);
    }
  }
  SDUNINDENT;

  /* Close the list. */ 
  if (pd) {
    pd->nxt = 0;
  }

 error:
  if (!d1) {
    /* No valid driver has be*/
    lef_free(prog);
  }
  sysdbg_indent(-1,0);
  SDDEBUG("<< %s() = %s\n", __FUNCTION__, d1 ? "OK" : "FAILED");
  
  return d1;
}

static void remove_driver(driver_list_t * dl, any_driver_t * d)
{
  lef_prog_t * lef = (lef_prog_t *) d->dll;

  SDDEBUG("%s([%s], [%s] : lef:%p count:%d\n", __FUNCTION__,
		  dl->name, d->name, lef, lef ? lef->ref_count : -666);
  driver_list_unregister(dl, d);
  SDDEBUG("Dereferencing driver\n");
  driver_dereference(d);
}

int plugin_load_and_register(const char *fname)
{
  lef_prog_t * lef;
  any_driver_t *d, *driver, *nxt, *old;
  int count = 0;
    
  driver = plugin_load(fname);
  if (!driver) {
    return 0;
  }
  
  /* We can set the lef here since it is the same for all this drivers */
  lef = (lef_prog_t *) d->dll;

  SDDEBUG("Scanning lef [%s] driver list\n", fname);
  SDINDENT;
  for (d=driver; d; d = nxt) {
    driver_list_t *dl;

    SDDEBUG("scan [%p %s]\n", d, d->name);

    /* Save nxt here, It will be crashed by driver_list */
    nxt = d->nxt;

    dl = driver_list_which(d);
    if (!dl) {
      SDERROR("Unexpected plugin type !\n");
      continue;
    }

    /* Remove driver with the same name if it exists. */
    old = driver_list_search(dl, d->name);
    if (old) {
      SDDEBUG("Existing driver [%p %s] : [%p %s]\n",
			  d, d->name, old, old->name);
      remove_driver(dl, old);
    }

    if (driver_reference(d) < 0 || driver_list_register(dl, d) < 0) {
      SDERROR( "[%s] Init or Registration failed, removed\n", d->name);
      remove_driver(dl, d);
    } else {
      SDDEBUG("++ [%s] added to [%s]\n", d->name, dl->name);
      ++count;
    }
  }
  SDUNINDENT;

  return count;
}

extern int filetype_lef;

static int r_plugin_path_load(char *path, unsigned int level)
{
  dirent_t *de;
  int count = 0;
  int fd = 0;
  char *path_end = 0;

  SDDEBUG(">> %s(%2d,[%s])\n", __FUNCTION__, level, path);
  SDINDENT;

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
    int type;

    type = filetype_get(de->name, de->size);
/* 	SDDEBUG("[%s] : %04x\n", de->name, type); */

    if (type == filetype_dir) {
      int cnt;
      strcpy(path_end+1, de->name);
      cnt = r_plugin_path_load(path, level-1);
      count += (cnt > 0) ? cnt : 0;
      continue;
    }

    if (type != filetype_lef) {
      continue;
    }

    strcpy(path_end+1, de->name);
    count += plugin_load_and_register(path);
  }

 error:
  if (fd) {
    fs_close(fd);
  }
  if (path_end) {
    *path_end = 0;
  }
  
  SDUNINDENT;
  SDDEBUG("<< %s(%2d,[%s]) = %d\n", __FUNCTION__, level, path, count);

  return count;
}

int plugin_path_load(const char *path, int max_recurse)
{
  char rpath[2048];

  max_recurse -= !max_recurse;
  strcpy(rpath, path);
  return r_plugin_path_load(rpath, max_recurse);
}
