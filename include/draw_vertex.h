/**
 *  @file    draw_vertex.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/10/15
 *  @brief   draw primitive vertex definition.
 *
 * $Id: draw_vertex.h,v 1.4 2002-10-22 10:35:47 benjihan Exp $
 */

#ifndef _DRAW_VERTEX_H_
#define _DRAW_VERTEX_H_

/** @name Draw flags bits.
 *  @{
 */
#define DRAW_FILTER_BIT   13 /**< 1st bit of filter type.  */
#define DRAW_OPACITY_BIT  14 /**< 1st bit of opacity type. */
#define DRAW_DEBUGIN_BIT  15 /**< User debug bit.          */
#define DRAW_TEXTURE_BIT  16 /**< 1st bit of texture.      */
/**@}*/

/** @name Draw flags texture access.
 *  @{
 */

/** Texture mask. */
#define DRAW_TEXTURE_MASK  (-1<<DRAW_TEXTURE_BIT)
/** Get texture. */
#define DRAW_TEXTURE(F)    ((unsigned int)(F)>>DRAW_TEXTURE_BIT)
/** No texture reserved value. */
#define DRAW_NO_TEXTURE    0

/**@}*/

/** @name Draw flags opacity access.
 *  @{
 */
/** Opacity mask. */
#define DRAW_OPACITY_MASK   (1<<DRAW_OPACITY_BIT)
/** Get opacity. */
#define DRAW_OPACITY(F)     ((F)&DRAW_OPACITY_MASK)
/** Opacity value for opaque drawing. */
#define DRAW_OPAQUE         (1<<DRAW_OPACITY_BIT)
/** Opacity value for translucent drawing. */
#define DRAW_TRANSLUCENT    (0<<DRAW_OPACITY_BIT)
/**@}*/

/** @name Draw flags texture filtering.
 *  @{
 */
/** Texture filter mask. */
#define DRAW_FILTER_MASK  (1<<DRAW_FILTER_BIT)
/** Get texture filter. */
#define DRAW_FILTER(F)    ((F)&DRAW_FILTER_MASK)
/** Value for no texture filter. */
#define DRAW_NO_FILTER    (1<<DRAW_FILTER_BIT)
/** Value for bilinear texture filter. */
#define DRAW_BILINEAR     (0<<DRAW_FILTER_BIT)
/**@}*/

/** Draw primitive vertex. */
typedef struct
{
  float x; /**< Screen X coordinate.                              */
  float y; /**< Screen Y coordinate.                              */
  float z; /**< Z-buffer coordinate.                              */
  float w; /**< For perspective correction when clipping. Unused. */
  float a; /**< Alpha color component [0..1].                     */
  float r; /**< Red color component [0..1].                       */
  float g; /**< Green color component [0..1].                     */
  float b; /**< Blue color component [0..1].                      */
  float u; /**< U mapping coordinate.                             */
  float v; /**< V mapping coordinate.                             */
} draw_vertex_t;

#endif /* #define _DRAW_VERTEX_H_ */
