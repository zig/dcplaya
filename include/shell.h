/**
 * @file      shell.h
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/12
 * @brief     shell support for dcplaya
 * @version   $Id: shell.h,v 1.1 2002-09-13 16:37:06 zig Exp $
 */


typedef void (* shell_shutdown_func_t)();
typedef int (* shell_command_func_t)(const char * command);

int shell_init();

void shell_shutdown();

/** Update keyboard and console echoing, to be called once per frame (and not more) */
void shell_update(float frameTime);

int shell_command(const char * command);
shell_command_func_t shell_set_command_func(shell_command_func_t func);

/** Load the dynamic part of the shell */
void shell_load(const char * fname);

