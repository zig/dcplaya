/**
 * @file      console.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.c,v 1.3 2002-09-11 14:29:13 zig Exp $
 */


#include <kos.h>
#include <malloc.h>

#include "console.h"

#include "sysdebug.h"
#include "syserror.h"


#include "video.h"


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


/* Functions to render consoles */

/* Get char address of a line */
static MUterm_char_t *term_line_addr(MUterm_t *term, int line)
{
  if(line<0) line = 0;
  line += term->start_line;
  line%=term->nline;
  return term->data + line*term->ncol;
}

void csl_basic_render(csl_console_t * c)
{
  int x, y;
  char s[2];
  uint16 * vram = (uint16 *) (char *) 0xa05f8000;

  //clrscr(0, ta_state.buffers[1].frame);

  s[1] = 0;

  for (y=0; y<c->term->nline; y++) {
    MUterm_char_t * ptr;
    
/*    ptr = c->term->data + (y + c->term->start_line)*c->term->ncol;
    if (ptr > c->term->dataend)
      ptr -= c->term->dataend - c->term->data;*/
    ptr = term_line_addr(c->term, y);
      
    for (x = 0; x<c->term->ncol; x++, ptr++) {

      s[0] = ptr->c;
      //if (s[0] && s[0] != 32)
      draw_string(x*12 + ta_state.buffers[1].frame/2, y*24, s, -1);
      //      bfont_draw(vram + ta_state.buffers[1].frame/2 + (x*12 + y*640*24), 640, 1, s[0]);

    }
  }
  
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
void csl_enable_render_mode(csl_console_t * console, int render_modes)
{
  console->render_modes |= render_modes;
}
void csl_disable_render_mode(csl_console_t * console, int render_modes)
{
  console->render_modes &= ~render_modes;
}


/* Functions to access to a console */
void csl_putchar(csl_console_t * console, char c )
{
  MUterm_inputc(c, console->term);
  if ((console->render_modes & CSL_RENDER_BASIC) && c == '\n')
    csl_basic_render(console);
}
void csl_putstring(csl_console_t * console, const char * s )
{
  MUterm_input(s, console->term);
  if ((console->render_modes & CSL_RENDER_BASIC) && strchr(s, '\n'))
    csl_basic_render(console);
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
/*  char tmp[2048];
  vsprintf(tmp, fmt, args);
  csl_putstring(console, tmp);*/
}


/* Main console */

csl_console_t * csl_main_console;

static dbgio_printk_func old_printk_func;


static void csl_printk_func(const char * s)
{
  csl_putstring(csl_main_console, s);

  if (old_printk_func)
    old_printk_func(s);
}

void csl_init_main_console()
{

  if (csl_main_console)
    return;

  csl_main_console = csl_console_create(640/12, 480/24, CSL_RENDER_BASIC);

  old_printk_func = dbgio_set_printk(csl_printk_func);
  
}
void csl_close_main_console()
{
  if (!csl_main_console)
    return;

  csl_console_destroy(csl_main_console);
  csl_main_console = 0;

  dbgio_set_printk(old_printk_func);

}

