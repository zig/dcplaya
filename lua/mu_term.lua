-- mu_term lua definitions
-- adapted from "mu_termdefs.h"

 MUTERM_ESC      =27
 MUTERM_LF       =10
 MUTERM_LF2      =11
 MUTERM_LF3      =12
 MUTERM_CR       =13
 MUTERM_TAB      =9
 MUTERM_BS       =8

-- This is Gemdos compatible VT52 ESC code

-- Move cursor
 MT_UP	=       "\027A"
 MT_DW	=       "\027B"
 MT_RT	=       "\027C"
 MT_LT	=       "\027D"

 MT_HOME	=     "\027H"
 MT_UPS	=      "\027I"

-- clear
 MT_CLS	=      "\027E"
 MT_CLRHOME	=  "\027d"
 MT_CLREND	=   "\027J"
 MT_CLREOL	=   "\027K"
 MT_CLRLINE	=  "\027l"
 MT_CLRBOL	=   "\027o"

-- Del & insert line
 MT_INSLINE	=  "\027L"
 MT_DELLINE	=  "\027M"

--#define MT_POS(X,Y) "\027Y"Y##X
 MT_POS		=  "\027Y"

-- Set Color
--#define MT_FGCOL(C)   "\027b"C
--#define MT_BKCOL(C)   "\027c"C
 MT_INVVIDEO	=   "\027p"
 MT_NORMVIDEO	=  "\027q"

 MT_BLACK	=      "\00"
 MT_LGREY	=      "\01"
 MT_MGREY	=      "\02"
 MT_WHITE	=      "\03"
 MT_LYELLOW	=    "\04"
 MT_LRED	=       "\05"
 MT_MRED	=       "\06"
 MT_HRED	=       "\07"
 MT_MYELLOW	=    "\010"
 MT_LGREEN	=     "\011"
 MT_MGREEN	=     "\012"
 MT_HGREEN	=     "\013"
 MT_HYELLOW	=    "\014"
 MT_LBLUE	=      "\015"
 MT_MBLUE	=      "\016"
 MT_HBLUE	=      "\017"

-- Show & hide Cursor
 MT_CURSON	=   "\027e"
 MT_CURSOFF	=  "\027f"

-- Save & restore cursor pos
 MT_CURSAVE	=   "\027j"
 MT_CURREST	=   "\027k"

-- activ / desactiv end of line wrap mode
 MT_WRAPON	=     "\027v"
 MT_WRAPOFF	=    "\027w"

mu_term_loaded=1
