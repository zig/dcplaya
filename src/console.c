/**
 * @ingroup   dcplaya_devel
 * @file      console.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     console handling for dcplaya
 * @version   $Id: console.c,v 1.26 2004-06-30 15:17:36 vincentp Exp $
 */


#include <kos.h>
#include <malloc.h>

#include "dcplaya/config.h"
#include "console.h"

#include "sysdebug.h"
#include "syserror.h"


#include "draw/video.h"
#include "draw/gc.h"
#include "draw/box.h"
#include "draw/draw.h"

#define CSL_BASIC_OFFSET_X 12
#define CSL_BASIC_OFFSET_Y 24

/** First element of the linked list of consoles */
csl_console_t * csl_first_console;

/* Main console */
csl_console_t * csl_main_console;

/* Basic console */
csl_console_t * csl_basic_console;

/* TA console */
csl_console_t * csl_ta_console;

static dbgio_printk_func old_printk_func;

/* added by ben for disabling console output in zed */
int csl_echo = 1;

static void csl_printk_func(const char * s)
{
  if (thd_enabled && csl_echo) {
    csl_putstring(csl_main_console, s);
  }

  /*  if ( !(csl_main_console->render_modes & CSL_RENDER_WINDOW) ) */{
    if (old_printk_func)
      old_printk_func(s);
  }
}

void csl_init_basic_console()
{
  int w,h;
  if (csl_basic_console) {
    return;
  }

  w = (640-CSL_BASIC_OFFSET_X) / 12;
  h = (480-CSL_BASIC_OFFSET_Y) / 24;

  csl_basic_console = csl_console_create(w,h,CSL_RENDER_BASIC);
  if (!csl_basic_console) {
    return;
  }
  csl_window_configure(csl_basic_console,
		       CSL_BASIC_OFFSET_X, CSL_BASIC_OFFSET_Y,
		       w*12, h*24,
		       1,1,0,0);
}

void csl_init_ta_console()
{
  const int fw = 8;
  const int fh = 14;
  int w,h;

  if (csl_ta_console) {
    return;
  }

  w = (640 - 32) / fw;
  //h = 400 / fh;
  h = 300 / fh;
  csl_ta_console = csl_console_create(w,h,0);

  csl_window_configure(csl_ta_console, 25, 50, w * fw, h * fh, 1, 1, 210, 0);
  csl_ta_console->window.ba1 = 0.7;   csl_ta_console->window.ba2 = 0.7;
  csl_ta_console->window.br1 = 0.25;  csl_ta_console->window.br2 = 0.00;
  csl_ta_console->window.bb1 = 0.25;  csl_ta_console->window.bg2 = 0.25;
  csl_ta_console->window.bg1 = 0.00;  csl_ta_console->window.bb2 = 0.00;

  csl_ta_console->window.ta = 1.0;
  csl_ta_console->window.tr = 0.26;
  csl_ta_console->window.tg = 1.0;
  csl_ta_console->window.tb = 0.18;

  csl_ta_console->window.ca = 1;
  csl_ta_console->window.cr = 0;
  csl_ta_console->window.cg = 1;
  csl_ta_console->window.cb = 1;

}

static void change_color(float * d, float * s, int copy)
{
  float save;
  int i;
  for (i=0; i<4; ++i) {
    save = d[i];
    if (s) {
      if (!(copy & 0x01)) { /* not read denied */
	d[i] = s[i];
      }
      if (!(copy & 0x10)) { /* not write denid */
	s[i] = save;
      }
    }
  }
}

void csl_console_setcolor(csl_console_t * con,
			  float * bkgcolor1, float * bkgcolor2,
			  float * txtcolor, float * cursorcolor,
			  int what)
{
  csl_window_t  * win;

  if (!con) {
    con = csl_main_console;
  }
  if (!con) {
    return;
  }

  win = &con->window;
  change_color(&win->ba1, bkgcolor1,   what >> 0);
  change_color(&win->ba2, bkgcolor2,   what >> 1);
  change_color(&win->ta,  txtcolor,    what >> 2);
  change_color(&win->ca,  cursorcolor, what >> 3);
}

