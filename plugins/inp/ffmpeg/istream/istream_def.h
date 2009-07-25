/**
 *  @ingroup istream
 *  @file    istream/istream_def.h
 *  @author  benjamin gerard
 *  @date    2003/04/11
 *  @brief   generic input stream.
 */

#ifndef _ISTREAM_DEF_H_
#define _ISTREAM_DEF_H_

#include "istream/istream.h"

/** @name input stream function types.
 *  @{
 */
typedef const char * (* istream_name_t) (istream_t *);
typedef int (* istream_open_t) (istream_t *);
typedef int (* istream_close_t) (istream_t *);
typedef int (* istream_length_t) (istream_t *);
typedef int (* istream_tell_t) (istream_t *);
typedef int (* istream_seek_t) (istream_t *, int);
typedef int (* istream_read_t) (istream_t *, void *, int);
typedef int (* istream_write_t) (istream_t *, const void *, int);
/**@}*/

/** Input stream structure. */
struct _istream_t {
  const istream_name_t name;
  const istream_open_t open;
  const istream_close_t close;
  const istream_read_t read;
  const istream_read_t write;
  const istream_length_t length;
  const istream_tell_t tell;
  const istream_seek_t seekf;
  const istream_seek_t seekb;
};

#endif /* #ifndef _ISTREAM_DEF_H_ */
