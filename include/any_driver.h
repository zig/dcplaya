/*
 * @ingroup dcplaya_plugin_devel
 * @file    any_driver.h
 * @author  benjamin gerard
 * @brief   dcplaya plugin structure.
 *
 * $Id: any_driver.h,v 1.11 2003-03-26 23:02:47 ben Exp $ 
 */
 
#ifndef _ANY_DRIVER_H_
#define _ANY_DRIVER_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/* @addtogroup  dcplaya_plugin_devel  Plugins
 *  @ingroup     dcplaya_devel
 *  @author      benjamin gerard
 *  @brief       dcplaya plugins
 *  @{
 */

/** @name Driver types.
 *  @{
 */
#define OBJ_DRIVER  'JBO' /**< 3D object         */
#define VIS_DRIVER  'SIV' /**< Visual plugin     */
#define INP_DRIVER  'PNI' /**< Input plugin      */
#define EXE_DRIVER  'EXE' /**< Executable plugin */
#define IMG_DRIVER  'GMI' /**< Image plugin      */
/**@}*/

#include "driver_option.h"

struct luashell_command_description;

/** Shared by any driver.
 *  @ingroup dcplaya_plugin_devel
 */
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

  /** Authors list. */
  const char *authors;

  /** Driver short description. */
  const char *description;

  /** DLL handler (currently the pointer to lef_prog_t. */
  void *dll;
  
  /** Driver first init. */
  int (*init)(struct _any_driver_s *);

  /** Driver shutdown (clean-up). */ 
  int (*shutdown)(struct _any_driver_s *);

  /** Get/Set driver options. */
  driver_option_t * (*options)(struct _any_driver_s *,
                               int idx, driver_option_t * opt);

  /** Lua shell command description list */
  struct luashell_command_description * luacommands;



  /* WARNING WARNING START OF DYNAMIC VARIABLES */
  /* THE FOLLOWING ENTRIES SHOULD BE INITIALLY SET TO ZERO */
  /* NOTE : the dll entry should also be here ... */

  /** Reference counter */
  int count;

  /** Mutex set during modification of driver state */
  void * mutex;

} any_driver_t;



/** These #define *MUST* be use to fill the any_driver_t.nxt field. It allows
 * to build a correct linked-list at compile time for a single multi-driver
 * plugin (.lef) file.
 */
#if !defined(FIRST_DRIVER) || (FIRST_DRIVER)
# define EXPORT_DRIVER(symbol) \
    any_driver_t *lef_main(void) { \
      return (any_driver_t *)&symbol; \
    }
#else
# define EXPORT_DRIVER(symbol)
#endif

#if !defined(NXT_DRIVER) || (NXT_DRIVER)
# define NEXT_DRIVER 0
#else
# define NEXT_DRIVER &NXT_DRIVER
  extern any_driver_t NXT_DRIVER; 
#endif

/**@}*/

DCPLAYA_EXTERN_C_END

#endif