void csl_init_main_console()
{
  csl_init_basic_console();
  csl_init_ta_console();

  csl_main_console = csl_basic_console;
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

  if (console == NULL) {
    STHROW_ERROR(error);
  }

  /* Insert into the list */
  console->next = csl_first_console;
  csl_first_console = console;

  console->term = MUterm_create(ncol, nline, 1);
  if (console->term == NULL) {
    STHROW_ERROR(error);
  }

  console->w = ncol;
  console->h = nline;
  console->render_modes = render_modes;;

  return console;

 error:
  if (console) {
    csl_console_destroy(console);
  }

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
  int y, yoffset;
  int nlines, ncols;
  char s[128];
  s[1] = 0;

  nlines = c->term->nline;
  ncols  = c->term->ncol;

  /*   spinlock_lock(&c->mutex); */
  for (y=0, yoffset=c->window.y; y<nlines; y++, yoffset+=24) {
    MUterm_char_t * ptr = term_line_addr(c->term, y);
    int x;

    for (x = 0; x < ncols; ++x) {
      s[x] = ptr[x].c + 32;
    }
    s[x] = 0;
    draw_string(c->window.x + ta_state.buffers[1].frame/2, yoffset, s, -1);
  }
  /*   spinlock_unlock(&c->mutex); */
}

void csl_window_transparent_render(csl_console_t * c)
{
  int y;
  char s[128]; // warning : console with 128 cars width max
  char * p;
  /*   float oldsize, oldaspect; */
  /*   int oldfont; */
  int oldescape;
  const float z = c->window.z;
  

  //spinlock_lock(&c->mutex);
  
  text_set_properties(1,16,1,1);
  /*   oldfont = text_set_font(1); // Select fixed spacing font */
  /*   oldsize = text_set_font_size(8); */
  /*   oldaspect = text_set_font_aspect(1); // Select fixed spacing font */
  /*   oldfilter = text_set */
  oldescape = text_set_escape(-1); // No escape character

  if (!c->opaque) {
    draw_box2v(c->window.x, c->window.y,
	       c->window.x + c->window.w, c->window.y + c->window.h, 
	       z,
	       c->window.ba1, c->window.br1, c->window.bg1, c->window.bb1,
	       c->window.ba2, c->window.br2, c->window.bg2, c->window.bb2);
  }
  

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
      text_set_color(c->window.ta, c->window.tr, c->window.tg, c->window.tb);
      text_draw_str(c->window.x, c->window.y + y*14*c->window.scaley, 
		    z + 10, p);
    }

    if (c->window.cursor_time < 0.5 && y == c->term->cursor.y) {
      float px = c->window.x;
      float w;
      int i;
      
      for (i=0; i<c->term->cursor.x; i++)
	px += text_measure_char(p[i]);

      if (p[i])
	w = text_measure_char(p[i]);
      else
	w = 8;

      draw_box1(px, c->window.y + y*14*c->window.scaley,
		px + w,
		c->window.y + (y+1)*14*c->window.scaley, 
		z + 20,
		c->window.ca,c->window.cr,c->window.cg,c->window.cb);
    }
  }

  text_set_escape(oldescape);
  /*   text_set_properties(oldfont,oldsize,oldaspect); */
  
  //  spinlock_unlock(&c->mutex);

}

void csl_window_opaque_render(csl_console_t * c)
{
  if (c->opaque) {
    draw_box2v(c->window.x, c->window.y,
	       c->window.x + c->window.w, c->window.y + c->window.h, 
	       c->window.z,
	       1, c->window.br1, c->window.bg1, c->window.bb1,
	       1, c->window.br2, c->window.bg2, c->window.bb2);
  }
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
  csl_console_t * c = csl_first_console;
  draw_set_clipping4(0, 0, draw_screen_width, draw_screen_height);
  while (c) {
    if (c->render_modes & CSL_RENDER_WINDOW) {
      csl_window_opaque_render(c);
    }
    c = c->next;
  }
}

void csl_window_transparent_render_all()
{
  csl_console_t * c = csl_first_console;

  draw_set_clipping4(0, 0, draw_screen_width, draw_screen_height);

  while (c) {
    if (c->render_modes & CSL_RENDER_WINDOW) {
      csl_window_transparent_render(c);
    }
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
			  float scalex, float scaley, float z, int opaque)
{
  console->window.x = x;
  console->window.y = y;
  console->window.w = w;
  console->window.h = h;
  console->window.scalex = scalex;
  console->window.scaley = scaley;
  console->window.z = z;
  console->opaque = opaque;
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


extern int controler_getchar();
extern int controler_peekchar();

int csl_getchar()
{
  return controler_getchar();
}

int csl_peekchar()
{
  return controler_peekchar();
}

