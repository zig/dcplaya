/****************************************************************************
*
* MU : My Utilities
*
* (c) Polygon Studio
*
* by Benjamin Gerard
*
*----------------------------------------------------------------------------
* mu_term.cpp
*----------------------------------------------------------------------------
*
* VT52 Terminal ( ST forever )
*
*****************************************************************************/

/* platform-specific definitions */
/*#include "mu_platform.h" */

#include <stdarg.h>
#include "mu_term.h"
#include "mu_alloc.h"
#include "mu_error.h"

static MUterm_t *activterm=NULL;

/*  Alloc buffers
*/
static MUterm_t *term_alloc(int ncol, int nline)
{
  int size;
  MUterm_t *term;
  size = sizeof(MUterm_t) + (ncol*nline*sizeof(MUterm_char_t));
  if(term=(MUterm_t *)MU_alloc(size), term==NULL)
  {
		MUerror_add("term_alloc() : Failed");
    return NULL;
  }
  memset(term,0,size);
  term->data     = (MUterm_char_t*)(term+1);
  term->dataend  = term->data + nline*ncol;
  term->nline = nline;
  term->ncol  = ncol;
  return term;
}


/*******************
*                  *
* GET CHAR ADDRESS *
*                  *
*******************/

/* Get cursor address in data */
static MUterm_char_t *term_data(MUterm_t *term)
{
  return  term->data +
          term->cursor.x +
          ((term->start_line+term->cursor.y)%term->nline)*term->ncol;
}

/* Get char address of a line */
static MUterm_char_t *term_line_addr(MUterm_t *term, int line)
{
  if(line<0) line = 0;
  line += term->start_line;
  line%=term->nline;
  return term->data + line*term->ncol;
}

/* Get char address of cursor line */
static MUterm_char_t *term_curline_addr(MUterm_t *term)
{
  return term_line_addr(term,term->cursor.y);
}


/****************
*               *
* VARIOUS CLEAR *
*               *
****************/

/* Clear a block a char */
static void char_clr(MUterm_char_t *c,MUterm_char_t *ce, u8 fg_bg)
{
  for(;c<ce;c++)
  {
    c->c = 0;
    c->fg_bg = fg_bg;
  }
}

/* Copy char */
static void char_move(MUterm_char_t *dest, MUterm_char_t *c, MUterm_char_t *ce)
{
  int size;
  size = (int)ce-(int)c;
  if(size<=0)
    return;
  memmove(dest, c, size);
  ce[-1].c = 0;
}

/* Clear all terminal with cursor color */
static void  term_clear(MUterm_t *term)
{
  char_clr(term->data,term->dataend,term->cursor.fg_bg);
}

/* Clear this line */
static void term_clear_line(MUterm_t *term, int line)
{
  MUterm_char_t *c=term_line_addr(term,line);
  char_clr(c,c+term->ncol,term->cursor.fg_bg);
}

/* Clear to Beginning of line */
static void term_clear_bol(MUterm_t *term)
{
  MUterm_char_t *c = term_line_addr(term,term->cursor.y);
  char_clr(c,c+term->cursor.x,term->cursor.fg_bg);
}

/* Clear to End of line */
static void term_clear_eol(MUterm_t *term)
{
  MUterm_char_t *c = term_line_addr(term,term->cursor.y);
  char_clr(c+term->cursor.x,c+term->ncol,term->cursor.fg_bg);
}

/* Clear HOME -> Cursor */
static void term_clear_home(MUterm_t *term)
{
  int y;
  for(y=0; y<term->cursor.y-1; y++)
    term_clear_line(term,y);
  term_clear_bol(term);
}

/* Clear Cursor -> END */
static void term_clear_end(MUterm_t *term)
{
  int y;
  term_clear_eol(term);
  for(y=term->cursor.y+1; y<term->nline; y++)
    term_clear_line(term,y);
}


/* Del Line */
static void term_del_line(MUterm_t *term)
{
  int y;
  MUterm_char_t *c = term_curline_addr(term), *ce;
  for(y=1; y<term->nline; y++)
  {
    ce = term_line_addr(term,y);
    char_move(c,ce,ce+term->ncol);
    c = ce;
  }
  char_clr(c,c+term->ncol,term->cursor.fg_bg);
}

