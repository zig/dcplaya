/*
 *
 * $Id: any_driver.h,v 1.1 2002-08-26 14:15:00 ben Exp $ 
 */
 
#ifndef _ANY_DRIVER_H_
#define _ANY_DRIVER_H_

/* Driver types */
#define OBJ_DRIVER  'JBO' /**< 3D object         */
#define VIS_DRIVER  'SIV' /**< Visual plugin     */
#define INP_DRIVER  'PNI' /**< Input plugin      */
#define EXE_DRIVER  'EXE' /**< Executable plugin */

#include "driver_option.h"

/** Shared by any driver */
typedef struct _any_driver_s
{
  /** Next driver in list. */
  struct _any_driver_s * nxt;

  /** Driver type. */
  int type;
  
  /** Driver version number. MAJOR * 256 + MINOR */
  int version;

  /** Driver unic name (for each type). */
  const char *name;

  /** Authors list */
  const char *authors;

  /** Driver short description */
  const char *description;

  /** DLL handler (currently the pointer to lef_prog_t. */
  void *dll;
  
  /** Driver first init. */
  int (*init)(struct _any_driver_s *);

  /** Driver shutdown (clean-up). */ 
  int (*shutdown)(struct _any_driver_s *);

  /** Get/Set driver options */
  driver_option_t * (*options)(struct _any_driver_s *,
                               int idx, driver_option_t * opt);

} any_driver_t;

/* These #define *MUST* be use to fill the any_driver_t.nxt field. It allows
 * to build a correct linked-list at compile time for a single multi-driver
 * plugin (.klf) file.
 */
#if !defined(FIRST_DRIVER) || (FIRST_DRIVER)
/* # warning "EXPORT_DRIVER include ko_main" */
# define EXPORT_DRIVER(symbol) \
    any_driver_t *ko_main(void) { \
      return (any_driver_t *)&symbol; \
    }
#else
/* # warning "EXPORT_DRIVER empty" */
# define EXPORT_DRIVER(symbol)
#endif

#if !defined(NXT_DRIVER) || (NXT_DRIVER)
# define NEXT_DRIVER 0
#else
# define NEXT_DRIVER &NXT_DRIVER
  extern any_driver_t NXT_DRIVER; 
#endif

#endif
