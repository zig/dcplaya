/**
 * @ingroup dcplaya_muterm_devel
 * @file    mu_term.h
 * @author  benjamin gerard
 * @brief  VT52 terminal
 */

#ifndef __MU_TERM_H__
#define __MU_TERM_H__

#include <stdarg.h>

/** @defgroup dcplaya_muterm_devel VT52 terminal
 *  @ingroup dcplaya_devel
 *  @brief  VT52 compatible terminal
 *  @author  benjamin gerard
 *  @{
 */

#include "mu_termdefs.h"
#include "my_types.h"

/** Terminal character */
typedef struct MUterm_char
{
  char  c;       /**< code.                           */
  u8    fg_bg;   /**< forground and background color. */
} MUterm_char_t;

/** Terminal. */
typedef struct MUterm
{
  MUterm_char_t *data;    /**< Buffer             */
  MUterm_char_t *dataend; /**< End of char buffer */

  int ncol,nline;         /**< Buffer size in line & columns */
  int start_line;         /**< First line (y=0) in buffer    */

  /** Cursor information. */
  struct
  {
    u8    x,y;    /**< Cursor current colonnes / lines */
    u8    fg_bg;  /**< Current colors ( FG,BG )        */
    u8    sx,sy;  /**< Saved position                  */
  }cursor;

  /** Terminal status. */
  struct
  {
    unsigned wrap:1;      /**< Last column wrap to next line */
    unsigned step:1;      /**< Input advance cursor          */
    unsigned insert:1;    /**< insert at input               */
    unsigned invvideo:1;  /**< invert video                  */
    unsigned hidden:1;    /**< cursor hidden                 */
    unsigned localecho:1; /**< input are output too          */
    unsigned lf_crlf:1;   /**< Lf char is LF&CR              */
  } mode;

  int escape;     /**< True if previous char was escape */
  int expected;   /**< True if a command                */
  int command;    /**< Current command on exec          */
  
  int (*getchar)(void); /**< Get char function : NULL for none */

} MUterm_t;

/** Create and alloc a terminal ( n lines, n cols ) and set as active.
 */
MUterm_t *MUterm_create(int ncol, int nline, int donotsetactiv);

/**  Set function that auto-get a char from a stream.
 */
int MUterm_setstream( int(*newreader)(void), MUterm_t *term);

/** Set default active terminal.
 *  @return previous active terminal
 */
MUterm_t *MUterm_setactive(MUterm_t *term);

/** Get default active terminal.
 */
MUterm_t *MUterm_getactive(void);

/** Kill a terminal : Clean free.
 */
void MUterm_kill(MUterm_t *term);

/** Reset a terminal : Clear, Home, Setcolor.
 */
int MUterm_reset(MUterm_t *term);

/** Update : read user stream.
 */
int MUterm_update(MUterm_t *term);

/** Write a caractere.
 */
void MUterm_inputc(char a, MUterm_t *term);

/** Write a 0 terminated string to terminal.
 */
void MUterm_input(const char *a, MUterm_t *term);

/** Send a VT52 cursor location string.
 */
void MUterm_locate(int x, int y, MUterm_t *term);

/** Send VT52 clear screen string.
 */
void MUterm_cls(MUterm_t *term);

/** Send a VT52 color setting string.
 */
void MUterm_setcolor(int fg, int bg, MUterm_t *term);

/** Save cursor position ( only one position could be stored ).
 */
void MUterm_savecursor(MUterm_t *term);

/** Restore stored position (MUterm_savecursor.
 */
void MUterm_restorecursor(MUterm_t *term);

/** Write a printf like formated string into aciv-terminal.
 */
void MUterm_printf( const char *fmt, ... );
void MUterm_vprintf( const char *fmt, va_list args);

/**@*/

#endif
