/**
 * @ingroup SHAtk
 * @file    SHAstream.h
 * @brief   sashipa toolkit abstract stream class definition.
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/05/14
 * @version $Id: SHAstream.h,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */

#ifndef _SHASTREAM_H_
#define _SHASTREAM_H_

#include "SHAsys/SHAPI.h"

/** Type redefinition for stream pointer position for further use. */
typedef int SHAstreamPos;

/** I/O stream class.
 *
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAPI SHAstream
{
protected:
  /** Access right
   *  - bit 0: read access
   *  - bit 1: write access
   */
  int right;

public:

  /** @name  Control methods
   *  @{
   */

  /** Check for opened stream
   *
   * @return  stream opening status
   * @retval  0   stream is closed
   * @retval  1   stream open for input (read) access
   * @retval  2   stream open for output (write) access
   * @retval  3   stream open for i/o (r/w) access
   */
  inline int IsOpened(void) {
    return right;
  }

  /** Close stream.
   *
   *    The Close() method closes stream. Closing a stream should Flush() data.
   *
   * @return  errorNo
   */
  virtual int Close(void) = 0;

  /** Flush a stream.
   *
   *    The Flush() methods ensures that buffered data is correctly
   *    flushed. This methods should be call by the Close() method.
   *
   * @return  errorNo
   */
  virtual int Flush(void) = 0;

  ///@}

  /** @name Data access methods
   *  @{
   */

  /** Read data from stream.
   *
   *    The Read() method reads n byte(s) and store them in memory buffer
   *    pointed by data.
   *
   * @param   data   Storage location
   * @param   n      Byte(s) to read
   * @return  number of byte not read or errorNo
   * @see     Read();
   */
  virtual int Read(void *data, SHAstreamPos n) = 0;

  /** Write data to stream.
   *
   *    The Write() method writes n byte(s) from memory buffer
   *    pointed by data to stream.
   *
   * @param   data   Data to be written
   * @param   n      Byte(s) to write
   * @return  number of byte not written or errorNo
   * @see     Read();
   */
  virtual int Write(const void *data, SHAstreamPos n) = 0;

  ///@}


  /** Seek origin enumeration. */
  enum SHAstreamSeek_e {
    CUR = 0,  //< Stream pointer moves from its current position.
    SET = 1,  //< Stream pointer set to absolute position.
    END = 2   //< Stream pointer moves from end position.
  };

  /** @name Location methods
   *  @{
   */

  /** Moves the file pointer to a specified location.
   *
   *    The Seek() method moves stream pointer to a new location
   *    specified by both offset and origin value.
   *
   * @param   offset   Number of byte from origin.
   * @param   origin   Initial position.
   * @return  errorNo   
   * @see     SHAstreamSeek_e
   * @see     Tell()
   */
  virtual int Seek(SHAstreamPos offset, SHAstreamSeek_e origin) = 0;

  /** Same as Seek(offset, SHAstreamSeek_e::SET)
   * @see     Seek()
   */
  inline int SeekTo(SHAstreamPos offset) {
    return Seek(offset, SET);
  }

  /** Same as Seek(offset, SHAstreamSeek_e::CUR)
   * @see     Seek()
   */
  inline int SeekFrom(SHAstreamPos offset) {
    return Seek(offset, CUR);
  }

  /** Same as Seek(offset, SHAstreamSeek_e::END)
   * @see     Seek()
   */
  inline int SeekEnd(SHAstreamPos offset) {
    return Seek(offset, END);
  }

  /** Get stream size.
   *
   *    The Size() method calculates stream size by successif
   *    calls of Seek() and Tell() methods.
   */
  inline unsigned int Size(void) {
    SHAstreamPos savePos = Tell();
    SHAstreamPos size;
    SeekEnd(0);
    size = Tell();
    SeekTo(savePos);
    return size;
  }

  /** Current position of stream pointer.
   *
   * @see     Seek()
   */
  virtual SHAstreamPos Tell(void) = 0;

  /** Is end of stream reach.
   *
   *    End of stream occurs when no more data is available when trying
   *    to access it. Thus stream pointer can be at the end of the stream
   *    and not set to EOF.
   */
  virtual int IsEOF(void) = 0;

  ///@}
  
};

#endif /* #define  _SHASTREAM_H_ */
