#ifndef _PLAYA_INFO_H_
#define _PLAYA_INFO_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


/** */
typedef struct {
  int valid;      /**< Are the following fields valids.
      Used as a counter for modification notify. */
  
  char * info;    /**< Global labeled info string */

  /* Decoder info */
  char * format;  /**< Stream format info string. */
  char * time;    /**< Duration string "[H:]MM:SS" */
  
  /* ID3 like info. Do *NOT* change order: hardcoded list in libmp3/main.c */
  char * artist;
  char * album;
  char * track;
  char * title;
  char * year;
  char * genre;
  char * comments;
  
} playa_info_t;

/** Stream format info. Filled by input plugin */
typedef struct 
{
  char * desc;        /**< e.g "Layer III mono" */  
  int bits;           /**< 0:8 1:16 */
  int stereo;         /**< 0:mono 1:stereo */
  unsigned int frq;   /**< Sampling rate */
  unsigned int bps;   /**< Nominal/reference bitrate (bps) */
  unsigned int bytes; /**< Stream length in byte */
  unsigned int time;  /**< Duration in ms */
  
} decoder_info_t;

DCPLAYA_EXTERN_C_END

#endif
