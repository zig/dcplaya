/**
 * @file    vmu_file.h
 * @ingroup dcplaya_vmu_file_devel
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2003/03/10
 * @brief   VMU file load and save function.
 *
 * $Id: vmu_file.h,v 1.4 2003-03-22 00:35:27 ben Exp $
 */

#ifndef _VMU_FILE_H_
#define _VMU_FILE_H_

/** @defgroup dcplaya_vmu_file_devel VMU file
 *  @ingroup dcplaya_devel
 *  @brief VMU file
 *
 *  @author  benjamin gerard <ben@sashipa.com>
 */

/** VMU transfert status code.
 *  @ingroup dcplaya_vmu_file_devel
 */
typedef enum {
  VMU_TRANSFERT_SUCCESS = 0, /**< The transfert has finished successfully. */
  VMU_TRANSFERT_READ = 1,    /**< A read transfert is running. */
  VMU_TRANSFERT_WRITE = 2,   /**< A write transfert is running. */
  VMU_TRANSFERT_BUSY = 4,    /**< Transfert is busy ? (unused !) */

  VMU_TRANSFERT_INVALID_HANDLE = -3, /**< Invalid transfert handle. */
  VMU_TRANSFERT_INIT_ERROR = -2,     /**< Module not initialized. */
  VMU_TRANSFERT_ERROR = -1           /**< Undefined error in transfert. */
} vmu_trans_status_e;

/** VMU transfert handle type.
 *  @ingroup dcplaya_vmu_file_devel
 */
typedef unsigned int vmu_trans_hdl_t;

/** @name VMU file init functions.
 *  @ingroup dcplaya_vmu_file_devel
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
 *  @ingroup dcplaya_vmu_file_devel
 *  @{
 */

/** Get vmu files header size.
 *
 *    The vmu_file_header_size() returns the size of the vmu header
 *    file which contains appliciation information, icon bitmap and animation
 *    and other stuff.
 *
 *  @return vmu header (in bytes).
 *  @retval -1  Error (module not initialized / file not found ?)
 */
int vmu_file_header_size(void);

/** Get the default vmu file path.
 *  @return default vmu file path.
 *  @retval 0 no vmu default file.
 */
const char * vmu_file_get_default(void);

/** Set the vmu default file path.
 *
 *  @param default_path the new vmu default file (0 : set no default path)
 *
 *  @return The newly vmu default path (must be "/vmu/[ad][1-4]/.{1,11}") 
 *  @retval 0 in case of error or if default_path is 0.
 *  @retval !0 success, returns a static buffer with a copy of default_path. 
 *
 *  @warning Since the given string is copied, the returned string points on 
 *           this copy. User MUST NOT assume is this the same value. Indeed
 *           the function may return 0 in error case. At this time only memory
 *           allocation may generate error, but in the futur may be this
 *           function will check given path validity.
 */
const char * vmu_file_set_default(const char * default_path);

/** Create dcplaya save file.
 *
 * @param  fname        Name of dcplaya save file.
 * @param  path         Path to archive (typically "/ram/dcplaya").
 * @param  set_defeult  Set this path as default path if default is not 0.
 * 
 * @return vmu transfert handle
 * @retval 0   failure
 */
vmu_trans_hdl_t vmu_file_save(const char * fname, const char * path,
			      int set_default);

/** Read and extract save file.
 *
 * @param  fname        Name of dcplaya save file.
 * @param  path         Extraction path (typically "/ram/dcplaya").
 * @param  set_default  Set this path as default path if default is not 0.
 * 
 * @return vmu transfert handle
 * @retval 0   failure
 */
vmu_trans_hdl_t vmu_file_load(const char * fname, const char * path,
			      int set_default);

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
 *
 *  @param  status  status to convert to string
 *
 *  @return always a valid string.
 *  @retval "unknown" unknown status 
 *  @retval "success" status is VMU_TRANSFERT_SUCCESS
 *  @retval "read" status is VMU_TRANSFERT_READ
 *  @retval "write" status is VMU_TRANSFERT_WRITE
 *  @retval "busy" status is VMU_TRANSFERT_BUSY
 *  @retval "invalid handle" status is VMU_TRANSFERT_INVALID_HANDLE
 *  @retval "not initialized" status is VMU_TRANSFERT_INIT_ERROR
 *  @retval "error" status is VMU_TRANSFERT_ERROR.
 */
const char * vmu_file_statusstr(vmu_trans_status_e status);

/**@}*/

#endif /* #define _VMU_FILE_H_ */
