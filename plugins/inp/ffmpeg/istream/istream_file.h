/**
 *  @ingroup istream
 *  @file    istream/istream_file.h
 *  @author  benjamin gerard
 *  @date    2003/04/11
 *  @brief   implements stream over "C" file.
 */
#ifndef _ISTREAM_FILE_H_
#define _ISTREAM_FILE_H_

#include "istream/istream.h"

/** Creates a stream for "C" FILE.
 *
 *  @param  fname  path of file.
 *  @param  mode   bit 0 : read access, bit 1 : write access.
 *
 *  @return stream
 *  @retval 0 on error
 *
 *  @note   filename is internally copied.
 */
istream_t * istream_file_create(const char * fname, int mode);

#endif /* #define _ISTREAM_FILE_H_ */
