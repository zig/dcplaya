/**
 * @ingroup dcplaya_mutermdef_devel
 * @file    mu_termdefs.h
 * @author  benjamin gerard
 * @brief   VT52 terminal code definition
 */

#ifndef __MU_TERMDEFS_H__
#define __MU_TERMDEFS_H__

/** @defgroup dcplaya_mutermdef_devel VT52 codes
 *  @ingroup dcplaya_muterm_devel
 *  @brief VT52 terminal codes and escape sequences.
 * 
 *  This is VT52 escape sequence from the Atari ST gemdos ! I can not tell
 *  if it is real VT52 standard.
 *
 *  @{
 */

/** @name Terminal key code.
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

/** @name Cursor move codes.
 *  @{
 */

#define MT_UP       "\033A" /**< move 1 line up.                */
#define MT_DW       "\033B" /**< move 1 line down.              */
#define MT_RT       "\033C" /**< move 1 column to the right.    */
#define MT_LT       "\033D" /**< move 1 column to the left.     */
#define MT_HOME     "\033H" /**< move top the top left corner.  */
#define MT_UPS      "\033I" /**< move to the top line ?         */
#define MT_POS(X,Y) "\033Y"Y##X /**< set column, line position. */
/**@}*/

/** @name Clear codes.
 *  @{
 */
#define MT_CLS      "\033E" /**< clear screen, cursor move home. */
#define MT_CLRHOME  "\033d" /**< clear from cursor to home. */
#define MT_CLREND   "\033J" /**< clear from cursor to end. */
#define MT_CLREOL   "\033K" /**< clear from cursor to end of line. */
#define MT_CLRLINE  "\033l" /**< clear cursor line. */
#define MT_CLRBOL   "\033o" /**< clear from cursor to start of line */
/**@}*/

/** @name Deletion / Insertion codes.
 *  @{
 */
#define MT_INSLINE  "\033L" /**< insert a line. (above/below ?) cursor line. */
#define MT_DELLINE  "\033M" /**< delete cursor line. */
/**@}*/

/** @name Color codes.
 *  @{
 */
/** Setting cursor, background, text, video inverse and more color stuff. */
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
/**@}*/

/** @name Show/Hide cursor codes.
 *  @{
 */
#define MT_CURSON   "\033e" /**< show cursor. */
#define MT_CURSOFF  "\033f" /**< hide cursor. */
/**@}*/

/** @name Save/Restore cursor position codes.
 *  @{
 */
#define MT_CURSAVE   "\033j" /**< save cursor position. */
#define MT_CURREST   "\033k" /**< restore saved position. */
/**@}*/

/** @name Line wrapping codes.
 *  @{
 */
#define MT_WRAPON     "\033v" /**< enable line wrapping. */
#define MT_WRAPOFF    "\033w" /**< disable line wrapping. */
/**@}*/

/**@}*/

#endif
