/**
 * @ingroup   SHAtranslator
 * @file      SHAtranslator.h
 * @brief     data translator abstract class definition
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslator.h,v 1.1 2002-09-27 16:45:07 benjihan Exp $
 */

/** @defgroup   SHAtranslator   Data translator
 */

#ifndef _SHATRANSLATOR_H_
#define _SHATRANSLATOR_H_

#include "SHAsys/SHAsysTypes.h"
#include "SHAtranslator/SHAtranslatorResult.h"
#include "SHAtk/SHAstream.h"

/** Data translator abstract class.
 *
 * @ingroup   SHAtranslator
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAtranslator
{
public:
  // Clean c-tor
  SHAtranslator();

  /** Get file extension list.
   *
   *    Extension list is a 0 terminated array of pointer
   *    to file extension commonly attached to this type of
   *    data. All extensions must be in the same memory buffer
   *    separate by a 0. Last extension is followed by a 2nd zero.
   *    The returned pointer must point to be the beginning of the buffer,
   *    others could be in any order, but it is probalbly easier to
   *    respect the same order. Each file extension must begin by
   *    a period '.' follwed by lower case characters. Their is no restriction
   *    for the size of the extension but maximum of 4 chars after the period
   *    seems respectable. 3 is better too keeps old compatibily...
   *
   * @code
   *    // Sample code :
   *    const char **SHAtranslatorExample::Extension(void) const
   *    {
   *                              // 01234 56789 A
   *      static const char ext[] = ".gif\0.tif\0\0";
   *      static const char *exts[] = { ext, ext+5, 0 };
   *      return exts;
   *    }
   * @endcode
   */
  virtual const char **Extension(void) const = 0;

  /// Quick test if stream is suitable format
  virtual int Test(SHAstream * inStream) = 0;

  /// Get familty specific info.
  virtual int Info(SHAstream * inStream,
                   SHAtranslatorResult * result) = 0;


  /// Translate from translator format to general format.
  virtual int Load(SHAstream * outStream,
                   SHAstream * inStream,
                   SHAtranslatorResult * result)= 0;


  /// Translate from general format to translator format.
  virtual int Save(SHAstream * outStream,
                   SHAstream * inStream,
                   SHAtranslatorResult * result)= 0;

};

#endif  //#define _SHATRANSLATOR_H_
