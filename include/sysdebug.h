/**
 * @ingroup    dcplaya
 * @file       sysdebug.h
 * @author     benjamin gerard <ben@sashipa.com>
 * @date       2002/09/04
 * @brief      Debug fonctions.
 *
 * @version    $Id: sysdebug.h,v 1.2 2002-09-12 17:57:31 ben Exp $
 */

#ifndef _SYSDEBUG_H_
#define _SYSDEBUG_H_

#include "config.h"

# if defined(DEBUG) && !defined(DEBUG_LOG) 
#  define DEBUG_LOG
# endif

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
  sysdbg_trace,                 /**< Trace message       */

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

/** Print a debug message.
 *
 *    The sysdbg_printf() functions calls the sysdbg_vprintf() functions with
 *    a suitable va_list parameter.
 *
 * @see sysdbg_vprintf()
 */
void sysdbg_printf(const char * file, int line,
		   int level, const char * fmt, ...);

/** Set debug print function.
 *
 *    The sysdbg_set_function() function allows to change the default debug
 *    message print function.
 *
 * @param    print   The new print function. 0 restores the default function.
 *                   The print function should be a printf-like function.
 * @param    cookie  Value given as 1st parameter (cookie) to the print
 *                   function when it is called by sysdebug_vprintf().
 *
 * @return   Previous debug function
 */
sysdbg_f sysdbg_set_function(sysdbg_f print, void * cookie);

/** Set debug level mask. */
void sysdbg_set_level(int level, int * prev);

/** Bitwise OR on debug level mask. */
void sysdbg_or_level(int level, int * prev);

/** Bitwise NAND (clear) on debug level mask. */
void sysdbg_and_level(int level, int * prev);

/** Change indent level. */
void sysdbg_indent(int indent, int * prev);

/** Register a new debug level. */
void sysdbg_register_level(sysdgb_level_e level,
			   const char *name, const char *twocc);


/** Predefined macros for logging messages.
 *
 *  This macros are only actif if DBG_LOG is defined. To log debug message in
 *  non-debug compilation, use directly the sysdbg_printf() function.
 *
 * @{
 */
# ifdef DEBUG_LOG
#  define SDMSG(level, ...) \
          sysdbg_printf(__FILE__, __LINE__, level, __VA_ARGS__)
# else
#  define SDMSG(level, ...) 
# endif
# define SDCRITICAL(...)   SDMSG(sysdbg_critical, __VA_ARGS)
# define SDERROR(...)      SDMSG(sysdbg_error, __VA_ARGS__)
# define SDWARNING(...)    SDMSG(sysdbg_warning, __VA_ARGS__)
# define SDNOTICE(...)     SDMSG(sysdbg_notice, __VA_ARGS__)
# define SDINFO(...)       SDMSG(sysdbg_info, __VA_ARGS__)
# define SDDEBUG(...)      SDMSG(sysdbg_debug, __VA_ARGS__)
# define SDTRACE(...)      SDMSG(sysdbg_trace, __VA_ARGS__)
# define SDUSER(user, ...) SDMSG((sysdbg_user+user), __VA_ARGS__)
# define SDENTER           if (1) SDGMSG(sysdbg_trace,">> {\n") else
# define SDLEAVE(code)     if (1) SDGMSG(sysdbg_trace,"} << [%d]\n",\
 (int)(code)) else
/**@}*/

DCPLAYA_EXTERN_C_END

#endif  /* #ifndef _SYSDEBUG_H_ */
