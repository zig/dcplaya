/**
 * @ingroup SHAtk
 * @file    SHAstreamMem.h
 * @brief   sashipa toolkit memory stream class definition
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/05/14
 * @version $Id: SHAstreamMem.h,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

#ifndef _SHASTREAMMEM_H_
#define _SHASTREAMMEM_H_

#include "SHAtk/SHAstream.h"

/** Static memory buffer stream class.
 *
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAPI SHAstreamMem : public SHAstream
{
protected:
  char *buf;  ///< Pointer to memory buffer. 0 when stream is closed.
  char *pos;  ///< Stream pointer position.
  char *end;  ///< End of memory buffer.
  int  eof;   ///< End of stream flag.

public:

  /** Closed-stream c-tor */
  SHAstreamMem();

  /** Opens from filename and mode attribut
   *
   *  @param  buffer    buffer for data
   *  @param  size      size in byte of buffer
   *  @param  access    access right
   *                    -bit 0:read
   *                    -bit 1:write
   *  @return errorNo
   */
  int Open(char *buffer, unsigned int size, int access = 3);

  /** @see SHAstream::~SHAstream() */
  virtual ~SHAstreamMem();

  /** @see SHAstream::Close() */
  virtual int Close(void);

  /** @see SHAstream::Read() */
  virtual int Read(void *data, SHAstreamPos n);

  /** @see SHAstream::Write() */
  virtual int Write(const void *data, SHAstreamPos n);

  /** @see SHAstream::Seek() */
  virtual int Seek(SHAstreamPos offset, SHAstreamSeek_e origin);

  /** @see SHAstream::Tell() */
  virtual SHAstreamPos Tell(void);

  /** @see SHAstream::Flush() */
  virtual int Flush(void);

  /** @see SHAstream::IsEOF() */
  virtual int SHAstreamMem::IsEOF(void);

};

#endif /* #define _SHASTREAMFILE_H_ */