/* Ins Line */
static void term_ins_line(MUterm_t *term)
{
  int y;
  MUterm_char_t *c = term_line_addr(term,term->nline-1), *ce;
  for(y=term->nline-2; y>=0; y--)
  {
    ce = term_line_addr(term,y);
    char_move(c,ce,ce+term->ncol);
    c = ce;
  }
  char_clr(c,c+term->ncol,term->cursor.fg_bg);
}

/*********
*        *
* SCROLL *
*        *
*********/


/* Scroll 1 line UP */
static void term_scroll(MUterm_t *term)
{
  term->start_line = (term->start_line+1) % term->nline;
  term_clear_line( term, term->nline-1 );
}

/* Scroll 1 line DW */
static void term_scroll_down(MUterm_t *term)
{
  if(term->start_line)
    term->start_line--;
  else
    term->start_line = term->nline-1;
  term_clear_line( term, 0 );
}

/* Delete cursor char => scroll left */
static void term_del_char(MUterm_t *term)
{
  MUterm_char_t *c=term_data(term);
  char_move(c, c+1, term_line_addr(term,term->cursor.y)+term->ncol);
}


/****************
*               *
* MOVING CURSOR *
*               *
****************/

/* HOME cursor */
static void term_home(MUterm_t *term)
{
  term->cursor.x = term->cursor.y = 0;
}

/*  Move cursor DOWN, scroll if necessary
*/
static void term_cursor_down(MUterm_t *term)
{
  if(term->cursor.y==term->nline-1)
  {
    term->start_line++;
    if(term->start_line==term->nline) term->start_line = 0;
    term_clear_line(term, term->cursor.y);
  }
  else term->cursor.y++;
}

/*  Move cursor RIGHT, wrap if necessary
*/
static void term_cursor_right(MUterm_t *term)
{
  if(term->cursor.x==term->ncol-1)
  {
    if(term->mode.wrap)
    {
      term->cursor.x = 0;
      term_cursor_down(term);
    }
  }
  else term->cursor.x++;
}

/*  Move cursor at this "clipped" position
*/
static void term_cursor_at(MUterm_t *term, int const x, int const y)
{
  if(x<0)
    term->cursor.x = 0;
  else if(x>=term->ncol)
    term->cursor.x = term->ncol-1;
  else
    term->cursor.x = x;

  if(y<0)
    term->cursor.y = 0;
  else if(y>=term->ncol)
    term->cursor.y = term->ncol-1;
  else
    term->cursor.y = y;
}

/*  relative cursor move
*/
static void term_move_cursor(MUterm_t *term, int stepx, int stepy)
{
  term_cursor_at(term,term->cursor.x+stepx,term->cursor.y+stepy);
}


/*************
*            *
* CHAR INPUT *
*            *
*************/

/*  Receiving a caractere
*/
static void term_input(MUterm_t *term, char const a)
{
  MUterm_char_t *p = term_data(term);
  if(term->mode.insert)
  {
    int n = term->ncol-term->cursor.x-1;
    if(n>0) memmove(p+1,p,n*sizeof(*p));
  }
  p->c = (a>=32) ? (a-32)&127 : 0;
  p->fg_bg = (term->mode.invvideo) ?
    ((term->cursor.fg_bg>>4)+(term->cursor.fg_bg<<4)) : term->cursor.fg_bg;
  if( term->mode.step )
    term_cursor_right(term);
}

