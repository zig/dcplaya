/**
 * @ingroup   SHAtranslator
 * @file      SHAtranslatorResult.h
 * @brief     Translator result class definition.
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorResult.h,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

#ifndef _SHATRANSLATORRESULT_H_
#define _SHATRANSLATORRESULT_H_

#include "SHAR/SHARimg.h"

class SHAtranslatorResult
{
public:

  SHAtranslatorResult();

  void Clean(void);

  //SHAtranslatorFamily Family(void) const;
  //void Family(SHAtranslatorFamily newFamily);
  //void Family(SHAtranslatorFamily::Type_e newFamilyType);

  inline int Error(void) const {
    return errorNo;
  }
  inline const char * ErrorStr(void) const {
    return errorStr;
  }
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
