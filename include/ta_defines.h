#ifndef _TA_DEFINES_H_
#define _TA_DEFINES_H_

/** @name List type, word 0, bit 24-26.
 *  @{
 */
#define TA_OPACITY_BIT               25
#define TA_OPAQUE_POLYGON_LIST       (0<<24)
#define TA_OPAQUE_MODIFIER_LIST      (1<<24)
#define TA_TRANSLUCENT_POLYGON_LIST  (2<<24)
#define TA_TRANSLUCENT_MODIFIER_LIST (3<<24)
#define TA_PUNCHTHRU_LIST            (4<<24)
/**@}*/

/** @name strip length, word 0, bit 18-19.
 *  @{
 */
#define TA_STRIP_LENGTH(N) (((N)>>1)<<18)
#define TA_STRIP_LENGTH1 (0<<18)
#define TA_STRIP_LENGTH2 (1<<18)
#define TA_STRIP_LENGTH4 (2<<18)
#define TA_STRIP_LENGTH6 (3<<18)
/**@}*/

/** @name clip mode, word 0, bit 16-17.
 *  @{
 */
#define TA_CLIP_MODE_DISABLE  (0<<16)
#define TA_CLIP_MODE_RESERVED (1<<16)
#define TA_CLIP_MODE_INSIDE   (2<<16)
#define TA_CLIP_MODE_OUTSIDE  (3<<16)
/**@}*/

/** @name modifier affect, word 0, bit 7.
 *  @{
 */
#define TA_MODIFIER_AFFECT_DISABLE (0<<7)
#define TA_MODIFIER_AFFECT_ENABLE  (1<<7) /**< only valid for POLYGON. */
/**@}*/

/** @name modifier mode, word 0, bit 6.
 *  @{
 */
#define TA_MODIFIER_CHEAP_SHADOW (0<<6)
#define TA_MODIFIER_NORMAL       (1<<6)
/**@}*/

/** @name color type, word 0, bit 4-5.
 *  @{
 */
#define TA_COLOR_TYPE_ARGB                    (0<<4)
#define TA_COLOR_TYPE_FLOAT                   (1<<4)
#define TA_COLOR_TYPE_INTENSITY               (2<<4)
#define TA_COLOR_TYPE_INTENSITY_PREVIOUS_FACE (3<<4)
/**@}*/

/** @name textured, word 0, bit 3.
 *  @{
 */
#define TA_TEXTURE_BIT     3
#define TA_TEXTURE_DISABLE (0<<3)
#define TA_TEXTURE_ENABLE  (1<<3)
/**@}*/

/** @name specular highlight, word 0, bit 2.
 *  @{
 */
#define TA_SPECULAR_DISABLE (0<<2)
#define TA_SPECULAR_ENABLE  (1<<2)
/**@}*/

/** @name shading, word 0, bit 1.
 *  @{
 */
#define TA_SHADING_BIT      1
#define TA_FLAT_SHADING     (0<<1)
#define TA_GOURAUD_SHADING  (1<<1)
/**@}*/

/** @name UV format, word 0, bit 0.
 *  @{
 */
#define TA_UV_32  (0<<0)
#define TA_UV_16  (1<<0)
/**@}*/

/** @name Depth mode, word 1, bit 29-31.
 *  @{
 */
#define TA_DEPTH_NEVER	       (0<<29)
#define TA_DEPTH_LESS	       (1<<29)
#define TA_DEPTH_EQUAL         (2<<29)
#define TA_DEPTH_LESSEQUAL     (3<<29)
#define TA_DEPTH_GREATER       (4<<29)
#define TA_DEPTH_NOTEQUAL	   (5<<29)
#define TA_DEPTH_GREATEREQUAL  (6<<29)
#define TA_DEPTH_ALWAYS	       (7<<29)
/**@}*/

/** @name Culling mode, word 1, bit 27-28.
 *  @{
 */
#define TA_CULLING_DISABLE	(0<<27)
#define TA_CULLING_SMALL	(1<<27)
#define TA_CULLING_CCW	    (2<<27)
#define TA_CULLING_CW	    (3<<27)
/**@)*/

/** @name Z-write, word 1, bit 26.
 *  @{
 */
#define TA_ZWRITE_DISABLE	(0<<26)
#define TA_ZWRITE_ENABLE	(1<<26)
/**@)*/


/** @name Mipmap D-calcul, word 1, bit 20
 *  @{
 */
#define TA_DCALC_APPROX (0<<20)
#define TA_DCALC_EXACT  (1<<20)
/**@}*/

#endif /* #define _TA_DEFINES_H_ */