static void term_set_escape_command(MUterm_t *term, char const a)
{
  term->escape = 0;
  term->command  = a;  // Default to command
  term->expected = 0;  // Default : no parameters
  switch( a )
  {
  // Move Cursor
  case 'A':
    term_move_cursor(term,0,-1);
    break;
  case 'B':
    term_move_cursor(term,0,1);
    break;
  case 'C':
    term_move_cursor(term,1,0);
    break;
  case 'D':
    term_move_cursor(term,-1,0);
    break;
  // Cursor Up & scroll
  case 'I':
    if(term->cursor.y==0)
    {
      term_scroll_down(term);
      term_clear_line(term,0);
    }
    else
      term->cursor.y--;
    break;

  // CLS & HOME
  case 'E':
    term_clear(term);
  case 'H':
    term_home(term);
    break;
  // Clear Home
  case 'd':
    term_clear_home(term);
    break;
  // Clear Line
  case 'l':
    term_clear_line(term,term->cursor.y);
    break;
  // Clear to Beginning of line
  case 'o':
    term_clear_bol(term);
    break;
  // Clear to End of line
  case 'K':
    term_clear_eol(term);
    break;
  // Clear to End of screen
  case 'J':
    term_clear_end(term);
    break;

  // Delete & Insert line
  case 'M':
    term_del_line(term);
    break;
  case 'L':
    term_ins_line(term);
    break;

  // Set foreground/background color
  case 'b': case 'c':
    term->expected=1;
    break;
  // Video inverse
  case 'p': case 'q':
    term->mode.invvideo = 'q'-a;
    break;
  // Cursor Hide/Show
  case 'e': case 'f':
    term->mode.hidden = 'f'-a;
    break;
  // Line Wrap
  case 'v': case 'w':
    term->mode.wrap = 'w'-a;
    break;
  // Save cursor position
  case 'j':
    term->cursor.sx = term->cursor.x;
    term->cursor.sy = term->cursor.y;
    break;
  // Restore cursor position
  case 'k':
    term->cursor.x = term->cursor.sx;
    term->cursor.y = term->cursor.sy;
    break;

  // Locate(X,Y)
  case 'Y':
    term->expected=2;
    break;

  default:
    term->expected = term->command = 0;
  }
}

static void term_exec_escape_command(MUterm_t *term, char const a)
{
  switch( term->command )
  {

  // Set FOREGROUND/BACKGROUND COLOR
  case 'b': case 'c':
    {
      int dec = (term->command-'b')*4;
      term->cursor.fg_bg &= 0x0f<<dec;
      term->cursor.fg_bg |= ((a&15)<<(dec^4));
    }
    break;

  // Locate(X,Y)
  case 'Y':
    if(term->expected==2)
      term->cursor.y = a-32;
    else
    {
      term->cursor.x = a-32;
      term_cursor_at(term,term->cursor.x,term->cursor.y);
    }
    break;
  }
  --term->expected;
}

/* Return number of char auto-read
*/
static int auto_read(MUterm_t *term)
{
  int nb=0,c;
  int (*fnct)(void);
  if(fnct=term->getchar, fnct==NULL) return 0;

  while(c=(*fnct)(), c>=0)
  {
    if( term->mode.localecho )
      MUterm_inputc(c,term);
    nb++;
  }
  return nb;
}

/*  Create and alloc a terminal ( n lines, n cols )
 */
MUterm_t *MUterm_create(int ncol, int nline, int donotsetactiv)
{
  MUterm_t *term;

  /* Check parameters */
  if(ncol<=0 || ncol>256 || nline<=0 || nline>256)
  {
    MUerror_add("Can't create %dx%d terminal",ncol,nline);
    return NULL;
  }

  /* Alloc Memory for terminal */
  if(term=term_alloc(ncol,nline), term==NULL)
    return NULL;

  /* reset terminal too default */
  if(MUterm_reset(term)<0)
    return NULL;

  if( !donotsetactiv ) activterm = term;
  return term;
}

/*  Set function that auto-get a char from a stream
 */
int MUterm_setstream( int(*newreader)(void), MUterm_t *term)
{
  if(term==NULL)
    if(term=activterm, term==NULL)
      return 0;
  term->getchar = newreader;
  return 0;
}



/*  Kill a terminal : Clean free
*/
void MUterm_kill(MUterm_t *term)
{
  if(term==NULL)
    return;
  if(term==activterm)
    activterm=NULL;
  MU_free(term);
}

/*  Reset a terminal : Clear, Home, Setcolor
 */
