/**
 *
 *
 */

#include <stdarg.h>
#include <stdio.h>

/** debug level enum */
typedef enum {
  sysdbg_critical = 0,
  sysdbg_error,
  sysdbg_warning,
  sysdbg_notice,
  sysdbg_info,
  sysdbg_debug,
  sysdbg_user  = 16,
  sysdbg_user0 = sysdbg_user,
  sysdbg_user1,
  sysdbg_user2,
  sysdbg_user3,
  sysdbg_user4,
  sysdbg_user5,
  sysdbg_user6,
  sysdbg_user7,
  sysdbg_user8,
  sysdbg_user9,
  sysdbg_userA,
  sysdbg_userB,
  sysdbg_userC,
  sysdbg_userD,
  sysdbg_userE,
  sysdbg_userF,
} sysdgb_level_e;

/** debug level definition. */
typedef struct {
  int bit;               /**< Level mask */
  char twocc[4];         /**< Level two char code [0 terminated] */
  const char * name;     /**< Level name */
  int reserved;          /**< Reserved must be set to 0 */
} sysdbg_debug_level_t;

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
    {0x00010000, "@0", "critical"},
    {0x00020000, "@1", "error"},
    {0x00040000, "@2", "warning"},
    {0x00080000, "@3", "notice"},
    {0x00100000, "@4", "info"},
    {0x00200000, "@5", "debug"},
    {0x00400000, "@6", unused},
    {0x00800000, "@7", unused},
    {0x01000000, "@8", unused},
    {0x02000000, "@9", unused},
    {0x04000000, "@A", unused},
    {0x08000000, "@B", unused},
    {0x10000000, "@C", unused},
    {0x20000000, "@D", unused},
    {0x40000000, "@E", unused},
    {0x80000000, "@F", unused},
  };

static int sd_level  = sysdbg_critical;  /**< Accepted debug message */

/* Indentation system */
#define SYSDBG_MAX_INDENT (sizeof(tabs)-1);
static int sd_indent = 0;                /**< Current indentation */
static const char tabs[16] = "               ";
static const int max_indent = SYSDBG_MAX_INDENT;

int sysdbg_vprintf(const char * file, int line,
		   int level,  const char * fmt, va_list list)
{
  int col = 0; /* $$$ */

  if ( (unsigned int)level > 31 || ! (sd_level & (1<<level)) ) {
    return 0;
  }

  if (col == 0) {
    int indent = sd_indent;
    
    indent &= ~ (indent >> (sizeof(int)-1));
    if (indent > max_indent) {
      indent = max_indent;
    }

    dbglog(DBG_DEBUG, sd_levels[level].twocc);
    dbglog(DBG_DEBUG, &tabs[max_indent-indent]);
    col = 3 + indent;
  }
  

  return 1;
}

int sysdbg_printf(const char * file, int line,
		  int level, const char * fmt, ...)
{
  va_list list;

  va_start(list, fmt);
  sysdbg_vprintf(file, line, level, fmt, list);
  va_end(list);

  return 0;
}

/** Set debug level mask. */
int sysdbg_level(int level)
{
  int old = sd_level;
  
  sd_level = level;
  return old;
}

/** Change indent level. */
int sysdbg_indent(int indent)
{
  int old = sd_indent;

  sd_indent += indent;
  return old;
}


int sysdbg_register_level(sysdgb_level_e level,
			  const char *name, const char *twocc)
{
  if ( (unsigned int)level > 31 ) {
    return -1;
  }

  sd_levels[level].name = name;
  sd_levels[level].twocc[0] = twocc[0];
  sd_levels[level].twocc[1] = twocc[1];
  sd_levels[level].twocc[2] = ' ';
  sd_levels[level].twocc[3] = 0;
  return 0;
}
