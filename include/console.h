/**
 * @ingroup   dcplaya_consol_devel
 * @file      console.h
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.h,v 1.7 2003-03-22 00:35:26 ben Exp $
 */

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#include <arch/spinlock.h>
#include "mu_term.h"

/** @defgroup  dcplaya_console_devel console
 *  @ingroup   dcplaya_devel
 *  @brief     console API.
 */

/** Console mode enumeration.
 * @ingroup   dcplaya_console_devel
 */
typedef enum csl_render_mode {
  CSL_RENDER_BASIC  = 1,  ///< Direct write to framebuffer, exclusive
  CSL_RENDER_WINDOW = 2,  ///< Render in a windows with the TA
  CSL_RENDER_VMU    = 4,  ///< Render in the VMU
} csl_render_mode_t;


/** Console window structure.
 *  @ingroup dcplaya_console_devel
 */
typedef struct csl_window {
  int x, y;
  int w, h;
  float scalex, scaley;
  float tr, tg, tb, ta;
  float br1, bg1, bb1, ba1;
  float br2, bg2, bb2, ba2;
  float cursor_time;
  float z;
} csl_window_t;

/** Console structure.
 *  @ingroup   dcplaya_console_devel
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
 *  @ingroup dcplaya_console_devel
 */

/** Create a console. */
csl_console_t * csl_console_create(int ncol, int nline, int render_modes);

/** Destroy a console */
void csl_console_destroy(csl_console_t * console);

/**@}*/

/** @name Console update and render functions.
 *  @ingroup dcplaya_console_devel
 */

void csl_update_all(float frametime);
void csl_basic_render_all();
void csl_window_opaque_render_all();
void csl_window_transparent_render_all();
void csl_vmu_render_all();

/**@}*/


/** @name Console management functions.
 *  @ingroup dcplaya_console_devel
 */

/** Enable render modes for a console. */ 
void csl_enable_render_mode(csl_console_t * console, int modes);

/** Disable render modes for a console. */ 
void csl_disable_render_mode(csl_console_t * console, int modes);

/** Configure console parameters. */
void csl_window_configure(csl_console_t * console, int x, int y, int w, int h,
			  float scalex, float scaley, float z, int opaque);

/**@}*/

/** @name Console access functions.
 *  @ingroup dcplaya_console_devel
 */
void csl_putchar(csl_console_t * console, char c );
void csl_putstring(csl_console_t * console, const char * s );
void csl_printf(csl_console_t * console, const char *fmt, ... );
void csl_vprintf(csl_console_t * console, const char *fmt, va_list args );
/**@}*/

/** @name Main console functions.
 *  @ingroup dcplaya_console_devel
 */
extern csl_console_t * csl_main_console;
void csl_init_main_console();
void csl_close_main_console();
/**@}*/

/** @name Keyboard input functions.
 *  @ingroup dcplaya_console_devel
 */
int csl_getchar();
int csl_peekchar();
/**@}*/

#endif /* #ifndef _CONSOLE_H_ */