int MUterm_reset(MUterm_t *term)
{
  if(term==NULL)
  {
    term = activterm;
    if(term==NULL) return 0;
  }
  term->escape = term->command = term->expected = 0;
  term->cursor.fg_bg = 0x30;
  term->cursor.sx = 0;
  term->cursor.sy = 0;
  term->start_line = 0;
  term->mode.wrap      = 1;
  term->mode.step      = 1;
  term->mode.insert    = 1;
  term->mode.invvideo  = 0;
  term->mode.hidden    = 0;
  term->mode.localecho = 1;
  term->mode.lf_crlf   = 1;
  term_clear(term);
  term_home(term);
  return 0;
}

/*  Set default active terminal
 *  return : previous active terminal
*/
MUterm_t *MUterm_setactive(MUterm_t *term)
{
  MUterm_t *oldterm = activterm;
  activterm = term;
  return oldterm;
}

/*  Get default active terminal
*/
MUterm_t *MUterm_getactive(void)
{
  return activterm;
}

/*  Update : read user stream
*/
int MUterm_update(MUterm_t *term)
{
  if(term==NULL)
    term = activterm;
  if(term==NULL)
     return 0;
  auto_read(term);
  return 0;
}

/*  Terminal recieve a CHAR.
 *  term==NULL is activ-term
 */
void MUterm_inputc(char a, MUterm_t *term)
{
  if(term==NULL)
  {
    term = activterm;
    if(term==NULL) return;
  }

  /* Previous was escape, Need a command */
  if(term->escape)
  {
    term_set_escape_command(term, a);
  }
  /* Char expected by command */
  else if(term->expected)
  {
    term_exec_escape_command(term,a);
    return;
  }
  else
  {
    switch(a)
    {
    case 0:
      break;
    case MUTERM_ESC:
      /* Store esc as previous for next input */
      term->escape = 1;
      break;
    case MUTERM_LF: case MUTERM_LF2: case MUTERM_LF3:
      term_cursor_down(term);
      if(!term->mode.lf_crlf)
        break;
    case MUTERM_CR:
      term->cursor.x=0;
      break;
    case MUTERM_TAB:
      term_input(term,' ');
      break;
    case MUTERM_BS:
      term_move_cursor(term,-1,0);
      term_del_char(term);
      break;

    default:
      term_input(term,a);
    }
  }
}

void MUterm_input(const char *a, MUterm_t *term)
{
  if(term==NULL)
  {
    term = activterm;
    if(term==NULL) return;
  }
  auto_read(activterm);
  for(; *a || term->expected ; MUterm_inputc(*a++,term));
}

/*  Send VT52 clear screen string
 */
void MUterm_cls(MUterm_t *term)
{
  static char s[]="\033E";
  MUterm_input(s,term);
}

/*  Send a VT52 cursor location string
*/
void MUterm_locate(int x, int y, MUterm_t *term)
{
  static char loc[]="\033Y  ";
  if(term==NULL)
  {
    term = activterm;
    if(term==NULL) return;
  }
  if(x<0) x=0;
  else if(x>=term->ncol) x=term->ncol-1;
  if(y<0) y=0;
  else if(y>=term->nline) y=term->nline-1;
  loc[2] = y+32;
  loc[3] = x+32;
  MUterm_input(loc,term);
}

/*  Send a VT52 color setting string
*/
void MUterm_setcolor(int fg, int bg, MUterm_t *term)
{
  static char s[]="\033b \033c ";
  fg &= 15;
  bg &= 15;
  s[2] = fg;
  s[5] = bg;
  MUterm_input(s,term);
}

/*  Save cursor position ( only one position could be stored )
*/
void MUterm_savecursor(MUterm_t *term)
{
  MUterm_input(MT_CURSAVE,term);
}

/*  Restore stored position (MUterm_savecursor
*/
void MUterm_restorecursor(MUterm_t *term)
{
  MUterm_input(MT_CURREST,term);
}

/*  Write a printf like formated string into aciv-terminal
*/
void MUterm_printf( const char *fmt, ... )
{
  char tmp[2048];
  va_list args;
  va_start(args, fmt);
  vsprintf(tmp, fmt, args);
  va_end(args);
  MUterm_input(tmp, 0);
}

void MUterm_vprintf( const char *fmt, va_list args )
{
  char tmp[2048];
  vsprintf(tmp, fmt, args);
  MUterm_input(tmp, 0);
}
