/**
 * @ingroup dcplaya_muterm_devel
 * @file    mu_termdefs.h
 * @author  benjamin gerard
 * @brief   VT52 terminal code definition
 */

#ifndef __MU_TERMDEFS_H__
#define __MU_TERMDEFS_H__

/** @addtogroup dcplaya_muterm_devel
 *  @{
 */

/** @name terminal key code.
 *  @{
 */
#define MUTERM_ESC      27  /**< escape          */
#define MUTERM_LF       10  /**< line feed       */
#define MUTERM_LF2      11  /**< line feed 2 ??? */
#define MUTERM_LF3      12  /**< line feed 3 ??? */
#define MUTERM_CR       13  /**< carriage return */
#define MUTERM_TAB      9   /**< Tab             */
#define MUTERM_BS       8   /**< Backspace       */
/**@}*/

/** @name VT52 escape sequence.
 * 
 *  This is VT52 escape sequence from the Atari ST gemdos ! I can not tell
 *  if it is real VT52 standard.
 *  @{
 */

/** Cursor move codes. */
#define MT_UP       "\033A"
#define MT_DW       "\033B"
#define MT_RT       "\033C"
#define MT_LT       "\033D"
#define MT_HOME     "\033H"
#define MT_UPS      "\033I"
#define MT_POS(X,Y) "\033Y"Y##X

/** Clear codes. */
#define MT_CLS      "\033E"
#define MT_CLRHOME  "\033d"
#define MT_CLREND   "\033J"
#define MT_CLREOL   "\033K"
#define MT_CLRLINE  "\033l"
#define MT_CLRBOL   "\033o"

/** Deletion / Insertoin codes. */
#define MT_INSLINE  "\033L"
#define MT_DELLINE  "\033M"

/** Color codes. */
#define MT_FGCOL(C)   "\033b"C
#define MT_BKCOL(C)   "\033c"C
#define MT_INVVIDEO   "\033p"
#define MT_NORMVIDEO  "\033q"

#define MT_BLACK      "\00"
#define MT_LGREY      "\01"
#define MT_MGREY      "\02"
#define MT_WHITE      "\03"
#define MT_LYELLOW    "\04"
#define MT_LRED       "\05"
#define MT_MRED       "\06"
#define MT_HRED       "\07"
#define MT_MYELLOW    "\010"
#define MT_LGREEN     "\011"
#define MT_MGREEN     "\012"
#define MT_HGREEN     "\013"
#define MT_HYELLOW    "\014"
#define MT_LBLUE      "\015"
#define MT_MBLUE      "\016"
#define MT_HBLUE      "\017"

/** Show/Hide cursor codes. */
#define MT_CURSON   "\033e"
#define MT_CURSOFF  "\033f"

/** Save/Restore cursor position codes. */
#define MT_CURSAVE   "\033j"
#define MT_CURREST   "\033k"

/** Line wrapping codes. */
#define MT_WRAPON     "\033v"
#define MT_WRAPOFF    "\033w"

/**@}*/

/**@}*/

#endif
