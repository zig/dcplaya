/**
 * @ingroup   dcplaya_devel
 * @file      SHAtranslatorResult.cxx
 * @brief     Translator result class implementation.
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 *
 * $Id: SHAtranslatorResult.cxx,v 1.3 2002-12-15 16:13:36 ben Exp $
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
