/**
 * @ingroup dcplaya_driverlist_devel
 * @file    driver_list.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002
 * @brief   Registered driver list.
 *
 * $Id: driver_list.h,v 1.14 2003-03-22 00:35:26 ben Exp $
 */

#ifndef _DRIVER_LIST_H_
#define _DRIVER_LIST_H_

#include <arch/spinlock.h>

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup  dcplaya_driverlist_devel  Driver and driver type
 *  @ingroup   dcplaya_devel
 *  @brief     Driver and driver type
 *
 *  @author    benjamin gerard <ben@sashipa.com>
 */

#include "any_driver.h"
#include "inp_driver.h"

/** Any driver list.
 * @ingroup dcplaya_driverlist_devel
 */
typedef struct
{
  const char *name;       /**< Name of driver list.      */
  int n;                  /**< Number of driver in list. */
  spinlock_t mutex;       /**< Thread safety.            */
  any_driver_t * drivers; /**< First entry.              */
} driver_list_t;

/** Registered driver type.
 *  @ingroup dcplaya_driverlist_devel
 */
typedef struct _driver_list_reg_s {
  struct _driver_list_reg_s * next; /**< Next registered driver type. */
  const char * name;                /**< Name of driver list.         */
  int type;                         /**< Type id.                     */
  /** Shutdown function. This function must do everything when a driver list
      is unregistred. This include freeing memory, removing filetypes ...
      When this function is called, the driver list is locked.
  */
  void (*shutdown)(struct _driver_list_reg_s * driver_list);
  driver_list_t * list;             /**< Driver list.                 */
} driver_list_reg_t;

/** @name Global driver lists.
 *  @ingroup dcplaya_driverlist_devel
 *  @{
 *  @todo Remove this. Use clean driver_lists api instead.
 */

/** List of input driver. */
extern driver_list_t inp_drivers;
/** List of object driver. */
extern driver_list_t obj_drivers;
/** List of visual driver. */
extern driver_list_t vis_drivers;
/** List of executable driver. */
extern driver_list_t exe_drivers;
/** List of image driver. */
extern driver_list_t img_drivers;

/* Table of all driver lists. */
/*extern driver_list_reg_t * driver_lists;*/

/*@}*/


/** @name Driver lists management.
 *  @ingroup dcplaya_driverlist_devel
 *
 *  Driver lists referes to the list all driver type available.
 *  User can add new driver type (e.g a video driver type should be welcome.)
 *
 *  @{
 */

/** Init all driver lists. */
int driver_lists_init(void);

/** Shutdown all driver lists. */
void driver_lists_shutdown(void);

/** Add a new driver type. */
int driver_lists_add(driver_list_reg_t * reg);

/** Remove a driver type. */
int driver_lists_remove(driver_list_reg_t * reg);

/** Lock and get a pointer to the list of regitered driver type. */
driver_list_reg_t * driver_lists_lock(void);

/** Unlock a previously locked list of registered driver type. */
void driver_lists_unlock(driver_list_reg_t * reg);

/* Initialize a driver list. */
/*int driver_list_init(driver_list_t * dl, const char *name);*/

/** Shutdown a driver list. */
void driver_list_shutdown(driver_list_t * dl);

/*@}*/


/** @name Driver list management.
 *  @ingroup dcplaya_driverlist_devel
 *  @{
 */

/** Lock a driver list. */
void driver_list_lock(driver_list_t *dl);

/** Unlock a driver list. */
void driver_list_unlock(driver_list_t *dl);

/** Add a driver into a specified driver list.
 *
 *    The driver_list_register() function adds a driver to a given driver list.
 *    If a driver with the same name already exist in the driver list, the
 *    driver_list_register() function failed. This function does some specific
 *    operations depending on the driver type. Currently these specific
 *    operations consist on filetype creation for input and image drivers.
 *
 *  @return error-code
 *  @retval 0, on success
 */
int driver_list_register(driver_list_t * dl, any_driver_t * driver);

