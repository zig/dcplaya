/**
 * @ingroup   dcplaya_consol_devel
 * @file      console.h
 * @author    vincent penne
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.h,v 1.10 2004-07-04 14:16:44 vincentp Exp $
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <arch/spinlock.h>
#include "mu_term.h"

/** @defgroup  dcplaya_console_devel Console
 *  @ingroup   dcplaya_devel
 *  @brief     generic console
 *  @author    vincent penne
 *  @{
 */

/** Console mode enumeration.
 */
typedef enum csl_render_mode {
  CSL_RENDER_BASIC  = 1,  ///< Direct write to framebuffer, exclusive
  CSL_RENDER_WINDOW = 2,  ///< Render in a windows with the TA
  CSL_RENDER_VMU    = 4,  ///< Render in the VMU
} csl_render_mode_t;


/** Console window structure.
 * @warning do not change color components order. Must be a,r,g,b.
 */
typedef struct csl_window {
  int x, y;
  int w, h;
  float scalex, scaley;
  float ca, cr, cg, cb;
  float ta, tr, tg, tb;
  float ba1, br1, bg1, bb1;
  float ba2, br2, bg2, bb2;
  float cursor_time;
  float z;
} csl_window_t;

/** Console structure.
 */
typedef struct csl_console {

  int w, h, opaque;

  spinlock_t mutex; ///< mutex for terminal access

  struct csl_console * next; ///< Next console

  int term_init;  ///< Is the terminal initialized
  MUterm_t * term;

  csl_render_mode_t render_modes;     ///< Render mode

  csl_window_t window; ///< Window mode settings

} csl_console_t;


/** First element of the linked list of consoles */
extern csl_console_t * csl_first_console;

/** Main console. */
extern csl_console_t * csl_main_console;

/** Basic console. */
extern csl_console_t * csl_basic_console;

/** TA console */
extern csl_console_t * csl_ta_console;

/** @name Console initialization functions.
 *  @{
 */

/** Create a console. */
csl_console_t * csl_console_create(int ncol, int nline, int render_modes);

/** Destroy a console */
void csl_console_destroy(csl_console_t * console);

/**@}*/

/** @name Console update and render functions.
 *  @{
 */

void csl_update_all(float frametime);
void csl_basic_render_all();
void csl_window_opaque_render_all();
void csl_window_transparent_render_all();
void csl_vmu_render_all();

/**@}*/


/** @name Console management functions.
 *  @{
 */

/** Enable render modes for a console. */ 
void csl_enable_render_mode(csl_console_t * console, int modes);

/** Disable render modes for a console. */ 
void csl_disable_render_mode(csl_console_t * console, int modes);

/** Configure console parameters. */
void csl_window_configure(csl_console_t * console, int x, int y, int w, int h,
			  float scalex, float scaley, float z, int opaque);

/** Set and get console colors. 
 *
 *    The function set and the console colors. For each of the color parameter
 *    a null value does nothing. Else it is a pointer value to a buffer of
 *    4 floats in a,r,g,b order. If @b read-denied bit is clear (0),
 *    the color is  used as new color. If the @b write-denied nit is clear
 *    the previous color is write back into the buffer.
 *    That way a default 0 will do a complete set and get operation.
 *    Notice that setting both read/write bit for a color do the same than
 *    a null pointer (vis-versa).
 *
 * @param  con         console (0 default to csl_main_console)
 * @param  bkgcolor1   top background color (a,r,g,b)
 * @param  bkgcolor2   bottom background color (a,r,g,b)
 * @param  txtcolor    text color (a,r,g,b)
 * @param  cursorcolor cursor color (a,r,g,b)
 * @param  what        bit 0-3: read-denied bits for respectively bkgcolor1,
 *                     bkgcolor2, txtcolor.
 *                     bit 4-7: write-denied bits for respectively bkgcolor1,
 *                     bkgcolor2, txtcolor.
 *@code
 * extern float b1[4], b2[4], t[4], c[4]; // Some color buffer...
 * csl_console_setcolor(0,b1,b2,t,0,0x12);
 * // b1 sets the top color of console background and gets the previous back.
 * // b2 is read-denied, so it only gets the previous background bottom color
 * // back.
 * // t is write denied so it only set the text color and is unchanged.
 * // Don't mind about cursor color.
 *@endcode
 */
 void csl_console_setcolor(csl_console_t * con,
			   float * bkgcolor1, float * bkgcolor2,
			   float * txtcolor, float * cursorcolor, int what);


/**@}*/

/** @name Console access functions.
 *  @{
 */
void csl_write(csl_console_t * console, const char * s, int len );
void csl_putchar(csl_console_t * console, char c );
void csl_putstring(csl_console_t * console, const char * s );
void csl_printf(csl_console_t * console, const char *fmt, ... );
void csl_vprintf(csl_console_t * console, const char *fmt, va_list args );
/**@}*/

/** @name Main console functions.
 *  @{
 */
extern csl_console_t * csl_main_console;
void csl_init_main_console();
void csl_close_main_console();
/**@}*/

/** @name Keyboard input functions.
 *  @{
 */
int csl_getchar();
int csl_peekchar();
/**@}*/

/**@}*/

#endif /* #ifndef _CONSOLE_H_ */

