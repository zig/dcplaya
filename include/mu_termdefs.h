/****************************************************************************
*
* MU : My Utilities
*
* (c) Polygon Studio
*
* by Benjamin Gerard
*
*----------------------------------------------------------------------------
* mu_termdefs.h
*----------------------------------------------------------------------------
*
* VT52 Terminal : Caractere code definition
*
*****************************************************************************/

#ifndef __MU_TERMDEFS_H__
#define __MU_TERMDEFS_H__

/*
#ifndef __MU__PLATFORM_SPECIFIC__H__
# error "MU_platform.h MUST be included"
#endif
*/

#define MUTERM_ESC      27
#define MUTERM_LF       10
#define MUTERM_LF2      11
#define MUTERM_LF3      12
#define MUTERM_CR       13
#define MUTERM_TAB      9
#define MUTERM_BS       8

// This is Gemdos compatible VT52 ESC code

// Move cursor
#define MT_UP       "\033A"
#define MT_DW       "\033B"
#define MT_RT       "\033C"
#define MT_LT       "\033D"

#define MT_HOME     "\033H"
#define MT_UPS      "\033I"

// clear
#define MT_CLS      "\033E"
#define MT_CLRHOME  "\033d"
#define MT_CLREND   "\033J"
#define MT_CLREOL   "\033K"
#define MT_CLRLINE  "\033l"
#define MT_CLRBOL   "\033o"

// Del & insert line
#define MT_INSLINE  "\033L"
#define MT_DELLINE  "\033M"

#define MT_POS(X,Y) "\033Y"Y##X

// Set Color
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

// Show & hide Cursor
#define MT_CURSON   "\033e"
#define MT_CURSOFF  "\033f"

// Save & restore cursor pos
#define MT_CURSAVE   "\033j"
#define MT_CURREST   "\033k"

// activ / desactiv end of line wrap mode
#define MT_WRAPON     "\033v"
#define MT_WRAPOFF    "\033w"

#endif
