/**
 * @ingroup sha123
 * @file    sha123/alloc.h
 * @author  benjamin gerard
 * @date    2003/04/11
 * @brief   memory allocation.
 */

#ifndef _SHA123_ALLOC_H_
#define _SHA123_ALLOC_H_

void * sha123_alloc(unsigned int n);
void sha123_free(void * data);

#endif /* #define _SHA123_ALLOC_H_ */
