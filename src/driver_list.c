/**
 * @ingroup dcplaya_devel
 * @file    driver_list.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002
 * @brief   Registered driver list.
 *
 * $Id: driver_list.c,v 1.11 2002-12-14 16:15:36 ben Exp $
 */

#include <string.h>

#include "driver_list.h"
#include "inp_driver.h"
#include "img_driver.h"
#include "filename.h"
#include "filetype.h"
#include "sysdebug.h"
#include "lef.h"

driver_list_t inp_drivers;
driver_list_t obj_drivers;
driver_list_t exe_drivers;
driver_list_t vis_drivers;
driver_list_t img_drivers;

/** Initialize a driver list */
int driver_list_init(driver_list_t *dl, const char *name)
{
  SDDEBUG("%s(%s)\n", __FUNCTION__, name);
  dl->n = 0;
  dl->drivers = 0;
  dl->name = name;
  return 0;
}

/** Init all driver list */
int driver_list_init_all()
{
  int err = 0;
  err |= driver_list_init(&inp_drivers, "inp");
  err |= driver_list_init(&obj_drivers, "obj");
  err |= driver_list_init(&exe_drivers, "exe");
  err |= driver_list_init(&vis_drivers, "vis");
  err |= driver_list_init(&img_drivers, "img");

  return err;
}

/** Shutddown all driver list */
void driver_list_shutdown_all(void)
{
  driver_list_shutdown(&vis_drivers);
  driver_list_shutdown(&exe_drivers);
  driver_list_shutdown(&obj_drivers);
  driver_list_shutdown(&inp_drivers);
}

/** Shutdown driver list : nothing to do ! */
void driver_list_shutdown(driver_list_t *dl)
{
}

int driver_list_register(driver_list_t *dl, any_driver_t * driver)
{
  const any_driver_t * d;

  if (!dl || !driver) {
    return -1;
  }
  d = driver_list_search(dl, driver->name);
  if (d) {
    SDERROR("%s([%s], [%s]) : Driver already exists.\n",
	    __FUNCTION__, dl->name, driver->name);
    return -1;
  }
  driver->nxt = dl->drivers;
  dl->drivers = driver;
  ++dl->n;

  /* $$$ This test should not be here. Drivers have nothing to do with
     file type. */
  switch(driver->type) {
  case INP_DRIVER:
	{
	  inp_driver_t * d = (inp_driver_t *) driver;
	  d->id = filetype_add(filetype_major_add("music"),
						   driver->name, d->extensions);
	  SDDEBUG("Driver '%s' : filetype %d\n", driver->name, d->id);
	} break;
	
  case IMG_DRIVER:
	{
	  img_driver_t * d = (img_driver_t *) driver;
	  d->id = filetype_add(filetype_major_add("image"),
						   driver->name, d->extensions);
	  SDDEBUG("Driver '%s' : filetype %d\n", driver->name, d->id);
	} break;
  }

  return 0;
}

driver_list_t * driver_list_which(any_driver_t *driver)
{
  if (!driver) {
    return 0;
  }
  switch(driver->type) {
  case OBJ_DRIVER:
    return &obj_drivers;
  case VIS_DRIVER:
    return &vis_drivers;
  case INP_DRIVER:
    return &inp_drivers;
  case EXE_DRIVER:
    return &exe_drivers;
  case IMG_DRIVER:
    return &img_drivers;
  default:
    return 0;
  }
}

int driver_register(any_driver_t * driver)
{
  return driver_list_register(driver_list_which(driver), driver); 
}

int driver_unregister(any_driver_t * driver)
{
  return driver_list_unregister(driver_list_which(driver), driver); 
} 

/** Remove a driver from the driver list */
int driver_list_unregister(driver_list_t *dl, any_driver_t * driver)
{
  any_driver_t *d, *p=0;

  if (!dl) {
    return -1;
  }

  /* driver type specific */
  switch(driver->type) {
  case INP_DRIVER:
	filetype_del(((inp_driver_t *)driver)->id);
	break;
  case IMG_DRIVER:
	filetype_del(((img_driver_t *)driver)->id);
	break;
  }

  for (p=0, d=dl->drivers; d && d != driver; p=d, d=d->nxt)
    ;
  if (d) {
    if (p) {
      p->nxt = d->nxt;
    } else {
      dl->drivers = d->nxt;
    }
    --dl->n;
  }
  if (!d) {
    SDERROR("Failed to unregister [%s] from [%s].\n", driver->name, dl->name);
  } else {
    SDDEBUG("Unregister [%s] from [%s] failed\n", driver->name, dl->name);
  }

  return d ? 0 : -1;
}

any_driver_t * driver_list_search(driver_list_t *dl, const char *name)
{
  any_driver_t * d;

  for (d=dl->drivers; d && strcmp(d->name, name); d=d->nxt)
    ;
  return d;
}

/***************************************************************
 ** Input driver 
 **************************************************************/

static int extfind(const char * extlist, const char * ext)
{
  if (extlist && ext) {
	int len;
	while (len = strlen(extlist), len > 0) {
	  if (!stricmp(ext,extlist)) {
		return 1;
	  }
	  extlist += len+1;
	}
  }
  return 0;
}

inp_driver_t * inp_driver_list_search_by_extension(const char *ext)
{
  inp_driver_t * d;

  /* Not an extension : get it */
  if (ext && *ext != '.') {
    ext = fn_secondary_ext(ext,".gz");
  }
  if (!ext) {
	return 0;
  }

  for (d=(inp_driver_t *)inp_drivers.drivers;
       d && !extfind(d->extensions, ext);
       d=(inp_driver_t *)d->common.nxt)
    ;
  return d;
}

inp_driver_t * inp_driver_list_search_by_id(int id)
{
  inp_driver_t * d;

  for (d=(inp_driver_t *)inp_drivers.drivers;
       d && d->id != id;
       d=(inp_driver_t *)d->common.nxt)
    ;
  return d;
}

#include <arch/spinlock.h>

int driver_reference(any_driver_t * drv)
{
  lef_prog_t * lef;
  int result = 0;

  spinlock_lock((spinlock_t *) &drv->mutex);

  drv->count++;
  if (drv->count == 1) {
    SDDEBUG("Calling init on driver '%s'\n", drv->name);
    result = drv->init(drv);
  }

  lef = drv->dll;
  if (lef) {
    lef->ref_count++;
  }

  spinlock_unlock((spinlock_t *) &drv->mutex);

  return result;
}

void driver_dereference(any_driver_t * drv)
{
  lef_prog_t * lef;

  spinlock_lock((spinlock_t *) &drv->mutex);

  drv->count--;
  if (drv->count == 0) {
    SDDEBUG("Calling shutdown on driver '%s'\n", drv->name);
    drv->shutdown(drv);
  }

  lef = drv->dll;
  if (lef) {
    int count = drv->count;
    lef_free(lef);

    /* past this point, the driver may not exist anymore ... */
    if (count == 0)
      return; /* do not release mutex ON PURPOSE */
  }

  spinlock_unlock((spinlock_t *) &drv->mutex);
}
