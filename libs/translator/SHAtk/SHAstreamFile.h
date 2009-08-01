/**
 * @ingroup SHAtk
 * @file    SHAstreamFile.h
 * @brief   sashipa toolkit FILE stream class definition
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/05/14
 * @version $Id: SHAstreamFile.h,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

#ifndef _SHASTREAMFILE_H_
#define _SHASTREAMFILE_H_

#include "SHAtk/SHAstream.h"

/** C-Lib FILE based stream class.
 *
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAPI SHAstreamFile : public SHAstream
{
protected:
  void *f;    ///< FILE pointer. Should be 0.

public:

  /** Closed-stream c-tor */
  SHAstreamFile();

  /** Opens from filename and mode attribut */
  int Open(const char *name, const char *mode);

  /** Opens from already opened FILE * */
  int Open(void *file, int access);

  /** @see SHAstream::~SHAstream() */
  virtual ~SHAstreamFile();

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
  virtual int IsEOF(void);

};

#endif /* #define _SHASTREAMFILE_H_ */
