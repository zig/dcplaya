/**
 * @ingroup dcplaya_devel
 * @file    SHARimg.h
 * @brief   Image descriptor definition
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/07/08
 * @version $Id: SHARimg.h,v 1.3 2002-12-15 16:15:03 ben Exp $
 */

#ifndef _SHARIMG_H_
#define _SHARIMG_H_

#include "SHAsys/SHAsysTypes.h"

struct SHARimgFileDesc;

/** Image descriptor.
 *
 * @ingroup dcplaya_devel
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @see SHARimgFileDesc
 * @see SHAtranslatorAudio
 * @see SHARheader
 */
struct SHARimgDesc
{
  unsigned int type:8;          ///< pixel format type @see Type_e
  unsigned int bitPerPixel:8;   ///< bit per pixel
  unsigned int lutSize:16;      ///< Look Up table size
  unsigned int lutType:8;       ///< Look up table pixel type
  unsigned int width;           ///< image width in pixel
  unsigned int height;          ///< image height in pixel

  SHARimgDesc & operator = (const SHARimgFileDesc & imgFileDesc);

  /** @name   Data size accessors.
   *  @{
   */
  inline unsigned int LinesBytes(void) const {
    return ((width << SHAPF_LOG2BPP(type)) + 7) >> 3;
  }

  inline unsigned int ImageBytes(void) const {
    return LinesBytes() * height;
  }

  inline unsigned int LutBytes(void) const {
    return ((lutSize << SHAPF_LOG2BPP(lutType)) + 7) >> 3;
  }

  ///@}

  static const char * ToString(unsigned int type);

};

/** Image file descriptor.
 *  @ingroup dcplaya_devel
 *
 *   SHAimgFileDesc structure is as image file descriptor.
 *   That is why all fields of the structures are arrays of chars.
 *   This structure was not design for application internal usages,
 *   but only for file storage. The SHAimgDesc methods deal with
 *   endianess and alignment problems, and for this reason are not
 *   neccessary optimal. Except for file operation, SHARimgDesc should be
 *   used.
 *
 *   SHAimgFileDesc structure is used sashipa image translator as standard
 *   image format and sashipa archiver for image file archive.
 *
 *   @see SHARimgDesc
 *   @see SHAtranslatorImage
 *   @see SHARheader
 */
struct SHARimgFileDesc
{
  friend struct SHARimgDesc;

public:
  SHARimgFileDesc();
  SHARimgFileDesc(SHARimgDesc & imgDesc);

  typedef SHApixelFormat_e Type_e;

protected:
  char type[1];         ///< pixel format type @see Type_e
  char bitPerPixel[1];  ///< bit per pixel
  char lutSize[2];      ///< Look Up table size
  char lutType[1];      ///< Look up table pixel type
  char reserved[3];     ///< reserved for futur use (must be zeroed)
  char width[4];        ///< bit per sample [4 | 8 | 16 | 32]
  char height[4];       ///< reserved for futur used (0)

protected:
  void Init(void);

  /** @name   Write accessor methods.
   *  @{
   */
  void Type(Type_e newType);
  void Type(unsigned int newType);
  void BitPerPixel(unsigned int newBitPerPixel);
  void LutSize(unsigned int newLutSize);
  void LutType(Type_e newLutType);
  void LutType(unsigned int newLutType);
  void Width(unsigned int newWidth);
  void Height(unsigned int newHeight);
  ///@}

  /** @name   Read accessor methods.
   *  @{
   */
  Type_e Type(void) const;
  unsigned int BitPerPixel(void) const;
  unsigned int LutSize(void) const;
  Type_e LutType(void) const;
  unsigned int Width(void) const;
  unsigned int Height(void) const;
  ///@}

  static const char * ToString(unsigned int type);

};

#endif //#define _SHARIMG_H_
