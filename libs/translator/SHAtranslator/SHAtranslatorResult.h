/**
 * @ingroup   dcplaya_devel
 * @file      SHAtranslatorResult.h
 * @brief     Translator result class definition.
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorResult.h,v 1.3 2002-12-15 16:13:36 ben Exp $
 */

#ifndef _SHATRANSLATORRESULT_H_
#define _SHATRANSLATORRESULT_H_

#include "SHAR/SHARimg.h"

/** Translator result class.
 *  @ingroup   dcplaya_devel
 */
class SHAtranslatorResult
{
public:

  SHAtranslatorResult();

  /// Clean result.
  void Clean(void);

  /// Get error code.
  inline int Error(void) const {
    return errorNo;
  }
  /// Get error string.
  inline const char * ErrorStr(void) const {
    return errorStr;
  }
  /// Set error string and code.
  int Error(const char *str, int no = -1);

protected:
  const char *errorStr;
  int errorNo;

public:
  int   multiple;
  char  name[16];
  union _data_u {
    //    SHARwavDesc audio;
    SHARimgDesc  image;
  } data;
};

#endif  //#define _SHATRANSLATORRESULT_H_
