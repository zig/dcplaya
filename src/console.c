/**
 * @file      console.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.c,v 1.2 2002-09-11 03:42:53 zig Exp $
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

  return console;

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
  MUterm_inputc(c, console->term);
}
void csl_putstring(csl_console_t * console, const char * s )
{
  MUterm_input(s, console->term);
}
void csl_printf(csl_console_t * console, const char *fmt, ... )
{
  va_list args;
  va_start(args, fmt);
  csl_vprintf(console, fmt, args);
  va_end(args);
}
void csl_vprintf(csl_console_t * console, const char *fmt, va_list args )
{
  MUterm_setactive(console->term);
  MUterm_vprintf(fmt, args);
}


/* Main console */

csl_console_t * main_console;

static dbgio_printk_func old_printk_func;


static void csl_printk_func(const char * s)
{
  csl_putstring(main_console, s);

  if (old_printk_func)
    old_printk_func(s);
}

void csl_init_main_console()
{

  if (main_console)
    return;

  main_console = csl_console_create(30, 20, CSL_RENDER_BASIC);

  old_printk_func = dbgio_set_printk(csl_printk_func);
  
}
void csl_close_main_console()
{
  if (!main_console)
    return;

  csl_console_destroy(main_console);
  main_console = 0;

  dbgio_set_printk(old_printk_func);

}

