/**
 * @ingroup dcplaya_devel
 * @file    driver_list.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002
 * @brief   Registered driver list.
 *
 * $Id: driver_list.h,v 1.9 2003-01-03 19:05:39 ben Exp $
 */

#ifndef _DRIVER_LIST_H_
#define _DRIVER_LIST_H_

#include <arch/spinlock.h>

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "any_driver.h"
#include "inp_driver.h"

/** Any driver list.
 * @ingroup dcplaya_devel
 */
typedef struct
{
  const char *name;       /**< Name of driver list.      */
  int n;                  /**< Number of driver in list. */
  spinlock_t mutex;       /**< Thread safety.            */
  any_driver_t * drivers; /**< First entry.              */
} driver_list_t;

/** @name Global driver lists.
 *  @ingroup dcplaya_devel
 *  @{
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

/*@}*/


/** @name Driver list initialization.
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Init all driver lists. */
int driver_list_init_all();

/** Shutdown all driver lists. */
void driver_list_shutdown_all(void);

/** Initialize a driver list. */
int driver_list_init(driver_list_t * dl, const char *name);

/** Shutdown a driver list. */
void driver_list_shutdown(driver_list_t * dl);

/*@}*/


/** @name Driver list management.
 *  @ingroup dcplaya_devel
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

/* @} */


/** @name Driver reference (instance) management.
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Add reference to a driver.
 *
 *   The driver_reference() function adds a reference to a driver. 
 *   If it is the first reference the driver init method is call.
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
 *  @see driver_reference()
 */
void driver_dereference(any_driver_t * drv);

/*@}*/


/** @name Input driver specific.
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Search an input driver that match a file extension. */
inp_driver_t * inp_driver_list_search_by_extension(const char *ext);

/** Search an input driver that a id (filetype). */
inp_driver_t * inp_driver_list_search_by_id(int id);

/*@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _DRIVER_LIST_H_ */
