/**
 * @file    vmu_file.h
 * @ingroup dcplaya_devel
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2003/03/10
 * @brief   VMU file load and save function.
 *
 * $Id: vmu_file.h,v 1.1 2003-03-10 22:51:48 ben Exp $
 */

#ifndef _VMU_FILE_H_
#define _VMU_FILE_H_

/** @defgroup dcplaya_vmu_file_devel VMU file
 *  @ingroup dcplaya_devel
 */

/** VMU transfert status code.
 *  @ingroup dcplaya_devel
 */
typedef enum {
  VMU_TRANSFERT_SUCCESS = 0,
  VMU_TRANSFERT_READ = 1,
  VMU_TRANSFERT_WRITE = 2,
  VMU_TRANSFERT_BUSY = 4,

  VMU_TRANSFERT_INVALID_HANDLE = -2,
  VMU_TRANSFERT_ERROR = -1
} vmu_trans_status_e;

/** VMU transfert handle type.
 *  @ingroup dcplaya_devel
 */
typedef unsigned int vmu_trans_hdl_t;

/** @name VMU file init functions.
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Initialized the vmu file module.
 *
 *  @return error-code
 *  @retval 0 success
 *  @retval <0 error
 */
int vmu_file_init(void);

/** Shutdown the vmu file module. */
void vmu_file_shutdown(void);

/**@}*/

/** @name VMU file functions.
 *  @ingroup dcplaya_devel
 *  @{
 */

/** Create dcplaya save file.
 *
 * @param  fname  Name of dcplaya save file.
 * @param  path   Path to archive (typically "/ram/dcplaya")
 * 
 * @return vmu transfert handle
 * @retval 0   failure
 */
vmu_trans_hdl_t vmu_file_save(const char * fname, const char * path);

/** Read and extract save file.
 *
 * @param  fname  Name of dcplaya save file
 * @param  path   Extraction path (typically "/ram/dcplaya")
 * 
 * @return vmu transfert handle
 * @retval 0   failure
 */
vmu_trans_hdl_t vmu_file_load(const char * fname, const char * path);

/** Get status of a vmu transfert operation.
 *
 * @param  transfer  vmu transfert handle
 *
 * @return status of vmu transfert associated with a given handle
 * @retval <0 error
 *
 * @see vmu_trans_status_e
 * @see vmu_file_load()
 * @see vmu_file_save()
 */
vmu_trans_status_e vmu_file_status(vmu_trans_hdl_t transfer);

/** Get a string description of a vmu transfert status.
 */
const char * vmu_file_statusstr(vmu_trans_status_e status);

/**@}*/

#endif /* #define _VMU_FILE_H_ */
