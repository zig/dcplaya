/*
*/

#include <string.h>

#include "driver_list.h"
#include "inp_driver.h"
#include "filetype.h"
#include "sysdebug.h"

driver_list_t inp_drivers;
driver_list_t obj_drivers;
driver_list_t exe_drivers;
driver_list_t vis_drivers;

static int file_type;

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
  file_type = FILETYPE_PLAYABLE;

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
  if (driver->type == INP_DRIVER) {
    ((inp_driver_t *)driver)->id = file_type++;
  }

/*   SDDEBUG("List [%s] : added #%d [%s]\n", dl->name, dl->n, dl->drivers->name); */

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

static int ToUpper(int c)
{
  if (c>='a' && c<='z') c += 'A'-'a';
  return c;
}

/** $$$ Ben: apparemant ya un bug ici : .lef_ est pris contre .lef !! */
static const char * extcmp(const char * extlist, const char * ext)
{
  int a,b;

  a = ToUpper(*extlist++);
  b = ToUpper(*ext++);
  while (a && a==b) {
    a = ToUpper(*extlist++);
    b = ToUpper(*ext++);
  }

  if (!a) {
    if (!b) {
      /* Both are null : found it */
      extlist = 0;
    }
  } else {
    while( *extlist++ )
      ;
  }
  return extlist;
}

static int extfind(const char *extlist, const char * ext)
{
  while (extlist && *extlist) {
    extlist = extcmp(extlist, ext);
  }
  return extlist == 0;
}

static const char * get_ext(const char *name)
{
  const char * e, * p;
  if (!name) {
    return 0;
  }
  e = strrchr(name,'.');
  p = strrchr(name,'/');
  if (e<p) {
    e = 0;
  } else if (ToUpper(e[1]) == 'G' && ToUpper(e[2]) == 'Z' && !e[3]) {
    /* .gz found, try to find secondary extension. */
    const char *e2;

    for (e2=e-1; e2 >= name && *e2 != '/'; --e2) {
      if (*e2 == '.') {
	e = e2;
	break;
      }
    }
  }

/*   SDDEBUG("%s([%s] := [%s]\n", __FUNCTION__, name, e); */

  return e;
}

inp_driver_t * inp_driver_list_search_by_extension(const char *ext)
{
  inp_driver_t * d;

  /* Not an extension : get it */
  if (ext && *ext != '.') {
    ext = get_ext(ext);
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

