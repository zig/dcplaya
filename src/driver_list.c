/**
 * @ingroup dcplaya_devel
 * @file    driver_list.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002
 * @brief   Registered driver list.
 *
 * $Id: driver_list.c,v 1.17 2003-03-04 15:26:52 ben Exp $
 */

#include <string.h>

#include "driver_list.h"
#include "inp_driver.h"
#include "img_driver.h"
#include "filename.h"
#include "filetype.h"
#include "sysdebug.h"
#include "lef.h"

/* $$$ ben : Should be remove to use clean interface */
driver_list_t inp_drivers;
driver_list_t obj_drivers;
driver_list_t exe_drivers;
driver_list_t vis_drivers;
driver_list_t img_drivers;

static spinlock_t driver_lists_mutex;
static driver_list_reg_t * driver_lists;

static void fake_shutdown(driver_list_reg_t * dl)
{
  SDNOTICE("Shutdowning driver list type [%s]\n",dl->name);
}

static driver_list_reg_t registered_lists[] = {
  { 0, "inp", INP_DRIVER, &fake_shutdown, &inp_drivers },
  { 0, "obj", OBJ_DRIVER, &fake_shutdown, &obj_drivers },
  { 0, "exe", EXE_DRIVER, &fake_shutdown, &exe_drivers },
  { 0, "vis", VIS_DRIVER, &fake_shutdown, &vis_drivers },
  { 0, "img", IMG_DRIVER, &fake_shutdown, &img_drivers },
};


static driver_list_reg_t * lists_search(driver_list_reg_t * lists,
					int type,
					const char * name)
{
  driver_list_reg_t * l;

  for (l=lists; l; l=l->next) {
    if (type && type == l->type) {
      break;
    }
    if (name && l->list && !stricmp(l->list->name, name)) {
      break;
    }
  }
  if (l) {
    driver_list_lock(l->list);
  }
  return l;
}

driver_list_reg_t * driver_lists_lock(void)
{
  spinlock_lock(&driver_lists_mutex);
  return driver_lists;
}

void driver_lists_unlock(driver_list_reg_t * reg)
{
  if (reg == driver_lists) {
    spinlock_unlock(&driver_lists_mutex);
  }
}

int driver_lists_add(driver_list_reg_t * reg)
{
  int err = -1;
  driver_list_reg_t * lists;

  /* Check parameter */
  if (!reg || !reg->type || !reg->name || !reg->shutdown || !reg->list) {
      SDERROR("[%s] : bad parameters\n", __FUNCTION__);
    return -1;
  }
  reg->next = 0;

  SDDEBUG("[%s] : adding [%s,%08x] to registered driver type list.\n",
	  __FUNCTION__, reg->name, reg->type);

  lists = driver_lists_lock();
  if (lists == driver_lists) {
    driver_list_reg_t * l, * p;

    /* Cannot add a list with the same name or the same type. */
    l = lists_search(lists,reg->type, reg->name);
    if (l) {
      SDERROR("[%s] : [%s,%08x] already exists as [%s,%08x].\n",
	      __FUNCTION__, reg->name, reg->type, l->name, l->type);
      driver_list_unlock(l->list);
    } else {
      /* Find last. */
      for (p=0,l=lists; l; p=l, l=l->next)
	;

      if (!p) {
	driver_lists = lists = reg;
	SDDEBUG("[%s] : [%s,%08x] has been registered as first\n",
		__FUNCTION__, reg->name, reg->type);
      } else {
	p->next = reg;
	SDDEBUG("[%s] : [%s,%08x] has been registered after [%s,%08x]\n",
		__FUNCTION__, reg->name, reg->type, p->name, p->type);
      }
      err = 0;
    }
  }
  driver_lists_unlock(lists);
  return err;
}

int driver_lists_remove(driver_list_reg_t * reg)
{
  if (reg) {
    SDERROR("[%s] : not implemented :(\n",__FUNCTION__);
    return -1;
  }
  return 0;
}

/* Initialize a driver list. */
static void list_init(driver_list_t *dl, const char *name)
{
  spinlock_init(&dl->mutex);
  dl->n = 0;
  dl->drivers = 0;
  dl->name = name;
}

/** Init everything for the driver manager. */
int driver_lists_init(void)
{
  int i, err = 0;
  const int n = sizeof(registered_lists) / sizeof(*registered_lists);

  spinlock_init(&driver_lists_mutex);
  driver_lists = 0;
  for (i=0; i<n; ++i) {
    driver_list_reg_t * reg = registered_lists + i;

    if (driver_lists_add(reg) < 0) {
      SDERROR("[%s] : [%s,%08x] registration failed.\n",
	      __FUNCTION__, reg->name, reg->type);
      ++ err;
    } else {
      /* $$$ ben : careful ! driver list will not been initiliazed if
	 the registration failed. This couls be dangerous since we used
	 extern driver list. This could be ok as soon as these extern be
	 removed.
      */
      list_init(reg->list, reg->name);
      SDDEBUG("[%s] : [%s,%08x] initialized.\n",
	      __FUNCTION__, reg->name, reg->type);
    }
  }
  return -err;
}

/** Shutddown all driver list */
void driver_lists_shutdown(void)
{
  driver_list_reg_t * reg, * next;

  for (reg=driver_lists_lock(); reg; reg = next) {
    next = reg->next;
    driver_list_shutdown(reg->list);
  }
  driver_lists = 0;
  SDDEBUG("[driver_list : all lists shutdowned\n");
}

/** Shutdown driver list : $$$ ben : we need to do thing here !! */
void driver_list_shutdown(driver_list_t *dl)
{
  if (dl) {
    spinlock_lock(&dl->mutex);
    SDDEBUG("[%s] : shutdown list [%s]\n", __FUNCTION__, dl->name);
    if (dl->n) {
      SDDEBUG("[%s] : %d remaining driver in [%s]\n",
	      __FUNCTION__, dl->n, dl->name);
    }
  }
}

void driver_list_lock(driver_list_t *dl) {
  if (dl) {
    spinlock_lock(&dl->mutex);
  }
}

void driver_list_unlock(driver_list_t *dl) {
  if (dl) {
    spinlock_unlock(&dl->mutex);
  }
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
