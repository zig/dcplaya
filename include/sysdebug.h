/**
 * @ingroup    dcplaya
 * @file       sysdebug.h
 * @author     benjamin gerard <ben@sashipa.com>
 * @date       2002/09/04
 * @brief      Debug fonctions.
 *
 * @version    $Id: sysdebug.h,v 1.1 2002-09-10 12:58:10 ben Exp $
 */

#ifndef _SYSDEBUG_H_
# define _SYSDEBUG_H_
# ifdef DEBUG
# include "extern_def.h"

DCPLAYA_EXTERN_C_START

# include <stdarg.h>

/** debug level enum. */
typedef enum {

  sysdbg_critical = 0,          /**< Critical message    */
  sysdbg_error,                 /**< Error message       */
  sysdbg_warning,               /**< Warning message     */
  sysdbg_notice,                /**< Notice message      */
  sysdbg_info,                  /**< Information message */
  sysdbg_debug,                 /**< Debug message       */

  sysdbg_user  = 16,            /**< First user message  */
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

/** Debug level definition. */
typedef struct {
  int bit;               /**< Level mask                         */
  char twocc[4];         /**< Level two char code [0 terminated] */
  const char * name;     /**< Level name                         */
  int reserved;          /**< Reserved must be set to 0          */
} sysdbg_debug_level_t;

/** Debug function type definition */
typedef void (*sysdbg_f)(void * cookie,
			const char *fmt, va_list list);

/** Print a debug message (variable argument version). */
void sysdbg_vprintf(const char * file, int line, int level,
		    const char * fmt, va_list list);

/** Print a debug message. */
void sysdbg_printf(const char * file, int line,
		  int level, const char * fmt, ...)

/** Set debug level mask. */
void sysdbg_level(int level, int * prev);

/** Change indent level. */
void sysdbg_indent(int indent, int * prev);

/** Register a new debug level. */
void sysdbg_register_level(sysdgb_level_e level,
			   const char *name, const char *twocc);


DCPLAYA_EXTERN_C_END

# else /* #ifdef DEBUG */

#  define sysdbg_vprintf(...)
#  define sysdbg_printf(...)
#  define sysdbg_level(level)
#  define sysdbg_indent(indent)
#  define sysdbg_register_level(level,name,twocc)

# endif /* #ifdef DEBUG */

# define SDMSG(level, ...) \
         sysdbg_printf(__FILE__, __LINE__, level, __VA_ARGS__)

# define SDCRITICAL(...)   SDMSG(sysdbg_critical, __VA_ARGS)
# define SDERROR(...)      SDMSG(sysdbg_error, __VA_ARGS__)
# define SDWARNING(...)    SDMSG(sysdbg_warning, __VA_ARGS__)
# define SDNOTICE(...)     SDMSG(sysdbg_notice, __VA_ARGS__)
# define SDINFO(...)       SDMSG(sysdbg_info, __VA_ARGS__)
# define SDDEBUG(...)      SDMSG(sysdbg_debug, __VA_ARGS__)
# define SDUSER(user, ...) SDMSG((sysdbg_user+user), __VA_ARGS__)

#endif  /* #ifndef _SYSDEBUG_H_ */
