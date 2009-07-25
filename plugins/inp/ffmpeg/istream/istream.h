/**
 *  @ingroup istream
 *  @file    istream/istream.h
 *  @author  benjamin gerard
 *  @date    2003/04/10
 *  @brief   generic stream.
 */

#ifndef _ISTREAM_H_
#define _ISTREAM_H_

typedef struct _istream_t istream_t;

const char * istream_filename(istream_t *istream);
int istream_open(istream_t *istream);
int istream_close(istream_t *istream);
int istream_read(istream_t *istream, void * data, int len);
int istream_write(istream_t *istream, const void * data, int len);
int istream_length(istream_t *istream);
int istream_tell(istream_t *istream);
int istream_seek(istream_t *istream, int offset);

#endif /* #ifndef _ISTREAM_H_ */
