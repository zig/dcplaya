/**
 * @file    reader.h
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/09/20
 * @brief   Memory file reader for dcplaya mikmod input plugin.
 *
 * $Id: reader.h,v 1.1 2002-09-21 09:53:40 benjihan Exp $
 */

#ifndef _READER_H_
#define _READER_H_

#include "mikmod.h"

/** Extended MREADER structure for memory file access. */
typedef struct {
  MREADER mreader; /**< Original MREADER driver. */
  char * data;     /**< File data.               */
  int pos;         /**< Current stream position. */
  int max;         /**< Size of data buffer.     */
} DCPLAYA_MREADER;

/** The instance of dcplaya mikmod reader driver. */
extern DCPLAYA_MREADER dcplaya_mreader;

#endif /* #define  _READER_H_ */

