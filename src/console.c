/**
 * @file      console.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.c,v 1.9 2002-09-15 15:31:04 zig Exp $
 */


#include <kos.h>
#include <malloc.h>

#include "console.h"

#include "sysdebug.h"
#include "syserror.h"


#include "video.h"
#include "gp.h"


#define CSL_BASIC_OFFSET_X 12
#define CSL_BASIC_OFFSET_Y 24



/** First element of the linked list of consoles */
csl_console_t * csl_first_console;



/* Main console */

csl_console_t * csl_main_console;

static dbgio_printk_func old_printk_func;


static void csl_printk_func(const char * s)
{
  csl_putstring(csl_main_console, s);

  if ( !(csl_main_console->render_modes & CSL_RENDER_WINDOW) ) {
    if (old_printk_func)
      old_printk_func(s);
  }
}

void csl_init_main_console()
{

  if (csl_main_console)
    return;

#if 0
  csl_main_console = csl_console_create((640 - 2*CSL_BASIC_OFFSET_X)/12, 
					(480 - 2*CSL_BASIC_OFFSET_Y)/24, 
					CSL_RENDER_BASIC);
#else
  csl_main_console = csl_console_create(60, 
					20, 
					0/*CSL_RENDER_BASIC*/);
#endif

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




/* Functions to create and destroy a console */
csl_console_t * csl_console_create(int ncol, int nline, int render_modes)
{
  csl_console_t * console = NULL;

  console = (csl_console_t *) calloc(1, sizeof(csl_console_t));

  if (console == NULL)
    STHROW_ERROR(error);

  /* Insert into the list */
  console->next = csl_first_console;
  csl_first_console = console;

  console->term = MUterm_create(ncol, nline, 1);
  if (console->term == NULL)
    STHROW_ERROR(error);

  console->w = ncol;
  console->h = nline;
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
  static int debug;
  int x, y;
  char s[2];
  //uint16 * vram = (uint16 *) (char *) 0xa05f8000;

  //clrscr(0, ta_state.buffers[1].frame);

  s[1] = 0;

  debug++;
  debug = 1;

  for (y=0; y<c->term->nline; y++) {
    MUterm_char_t * ptr;
    
/*    ptr = c->term->data + (y + c->term->start_line)*c->term->ncol;
    if (ptr > c->term->dataend)
      ptr -= c->term->dataend - c->term->data;*/
    ptr = term_line_addr(c->term, y);
      
    for (x = 0; x<c->term->ncol; x++, ptr++) {

      s[0] = ptr->c + 32;
      if (!(debug&31) && old_printk_func) {
	old_printk_func(s);
      }
      //if (s[0] && s[0] != 32)
      draw_string(CSL_BASIC_OFFSET_X + x*12 + ta_state.buffers[1].frame/2, 
		  CSL_BASIC_OFFSET_Y + y*24, s, -1);
      //      bfont_draw(vram + ta_state.buffers[1].frame/2 + (x*12 + y*640*24), 640, 1, s[0]);

    }
    if (!(debug&31) && old_printk_func) {
      old_printk_func("\n");
    }
  }
  
}

void csl_window_transparent_render(csl_console_t * c)
{
  int y;
  char s[128]; // warning : console with 128 cars width max
  char * p;
  float oldscale;

  //oldscale = text_set_font_size(c->window.scalex);


  draw_poly_box(c->window.x, c->window.y,
		c->window.x + c->window.w, c->window.y + c->window.h, 
		200.0f,
		c->window.ba1, c->window.br1, c->window.bg1, c->window.bb1,
		c->window.ba2, c->window.br2, c->window.bg2, c->window.bb2);
  

  for (y=0; y<c->term->nline; y++) {
    MUterm_char_t * ptr;
    MUterm_char_t * ptrs;
    
    ptrs = ptr = term_line_addr(c->term, y);

    ptr += c->w-1;
    p = s+sizeof(s);

    *--p = 0;
    while (!ptr->c)
      ptr--;
    while (ptr >= ptrs) {
      *--p = ptr->c + 32;
      ptr--;
    }

    if (*p) {
      draw_poly_text(c->window.x, c->window.y + y*16*c->window.scaley, 
		     205.0f,
		     c->window.ta, c->window.tr, c->window.tg, c->window.tb,
		     p);
      //      printf(p);
    }

    if (c->window.cursor_time < 0.5 && y == c->term->cursor.y) {
      float px = c->window.x;
      float w;
      int i;
      
      for (i=0; i<c->term->cursor.x; i++)
	px += measure_poly_char(p[i]);

      if (p[i])
	w = measure_poly_char(p[i]);
      else
	w = 8;

      draw_poly_box(px, c->window.y + y*16*c->window.scaley,
		    px + w, c->window.y + y*16*c->window.scaley + 16*c->window.scaley, 
		    200.0f,
		    1.0f, 0.0f, 1.0f, 1.0f,
		    1.0f, 0.0f, 1.0f, 1.0f);
    }



  }

  //text_set_font_size(oldscale);

}

static void csl_update(csl_console_t * c, float frametime)
{
  c->window.cursor_time += frametime;
  if (c->window.cursor_time > 1) {
    c->window.cursor_time -= 1;
  }
}


/* Functions to update and render all consoles */
void csl_update_all(float frametime)
{
  csl_console_t * c = csl_first_console;

  while (c) {

    csl_update(c, frametime);

    c = c->next;

  }
}

void csl_basic_render_all()
{
}

void csl_window_opaque_render_all()
{
}
void csl_window_transparent_render_all()
{
  csl_console_t * c = csl_first_console;

  while (c) {

    if (c->render_modes & CSL_RENDER_WINDOW)
      csl_window_transparent_render(c);

    c = c->next;

  }
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

void csl_window_configure(csl_console_t * console, int x, int y, int w, int h,
			  float scalex, float scaley)
{
  console->window.x = x;
  console->window.y = y;
  console->window.w = w;
  console->window.h = h;
  console->window.scalex = scalex;
  console->window.scaley = scaley;
}



/* Functions to access to a console */
void csl_putchar(csl_console_t * console, char c )
{
  spinlock_lock(&console->mutex);
  MUterm_inputc(c, console->term);
  if ((console->render_modes & CSL_RENDER_BASIC) && c == '\n')
    csl_basic_render(console);
  console->window.cursor_time = 0;
  spinlock_unlock(&console->mutex);
}
void csl_putstring(csl_console_t * console, const char * s )
{
  spinlock_lock(&console->mutex);
  MUterm_input(s, console->term);
  if ((console->render_modes & CSL_RENDER_BASIC) && strchr(s, '\n'))
    csl_basic_render(console);
  console->window.cursor_time = 0;
  spinlock_unlock(&console->mutex);
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

  spinlock_lock(&console->mutex);
  MUterm_setactive(console->term);
  MUterm_vprintf(fmt, args);

  /*
  char tmp[2048];
  vsprintf(tmp, fmt, args);
  csl_putstring(console, tmp);
  */

  console->window.cursor_time = 0;
  spinlock_unlock(&console->mutex);
}


int csl_getchar()
{
  int k;

  for ( ;; ) {
    
    k = csl_peekchar();
    if (k != -1)
      return k;

    thd_pass();
  }

}



int csl_peekchar()
{
  static last_frame = -1;
  int k;


  if (ta_state.frame_counter != last_frame) {
    kbd_poll(maple_first_kb());
    k = kbd_get_key();
    
    if (k != -1)
      return k;
  }

  return -1;
}
