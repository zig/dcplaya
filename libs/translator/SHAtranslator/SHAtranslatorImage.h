/**
 * @ingroup   SHAtranslatorImage
 * @file      SHAtranslatorImage.h
 * @brief     Image translator base class definition
 * @date      2001/07/11
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 * @version   $Id: SHAtranslatorImage.h,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 */

/** @defgroup   SHAtranslatorImage   Image translator
 *  @ingroup    SHAtranslator
 */

#ifndef _SHATRANSLATORIMAGE_H_
#define _SHATRANSLATORIMAGE_H_

#include "SHAtranslator/SHAtranslator.h"

/** Image translator base class.
 *
 * @ingroup   SHAtranslatorImage
 * @author    BeN(jamin) Gerard <ben@sashipa.com>
 */
class SHAtranslatorImage : public SHAtranslator
{
public:
  SHAtranslatorImage();

protected:
  /** Save SHAR header for image.
   *
   * @param result Valid image descriptor (filled result->data.image), error
   *               status could be updated.
   * @param out    Output stream
   *
   * @return      errorNo
   * @retval  0   No error
   * @retval  <0  Error, result is updated.
   */
  static int WriteHeader(SHAtranslatorResult * result, SHAstream * out);

private:
  void Init(void);

};

#endif //#define _SHATRANSLATORIMAGE_H_
