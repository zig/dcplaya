/**
 * @ingroup   dcplaya_shell_devel
 * @file      shell.h
 * @author    vincent penne
 * @date      2002/09/12
 * @brief     shell support for dcplaya
 * @version   $Id: shell.h,v 1.8 2003-03-29 15:33:06 ben Exp $
 */

/** @defgroup dcplaya_shell_devel Shell
 *  @ingroup  dcplaya_devel
 *  @brief    shell
 *  @author   vincent penne
 *  @{
 */

/** shell shutdown fucntion. */
typedef void (* shell_shutdown_func_t)();
/** shell output command fucntion. */
typedef int (* shell_command_func_t)(const char * command, ...);

/** initialize shell. */
int shell_init();

/** shutdown shell. */
void shell_shutdown();

/** Update shell console position on screen.
 *  @param  frameTime  elapsed time (in sec ?)
 */
void shell_update(float frameTime);

/** Issue the given command on currently loaded shell.
 *  @param command command string
 */
int shell_command(const char * command);

/** Set the shell command handler.
 *  @param func new command handler.
 *  @return old command handler ?
 */
shell_command_func_t shell_set_command_func(shell_command_func_t func);

/** Load the dynamic part of the shell.
 *  @param fname path to an executable plugin (lef) to load
 *  @see dcplaya_exe_plugin_devel
 *  @see dcplaya_lef_devel
 */
void shell_load(const char * fname);

/** Wait all shells command are treated 
 *  @warning: do not call this from within the shell thread !
 */
void shell_wait();

/** @name Console visibility
 *  @{
 */

/** Toggle console visible state.
 *  @return old state
 */
int shell_toggleconsole();

/** Show console.
 *  @return old state
 */
int shell_showconsole();

/** Hide console.
 *  @return old state
 */
int shell_hideconsole();

/**@}*/

/**@}*/

