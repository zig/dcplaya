/**
 * @file      console.h
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.h,v 1.2 2002-09-11 14:29:13 zig Exp $
 */


#ifndef _CONSOLE_H_
#define _CONSOLE_H_


#include "mu_term.h"


typedef enum csl_render_mode {
  CSL_RENDER_BASIC  = 1,  ///< Direct write to framebuffer, exclusive
  CSL_RENDER_WINDOW = 2,  ///< Render in a windows with the TA
  CSL_RENDER_VMU    = 4,  ///< Render in the VMU
} csl_render_mode_t;

typedef struct csl_console {

  struct csl_console * next; ///< Next console

  int term_init;  ///< Is the terminal initialized
  MUterm_t * term;

  csl_render_mode_t render_modes;     ///< Render mode

} csl_console_t;


/** First element of the linked list of consoles */
extern csl_console_t * csl_first_console;


/* Functions to create and destroy a console */
csl_console_t * csl_console_create(int ncol, int nline, int render_modes);
void csl_console_destroy(csl_console_t * console);


/* Functions to update and render all consoles */
void csl_update_all();

void csl_basic_render_all();

void csl_window_opaque_render_all();
void csl_window_transparent_render_all();

void csl_vmu_render_all();


/* Functions to manage consoles */
void csl_enable_render_mode(csl_console_t * console, int modes);
void csl_disable_render_mode(csl_console_t * console, int modes);


/* Functions to access to a console */
void csl_putchar(csl_console_t * console, char c );
void csl_putstring(csl_console_t * console, const char * s );
void csl_printf(csl_console_t * console, const char *fmt, ... );
void csl_vprintf(csl_console_t * console, const char *fmt, va_list args );


/* Main console */
extern csl_console_t * csl_main_console;
void csl_init_main_console();
void csl_close_main_console();


#endif /* #ifndef _CONSOLE_H_ */

