/**
 *  @file    draw_vertex.h
 *  @author  benjamin gerard <ben@sashipa.com>
 *  @date    2002/10/15
 *  @brief   draw primitive vertex definition.
 *
 * $Id: draw_vertex.h,v 1.2 2002-10-19 18:34:40 benjihan Exp $
 */

#ifndef _DRAW_VERTEX_H_
#define _DRAW_VERTEX_H_

/** @name Draw flags bits.
 *  @{
 */
#define DRAW_OPACITY_BIT  12 /**< 1st bit of opacity type. */
#define DRAW_SHADING_BIT  13 /**< 1st bit of shading type. */
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
#define DRAW_TRANSLUCENT  (0<<DRAW_OPACITY_BIT)
/**@}*/

/** @name Draw flags shading access.
 *  @{
 */

/** Shading mask. */
#define DRAW_SHADING_MASK  (1<<DRAW_SHADING_BIT)
/** Get shading. */
#define DRAW_SHADING(F)    ((F)&DRAW_SHADING_MASK)
/** Value for flat shading. */
#define DRAW_FLAT          (1<<DRAW_SHADING_BIT)
/** Value for gouraud shading. */
#define DRAW_GOURAUD       (0<<DRAW_SHADING_BIT)

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
