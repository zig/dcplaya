/**
 * @file      shell.h
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/12
 * @brief     shell support for dcplaya
 * @version   $Id: shell.h,v 1.3 2002-09-14 06:57:36 zig Exp $
 */


typedef void (* shell_shutdown_func_t)();
typedef int (* shell_command_func_t)(const char * command, ...);

int shell_init();

void shell_shutdown();

/** Update keyboard and console echoing, to be called once per frame (and not more) */
void shell_update(float frameTime);

/** Issue the given command on currently loaded shell */
int shell_command(const char * command);

/** Set the shell command handler */
shell_command_func_t shell_set_command_func(shell_command_func_t func);

/** Load the dynamic part of the shell */
void shell_load(const char * fname);

/** Wait all shells command are treated 
 *
 *  @warning: do not call this from within the shell thread !
 *
 */
void shell_wait();