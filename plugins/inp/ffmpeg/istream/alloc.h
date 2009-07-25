/**
 * @ingroup istream
 * @file    istream/alloc.h
 * @author  benjamin gerard
 * @date    2003/04/11
 * @brief   memory allocation.
 */

#ifndef _ISTREAM_ALLOC_H_
#define _ISTREAM_ALLOC_H_

void * istream_alloc(unsigned int n);
void stream_free(void * data);

#endif /* #define _ISTREAM_ALLOC_H_ */