/** Add a driver into a suitable driver list.
 *
 *   The driver_register() function is short cut to :
 * @code
 * driver_list_register(driver_list_which(), driver);
 * @endcode
 *
 * @see driver_list_register()
 * @see driver_list_which()
 * @see driver_unregister()
 */
int driver_register(any_driver_t * driver);

/** Remove a driver into a specified driver list.
 *
 *    The driver_list_unregister() function removes a driver to a given driver
 *    list.
 *    If the dirver in not found an the list the function failed.
 *    This function does some specific operations depending on the driver type.
 *    Currently these specific operations consist on filetype removal for input
 *    and image drivers.
 *
 *  @return error-code
 *  @retval 0, on success
 */
int driver_list_unregister(driver_list_t * dl, any_driver_t * driver);

/** Remove a driver from a suitable driver list.
 *
 *   The driver_unregister() function is short cut to :
 * @code
 * driver_list_unregister(driver_list_which(), driver);
 * @endcode
 *
 * @see driver_list_unregister()
 * @see driver_list_which()
 * @see driver_register()
 */
int driver_unregister(any_driver_t * driver);

/** Find a suitable driver list for a driver.
 *
 *   The driver_list_which() function returns the driver list corresponding to
 *   the driver type.
 *
 * @return driver-list
 * @retval 0, on error
 */
driver_list_t * driver_list_which(any_driver_t *driver);

/** Search a driver in a driver list by its name.
 *
 * @return driver
 * @retval 0, on error
 */
any_driver_t * driver_list_search(driver_list_t * dl, const char *name);

/** Search a driver by index.
 *
 * @return driver
 * @retval 0, on error
 */
any_driver_t * driver_list_index(driver_list_t *dl, int idx);

/* @} */


/** @name Driver reference (instance) management.
 *  @ingroup dcplaya_driverlist_devel
 *  @{
 */

/** Add reference to a driver.
 *
 *   The driver_reference() function adds a reference to a driver. 
 *   If it is the first reference the driver init method is call.
 *
 *  @param  drv  driver to reference. It is safe to pass 0.
 *
 *  @return error-code
 *  @retval 0, on success
 *
 *  @see driver_dereference()
 */
int driver_reference(any_driver_t * drv);

/** Release one reference to a driver.
 *
 *   The driver_unreference() function removes a reference to a driver. 
 *   If it is the last reference the driver shutdown method is call and the
 *   driver is free.
 *
 *  @param  drv  driver to reference. It is safe to pass 0.
 *
 *  @see driver_reference()
 */
void driver_dereference(any_driver_t * drv);

/** Lock a already refernced driver and add reference to it.
 *
 *   The driver_lock() function locks the driver and adds a reference to it.
 *
 *  @param  drv  driver to reference. It is safe to pass 0.
 *
 *  @return pointer to the locked driver.
 *  @retval 0, on error (passed drv was null)
 *
 *  @see driver_unlock()
 *
 *  @warning Do not forget to unlock driver when your done with it.
 *  @warning Calling this function with unreferenced driver will not crash
 *           but no initialisation stuff is performed.
 *
 */
any_driver_t * driver_lock(any_driver_t * drv);

/** Unlock a previously locker driver and dereference it.
 *
 *   The driver_unlock() function unlocks the driver and dereference it.
 *
 *  @param  drv  driver to dereference. It is safe to pass 0.
 *
 *  @see driver_lock()
 *
 *  @warning Only call this function with a previously locked driver.
 *  @warning Calling this function with unreferenced driver will not crash
 *           but no shutdown stuff is performed.
 *
 */
void driver_unlock(any_driver_t * drv);


/*@}*/


/** @name Input driver specific.
 *  @ingroup dcplaya_driverlist_devel
 *  @{
 */

/** Search an input driver that match a file extension. */
inp_driver_t * inp_driver_list_search_by_extension(const char *ext);

/** Search an input driver that a id (filetype). */
inp_driver_t * inp_driver_list_search_by_id(int id);

/*@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _DRIVER_LIST_H_ */
