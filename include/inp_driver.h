/**
 * @ingroup dcplaya_plugin_inp
 * @file    inp_driver.h
 * @author  benjamin gerard <ben@sahipa.com>
 * @date    2002
 * @brief   dcplaya input plugin.
 *
 * $Id: inp_driver.h,v 1.5 2002-12-14 16:15:36 ben Exp $
 */

#ifndef _INP_DRIVER_H_
#define _INP_DRIVER_H_

/** @defgroup dcplaya_plugin_inp Input driver API
 *  @ingroup  dcplaya_plugin_devel
 *
 *  Input plugins are dcplaya music drivers.
 */

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "playa_info.h"
#include "any_driver.h"

/** @name Input plugin decoding return codes.
 *  @ingroup dcplaya_plugin_inp
 *
 *   Return bits code for decode() handler.
 *  @{
 */
#define INP_DECODE_ERROR -1 /**< Decoder reach end because of error */
#define INP_DECODE_CONT   1 /**< Decoder has decode some sample     */
#define INP_DECODE_END    2 /**< Decoder has reach end of track     */
#define INP_DECODE_INFO   4 /**< Decoder has detect some info
							     change. Call info() handler to
								 retrieve them.                   */      
/**@}*/

/** Input driver structure.
 *  @ingroup dcplaya_plugin_inp
 */
typedef struct
{
  /** Any driver common structure :  {nxt, id, name} */ 
  any_driver_t common;

  /** Id code for FILE_TYPE. Set it as you mind. */
  int id;

  /** Extensions list terminated by a double zero. */
  const char * extensions;

  /** Start a new file at given tarck and set info. */
  int (*start)(const char *fn, int track, playa_info_t *info);

  /** Stop playing file */
  int (*stop)(void);

  /** Decode next frame and updates info. */
  int (*decode)(playa_info_t *info);
  
  /** Get file info. Can be use as is_mine() function. */
  int (*info)(playa_info_t * info, const char *fn);

} inp_driver_t;

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _INP_DRIVER_H_ */
