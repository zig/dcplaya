/**
 * @file    songmenu.h
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/02/10
 * @brief   file and playlist browser
 * 
 * $Id: songmenu.h,v 1.3 2002-09-20 13:42:41 benjihan Exp $
 */

#ifndef _SONGMENU_H_
#define _SONGMENU_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

#define WIN68_DIR_X       42.0f  /**< X position of file browser window.     */
#define WIN68_PLAYLIST_X  345.0f /**< X position of playlist browser window. */
#define WIN68_Y           122.0f /**< Y position of both browser window.     */

/** songmenu first init.
 *
 *   The songmenu_init() function initializes window, entrylist and more
 *   things.
 *
 * @return error code
 * @retval  0 success
 * @retval <0 error
 */
int songmenu_init(void);

/** songmenu start.
 *
 *    The songmenu_start() function runs the directory loader thread.
 *
 * @return error code
 * @retval  0 success
 * @retval <0 error
 */ 
int songmenu_start(void);

/** songmenu clean exit.
 *
 *    The songmenu_kill() function stops loader thread and free any allocated
 *    buffers.
 *
 * @return error code
 * @retval  0 success
 * @retval <0 error
 */
int songmenu_kill(void);

/** songmenu render.
 *
 *     The songmenu_render() function displays both file and playlist browser
 *     windows. This function must be call while rendering translucent list.
 *
 * @return error code
 * @retval  0 success
 * @retval <0 error
 *
 * @warning This function must be call while rendering translucent list.
 */
void songmenu_render(int elapsed);

/** Get songmenu current window.
 *
 *     The songmenu_current_window() returns the current songmenu active
 *     window.
 *
 *  @return current songmenu active window
 *  @retval  0  file browser window
 *  @retval  1  playlist browser window
 */
int songmenu_current_window(void);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _SONGMENU_H_ */
