/**
 * @ingroup dcplaya_devel
 * @file    driver_list.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002
 * @brief   Registered driver list.
 *
 * $Id: driver_list.c,v 1.16 2003-03-03 13:01:27 ben Exp $
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

driver_list_reg_t * driver_lists;

static driver_list_reg_t registered_lists[] = {
  { 0, "inp", INP_DRIVER, &inp_drivers },
  { 0, "obj", OBJ_DRIVER, &obj_drivers },
  { 0, "exe", EXE_DRIVER, &exe_drivers },
  { 0, "vis", VIS_DRIVER, &vis_drivers },
  { 0, "img", IMG_DRIVER, &img_drivers },
};

/** Initialize a driver list */
int driver_list_init(driver_list_t *dl, const char *name)
{
  SDDEBUG("[driver_list] : init list [%s]\n", name);
  spinlock_init(&dl->mutex);
  dl->n = 0;
  dl->drivers = 0;
  dl->name = name;
  return 0;
}

/** Init all driver list */
int driver_list_init_all()
{
  int i, err = 0;
  const int n = sizeof(registered_lists) / sizeof(*registered_lists);

  driver_lists = 0;
  for (i=0; i<n; ++i) {
    driver_list_reg_t * reg = registered_lists + i;
    if (!driver_lists) {
      driver_lists = reg;
    }
    err |= driver_list_init(reg->list, reg->name);
    reg->next = (i == n-1) ? 0 :  reg+1;
  }

  return err;
}

/** Shutddown all driver list */
void driver_list_shutdown_all(void)
{
  driver_list_reg_t * reg, * next;

  for (reg=driver_lists; reg; reg=next) {
    next = reg->next;
    driver_list_shutdown(reg->list);
  }
  driver_lists = 0;
  SDDEBUG("[driver_list : all lists shutdowned\n");
}

/** Shutdown driver list : nothing to do ! */
void driver_list_shutdown(driver_list_t *dl)
{
  spinlock_lock(&dl->mutex);
  SDDEBUG("[driver_list] : shutdown list [%s]\n", dl->name);
}

void driver_list_lock(driver_list_t *dl) {
  spinlock_lock(&dl->mutex);
}

void driver_list_unlock(driver_list_t *dl) {
  spinlock_unlock(&dl->mutex);
}

static any_driver_t * drv_list_search(driver_list_t *dl, const char *name);


static int specific_register(any_driver_t * driver)
{
  int err = 0;
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
      /* Add to translator */
      err = AddTranslator(d->translator);
      if (!err) {
	/* Add filetype. */
	d->id = filetype_add(filetype_major_add("image"),
			     driver->name, d->extensions);
	SDDEBUG("Driver '%s' : filetype %d\n", driver->name, d->id);
      }
    } break;
  }
  return err;
}

static int specific_unregister(any_driver_t * driver)
{
  /* driver type specific */
  switch(driver->type) {
  case INP_DRIVER:
    filetype_del(((inp_driver_t *)driver)->id);
    break;
  case IMG_DRIVER:
    filetype_del(((img_driver_t *)driver)->id);
    DelTranslator(((img_driver_t *)driver)->translator);
    break;
  }
}


int driver_list_register(driver_list_t *dl, any_driver_t * driver)
{
  const any_driver_t * d;
  int err = 0;

  if (!dl || !driver) {
    return -1;
  }

  driver_list_lock(dl);

  d = drv_list_search(dl, driver->name);
  if (d) {
    SDERROR("%s([%s], [%s]) : Driver already exists.\n",
	    __FUNCTION__, dl->name, driver->name);
    err = -1;
  } else {
    err = driver_reference(driver);
    if (!err) {
      driver->nxt = dl->drivers;
      dl->drivers = driver;
      ++dl->n;
    }
  }
  driver_list_unlock(dl);

  SDDEBUG("[%s] := [%d]\n",__FUNCTION__, err);

  return err;
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

  if (!dl || !driver) {
    return -1;
  }

  driver_list_lock(dl);

  for (p=0, d=dl->drivers; d && d != driver; p=d, d=d->nxt)
    ;

  if (d) {
    if (p) {
      p->nxt = d->nxt;
    } else {
      dl->drivers = d->nxt;
    }
    --dl->n;
    SDDEBUG("Unregister [%s] from [%s] success\n", driver->name, dl->name);
    specific_unregister(d);
    driver_dereference(d);
  } else {
    SDERROR("Failed to unregister [%s] from [%s].\n", driver->name, dl->name);
  }

  driver_list_unlock(dl);

  return d ? 0 : -1;
}

static any_driver_t * drv_list_search(driver_list_t *dl, const char *name)
{
  any_driver_t * d;
  for (d=dl->drivers; d && strcmp(d->name, name); d=d->nxt)
    ;
  return d;
}

static any_driver_t * drv_list_idx(driver_list_t *dl, int idx)
{
  any_driver_t * d;
  int i;
  for (i=0, d=dl->drivers; d && idx != i; d=d->nxt, ++i)
    ;
  return d;
}

any_driver_t * driver_list_search(driver_list_t *dl, const char *name)
{
  any_driver_t * d;

  if (!dl || !name) {
    return 0;
  }

  driver_list_lock(dl);
  d = drv_list_search(dl, name);
  driver_reference(d);
  driver_list_unlock(dl);

  return d;
}

any_driver_t * driver_list_index(driver_list_t *dl, int idx)
{
  any_driver_t * d;

  if (!dl) {
    return 0;
  }

  driver_list_lock(dl);
  d = drv_list_idx(dl, idx);
  driver_reference(d);
  driver_list_unlock(dl);

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

  driver_list_lock(&inp_drivers);
  for (d=(inp_driver_t *)inp_drivers.drivers;
       d && !extfind(d->extensions, ext);
       d=(inp_driver_t *)d->common.nxt)
    ;
  driver_reference(&d->common);
  driver_list_unlock(&inp_drivers);

  return d;
}

inp_driver_t * inp_driver_list_search_by_id(int id)
{
  inp_driver_t * d;

  driver_list_lock(&inp_drivers);
  for (d=(inp_driver_t *)inp_drivers.drivers;
       d && d->id != id;
       d=(inp_driver_t *)d->common.nxt)
    ;
  driver_reference(&d->common);
  driver_list_unlock(&inp_drivers);

  return d;
}

any_driver_t * driver_lock(any_driver_t * drv)
{
  if (drv) {
    spinlock_lock((spinlock_t *) &drv->mutex);
    ++drv->count;
  }
  return drv;
}

void driver_unlock(any_driver_t * drv)
{
  if (drv) {
    --drv->count;
    spinlock_unlock((spinlock_t *) &drv->mutex);
  }
}

int driver_reference(any_driver_t * drv)
{
  lef_prog_t * lef;
  int result = 0;

  if (!drv) return -1;

  spinlock_lock((spinlock_t *) &drv->mutex);
  drv->count++;
  if (drv->count == 1) {
    SDDEBUG("Calling init on driver '%s'\n", drv->name);
    result = drv->init(drv);
    if (!result) {
      result = specific_register(drv);
    } else {
      SDERROR("[%s] : init failed [%d]\n", drv->name, result);
    }
  }
  lef = drv->dll;
  if (lef) {
    lef->ref_count++;
  }
  spinlock_unlock((spinlock_t *) &drv->mutex);

  if (result) {
    driver_dereference(drv);
  }

  return result;
}

void driver_dereference(any_driver_t * drv)
{
  lef_prog_t * lef;

  if (!drv) return;

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
