/**
 * @ingroup   dcplaya_shaimgtranslator_devel
 * @file      SHAtranslatorImage.h
 * @brief     Image translator base class definition
 * @date      2001/07/11
 * @author    benjamin gerard <ben@sashipa.com>
 *
 * $Id: SHAtranslatorImage.h,v 1.3 2003-03-22 00:35:27 ben Exp $
 */

/** @defgroup   dcplaya_shaimgtranslator_devel   Image translator
 *  @ingroup    dcplaya_shatranslator_devel
 *  @brief      Image translator
 *
 * @author    benjamin gerard <ben@sashipa.com>
 */

#ifndef _SHATRANSLATORIMAGE_H_
#define _SHATRANSLATORIMAGE_H_

#include "SHAtranslator/SHAtranslator.h"

/** Image translator base class.
 *
 * @ingroup   dcplaya_shaimgtranslator_devel
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
