/**
 * @ingroup  dcplaya_draw_text
 * @file     text.h
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @brief    drawing text interface
 *
 * $Id: text.h,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#ifndef _TEXT_H_
#define _TEXT_H_

#include <stdarg.h>
#include "draw/texture.h"
#include "draw/color.h"

/** @defgroup  dcplaya_draw_text   dcplaya text drawing API.
 *  @ingroup   dcplaya_draw
 *  
 */

/** Font identifier. */
typedef unsigned int fontid_t;

/** @name text initialize functions.
 *  @ingroup dcplaya_draw_text
 *  @{
 */

/** Init the text drawing system. */
int text_init(void);

/** Shutdown text drawing system. */
void text_shutdown(void);

/**@}*/


/** @name text font functions.
 *  @ingroup dcplaya_draw_text
 *  @{
 */

/** Create a new font. */
fontid_t text_new_font(texid_t texid, int wc, int hc, int fixed);

/** Set current font size. */
float text_set_font_size(float size);

/** Set current font. */
fontid_t text_set_font(fontid_t fontid);

/**@}*/


/** @name text color functions.
 *  @ingroup dcplaya_draw_text
 *  @{
 */

/** Get text packed ARGB color. */
draw_argb_t text_get_argb(void);

/** Get text float color componants. */
void text_get_color(float *a, float *r, float *g, float *b);

/** Set text packed ARGB color. */
void text_set_argb(draw_argb_t argb);

/** Set text float color componants. */
void text_set_color(const float a, const float r,
					const float g, const float b);

/**@}*/

/** @name text drawing functions.
 *  @ingroup dcplaya_draw_text
 *  @{
 */

/** Set text format strong escape char. */
int text_set_escape(int n);

/** Draw formatted text. (va_list version). */
float text_draw_vstrf(float x1, float y1, float z1, const char *s,
					  va_list list);

/** Draw formatted text. (variable argument version). */
float text_draw_strf(float x1, float y1, float z1, const char *s, ...);

/** Draw text string. */
float text_draw_str(float x1, float y1, float z1, const char *s);

/** Draw text centered and scaled to fit in a box. */
float text_draw_str_inside(float x1, float y1, float x2, float y2, float z1,
						   const char *s);

/** Draw text centered in a box. */
float text_draw_strf_center(float x1, float y1, float x2, float y2, float z1,
                            const char *s, ...);

/**@}*/


/** @name text measure functions.
 *  @ingroup dcplaya_draw_text
 *  @{
 */

/** Measure text charactere width. */
float text_measure_char(int c);

/** Measure text string width. */
float text_measure_str(const char * s);

/** Measure formated text width. va_list version.*/
float text_measure_vstrf(const char * s, va_list list);

/** Measure formated text width. Variable argument version.*/
float text_measure_strf(const char * s, ...);

/** Measure text string width and height. */
void text_size_str(const char * s, float * w, float * h);

/** Measure charactere width and height. */
void text_size_char(char c, float * w, float * h);

/** @} */

#endif /* #define _TEXT_H_ */

