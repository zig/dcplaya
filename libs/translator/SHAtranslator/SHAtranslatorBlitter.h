/**
 * @ingroup   dcplaya_shaimgtranslator_devel
 * @file      SHAtranslatorBlitter.h
 * @brief     Translator image blitter.
 * @date      2001/07/11
 * @author    benjamin gerard <ben@sashipa.com>
 *
 * $Id: SHAtranslatorBlitter.h,v 1.5 2003-03-22 00:35:27 ben Exp $
 */

#ifndef _SHATRANSLATORBLITTER_H_
#define _SHATRANSLATORBLITTER_H_


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/** @name Blitter functions.
 *  @ingroup   dcplaya_shaimgtranslator_devel
 *  @{
 */

/** Image pixel convertion. */
  void GREY8toARGB32(void * dst, const void * src, int n);
  void RGB565toARGB32(void * dst, const void * src, int n);
  void ARGB1555toARGB32(void * dst, const void * src, int n);
  void ARGB4444toARGB32(void * dst, const void * src, int n);
  void RGB24toARGB32(void * dst, const void * src, int n);
  void ARGB32toARGB32(void * dst, const void * src, int n);
  void ARGB16toARGB16(void * dst, const void * src, int n);
  void GREY8toGREY8(void * dst, const void * src, int n);
  void ARGB32toARGB1555(void * dst, const void * src, int n);
  void ARGB32toRGB565(void * dst, const void * src, int n);
  void ARGB32toARGB4444(void * dst, const void * src, int n);
/** @} */

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //#define _SHATRANSLATORBLITTER_H_
