/**
 * @ingroup   dcplaya_luashell_devel
 * @file      luashell.h
 * @author    vincent penne
 * @date      2002/09/22
 * @brief     lua shell support for dcplaya
 * @version   $Id: luashell.h,v 1.2 2003-03-26 23:02:47 ben Exp $
 */

#ifndef _LUASHELL_H_
#define _LUASHELL_H_

#include "lua.h"


/** @defgroup  dcplaya_luashell_devel  LUA shell
 *  @ingroup   dcplaya_shell_devel
 *  @brief     lua shell
 *  @author    vincent penne
 *  @{
 */

/** Command type. */
typedef enum {
  SHELL_COMMAND_LUA, /**< Command is a LUA function. */
  SHELL_COMMAND_C,   /**< Command is a C function.   */
} luashell_command_type_t;

/** Shell command description. */
typedef struct luashell_command_description {
  /* beginning of static variables */

  /** long name of the command (mandatory). */
  char * name;
  /** short name of the command (optionnal). */
  char * short_name;
  /** lua command that should print usage of the command. */
  char * usage;
  /** type of command (LUA or C). */
  luashell_command_type_t  type; 
  /** function to call (or pointer on lua function in string format). */
  lua_CFunction function; 

  /* beginning of dynamic variables */
  /** Is this command registered ?.
   *  I dunno what the hell it is ! Ask ziggy for more info.
   */
  int registered;    
} luashell_command_description_t;

/**@}*/

#endif /* #ifdef _LUASHELL_H_ */
