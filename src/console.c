/**
 * @file      console.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.c,v 1.1 2002-09-11 03:23:33 zig Exp $
 */


#include <kos.h>
#include <malloc.h>

#include "console.h"

#include "sysdebug.h"
#include "syserror.h"


/** First element of the linked list of consoles */
csl_console_t * csl_first_console;


/* Functions to create and destroy a console */
csl_console_t * csl_console_create(int ncol, int nline, int render_modes)
{
  csl_console_t * console = NULL;

  console = (csl_console_t *) calloc(1, sizeof(csl_console_t));

  if (console == NULL)
    SERROR(error);

  /* Insert into the list */
  console->next = csl_first_console;
  csl_first_console = console;

  console->term = MUterm_create(ncol, nline, 1);
  if (console->term == NULL)
    SERROR(error);

  console->render_modes = render_modes;;

 error:
  if (console)
    csl_console_destroy(console);

  return NULL;
}

void csl_console_destroy(csl_console_t * console)
{
  /* Remove from the list */
  {
    csl_console_t * c;
    csl_console_t ** pc;

    c = csl_first_console;
    pc = &csl_first_console;
    while (c) {
      if (c == console) {
	*pc = c->next;
	break;
      }

      pc = &c->next;
      c = c->next;
    }
  }

  if (console->term)
    MUterm_kill(console->term);

  free(console);
}


/* Functions to update and render all consoles */
void csl_update_all()
{
}

void csl_basic_render_all()
{
}

void csl_window_opaque_render_all()
{
}
void csl_window_transparent_render_all()
{
}

void csl_vmu_render_all()
{
}


/* Functions to manage consoles */
void csl_enable_render_mode(csl_console_t * console, int modes)
{
}
void csl_disable_render_mode(csl_console_t * console, int modes)
{
}


/* Functions to access to a console */
void csl_putchar(csl_console_t * console, char c )
{
}
void csl_putstring(csl_console_t * console, const char * s )
{
}
void csl_printf(csl_console_t * console, const char *fmt, ... )
{
}
void csl_vprintf(csl_console_t * console, const char *fmt, va_list args )
{
}


/* Main console */
csl_console_t * main_console;
void csl_init_main_console()
{
}
void csl_close_main_console()
{
}

