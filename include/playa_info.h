/**
 * @file    playa_info.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/23
 * @brief   Music informations
 *
 * $Id: playa_info.h,v 1.4 2002-11-29 08:29:41 ben Exp $
 */

#ifndef _PLAYA_INFO_H_
#define _PLAYA_INFO_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/**
 * @warning  Do not change order : hardcode
 */
typedef enum {

  /** @name  Numeric info.
   *  @{
   */
  PLAYA_INFO_BITS = 0, /**< 0:8 1:16                        */
  PLAYA_INFO_STEREO,   /**< 0:mono 1:stereo                 */
  PLAYA_INFO_FRQ,      /**< Sampling rate (in Hz)           */
  PLAYA_INFO_TIME,     /**< Duration (in 1024th of second)  */
  PLAYA_INFO_BPS,      /**< Nominal/reference bitrate (bps) */
  PLAYA_INFO_BYTES,    /**< Stream length (in bytes)        */
  /*@}*/

  PLAYA_INFO_DESC,     /**< Music format e.g "mpeg Layer III mono" */

  /** @name  Auto build info.
   *  @{
   */
  PLAYA_INFO_VMU,      /**< VMU scroll text                        */
  PLAYA_INFO_FORMAT,   /**< Complet format                         */
  PLAYA_INFO_TIMESTR,  /**< Time string [hh:]mm:ss                 */
  /*@}*/

  /** @name  ID3 like info.
   *  @{
   */
  PLAYA_INFO_ARTIST,
  PLAYA_INFO_ALBUM,
  PLAYA_INFO_TRACK,
  PLAYA_INFO_TITLE,
  PLAYA_INFO_YEAR,
  PLAYA_INFO_GENRE,
  PLAYA_INFO_COMMENTS,
  /*@}*/

  PLAYA_INFO_SIZE
} playa_info_e;

typedef union {
  char * s;
  unsigned int v;
} playa_info_u;

/** */
typedef struct {
  int valid;       /**< Are the following fields valids.
		      Used as a counter for modification notify. */ 

  int update_mask; /**< Bit mask of modified info field */ 

  playa_info_u info[PLAYA_INFO_SIZE];

} playa_info_t;

int playa_info_init(void);
void playa_info_shutdown(void);

char * playa_info_make_timestr(char * time, unsigned int ms);

playa_info_t * playa_info_lock(void);
void playa_info_release(playa_info_t *info);

void playa_info_clean();
void playa_info_free(playa_info_t *info);

int playa_info_update(playa_info_t *info);

void playa_info_dump(playa_info_t * info);

/** @name Numeric information accessor.
 *  @{
 */

int playa_info_bits(playa_info_t * info, int v);

int playa_info_stereo(playa_info_t * info, int v);

int playa_info_frq(playa_info_t * info, int v);

int playa_info_time(playa_info_t * info, int v);

int playa_info_bps(playa_info_t * info, int v);

int playa_info_bytes(playa_info_t * info, int v);

/**@}*/

/** @name String information accessor.
 *  @{
 */
char * playa_info_desc(playa_info_t * info, char * v);

char * playa_info_artist(playa_info_t * info, char * v);

char * playa_info_album(playa_info_t * info, char * v);

char * playa_info_track(playa_info_t * info, char * v);

char * playa_info_title(playa_info_t * info, char * v);

char * playa_info_year(playa_info_t * info, char * v);

char * playa_info_genre(playa_info_t * info, char * v);

char * playa_info_comments(playa_info_t * info, char * v);

char * playa_info_format(playa_info_t * info);

char * playa_info_timestr(playa_info_t * info);

/**@}*/

DCPLAYA_EXTERN_C_END

#endif
