/**
 * @ingroup   SHAtranslator
 * @file      SHAtranslatorResult.cxx
 * @brief     Translator result class implementation.
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorResult.cxx,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

#include "SHAtranslator/SHAtranslatorResult.h"
#include <string.h>

void SHAtranslatorResult::Clean(void)
{
  memset(this,0,sizeof(*this));
}


SHAtranslatorResult::SHAtranslatorResult()
{
  Clean();
}

int SHAtranslatorResult::Error(const char *str, int no)
{
  errorStr = str;
  return errorNo = no;
}
