/**
 * @ingroup    dcplaya
 * @file       sysdebug.c
 * @author     benjamin gerard <ben@sashipa.com>
 * @date       2002/09/04
 * @brief      Debug fonctions.
 *
 * @version    $Id: sysdebug.c,v 1.7 2002-09-13 14:48:25 ben Exp $
 */

#include <stdarg.h>
#include <stdio.h>
#include "sysdebug.h"

/* From modified kos 1.1.5 */
extern void dbglogv(int level, const char *fmt, va_list args);

/** Default user message level mask (accept all messages) */
#define SD_DEFAULT_LEVEL -1

/** The SD_GLOBAL_LEVEL is a compile time message level mask. */
#if defined(RELEASE)
# define SD_GLOBAL_LEVEL ((int)sysdbg_critical)
#else
# define SD_GLOBAL_LEVEL -1
#endif

static const char unused[] = "unused";

static sysdbg_debug_level_t sd_levels[32] =
  {
    /* System defined level */
    {0x00000001, "!! ", "critical"},
    {0x00000002, "$$ ", "error"},
    {0x00000004, "%% ", "warning"},
    {0x00000008, "## ", "notice"},
    {0x00000010, "ii ", "info"},
    {0x00000020, ":: ", "debug"},
    {0x00000040, ".. ", unused},
    {0x00000080, ".. ", unused},

    {0x00000100, ".. ", unused},
    {0x00000200, ".. ", unused},
    {0x00000400, ".. ", unused},
    {0x00000800, ".. ", unused},
    {0x00001000, ".. ", unused},
    {0x00002000, ".. ", unused},
    {0x00004000, ".. ", unused},
    {0x00008000, ".. ", unused},

    /* user defined level */
    {0x00010000, "@0 ", "user0"},
    {0x00020000, "@1 ", "user1"},
    {0x00040000, "@2 ", "user2"},
    {0x00080000, "@3 ", "user3"},
    {0x00100000, "@4 ", "user4"},
    {0x00200000, "@5 ", "user5"},
    {0x00400000, "@6 ", "user6"},
    {0x00800000, "@7 ", "user7"},
    {0x01000000, "@8 ", "user8"},
    {0x02000000, "@9 ", "user9"},
    {0x04000000, "@A ", "userA"},
    {0x08000000, "@B ", "userB"},
    {0x10000000, "@C ", "userC"},
    {0x20000000, "@D ", "userD"},
    {0x40000000, "@E ", "userE"},
    {0x80000000, "@F ", "userF"},
  };

/** User defined accepted debug message levels. */
static int sd_level  = SD_DEFAULT_LEVEL;
/** Compilation defined accepted debug message levels. */  
static const int sd_global_level = SD_GLOBAL_LEVEL;

/* Indentation system */
#define SYSDBG_MAX_INDENT (sizeof(tabs)-1);
static const char tabs[16] = "               ";
static const int max_indent = SYSDBG_MAX_INDENT;
static int sd_indent = 0; /**< Current indentation */
static int sd_col = 0;    /**< Curent column */

/* Default vprintf like function. */
static void sd_default_vprintf(void * cookie,
				   const char * fmt, va_list list)
{
  dbglogv((int)cookie, fmt, list);
}

static sysdbg_f sd_current = sd_default_vprintf;
static void * sd_cookie = 0;

static void sd_print_location(const char * fmt, ...)
{
  va_list list;

  va_start(list, fmt);
  sd_current(sd_cookie, fmt, list);
  va_end(list);
}

void sysdbg_vprintf(const char * file, int line, int level,
		    const char * fmt, va_list list)
{
  int len, nl_cnt;

  /* Wrong or masked level are simply ignored */
  if ( (unsigned int)level > 31
       || ! (sd_level & (1<<level) & sd_global_level)) {
    return;
  }

  /* Count, skip and print '\n' at start of format string. */
  for (nl_cnt = 0; *fmt == '\n'; ++nl_cnt, ++fmt) {
    sd_current(sd_cookie, "\n", list);
    sd_col = 0;
  }

  if (sd_col == 0) {
    /* Printing on first column : add extra info */
    int indent = sd_indent;
    
    /* Prevent under/overflow in indent string */
    if (indent < 0) {
      indent = 0;
    } else if (indent > max_indent) {
      indent = max_indent;
    }

    /* Print Two-Char code */
    sd_current(sd_cookie, sd_levels[level].twocc, list);

    if (!indent) {
      /* Print "file:line: " if no indentation requested */
      sd_print_location("%s:%d: ",file, line);
    } else {
      /* Or print indentation string */
      sd_current(sd_cookie, &tabs[max_indent-indent], list);
    }

    sd_col = 1;
  }

  /* Write *REAL* debug message a*/
  sd_current(sd_cookie, fmt, list);

  /* Reset column if last char is '\n' */
  for (len=0; fmt[len]; ++len)
    ;
  if (len > 0 && fmt[len-1]=='\n') {
    sd_col = 0;
  }

}

void sysdbg_printf(const char * file, int line,
		  int level, const char * fmt, ...)
{
  va_list list;

  va_start(list, fmt);
  sysdbg_vprintf(file, line, level, fmt, list);
  va_end(list);
}

sysdbg_f sysdbg_set_function(sysdbg_f print, void * cookie)
{
  sysdbg_f save = sd_current;

  sd_current = print ? print : sd_default_vprintf;
  sd_cookie  = cookie;
  return save;
}

/** Set debug level mask. */
void sysdbg_set_level(int level, int * prev)
{
  if (prev) {
    *prev = sd_level;
  }
  sd_level = level;
}

void sysdbg_or_level(int level, int * prev)
{
  if (prev) {
    *prev = sd_level;
  }
  sd_level |= level;
}

void sysdbg_and_level(int level, int * prev)
{
  if (prev) {
    *prev = sd_level;
  }
  sd_level &= ~level;
}


/** Change indent level. */
void sysdbg_indent(int indent, int * prev)
{
  if (prev) {
    *prev = sd_indent;
  }
  sd_indent += indent;
}

/** Register a new debug level. */
void sysdbg_register_level(sysdgb_level_e level,
			   const char *name, const char *twocc)
{
  if ( (unsigned int)level > 31 ) {
    return;
  }

  sd_levels[level].name = name;
  sd_levels[level].twocc[0] = twocc[0];
  sd_levels[level].twocc[1] = twocc[1];
  sd_levels[level].twocc[2] = ' ';
  sd_levels[level].twocc[3] = 0;
}

