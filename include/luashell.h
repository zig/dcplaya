/**
 * @file      luashell.h
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/09/22
 * @brief     lua shell support for dcplaya
 * @version   $Id: luashell.h,v 1.1 2002-09-24 13:47:04 vincentp Exp $
 */

#ifndef _LUASHELL_H_
#define _LUASHELL_H_

#include "lua.h"

enum luashell_command_type_t {
  SHELL_COMMAND_LUA,
  SHELL_COMMAND_C,
};

typedef struct luashell_command_description {
  /* beginning of static variables */
  char * name;       ///< long name of the command
  char * short_name; ///< short name of the command
  char * usage;      ///< lua command that should print usage of the command
  int  type;         ///< type of command : LUA or C
  lua_CFunction function; ///< function to call (or pointer on lua function in string format)

  /* beginning of dynamic variables */
  int registered;    ///< is this command registered ?
} luashell_command_description_t;

#endif /* #ifdef _LUASHELL_H_ */
