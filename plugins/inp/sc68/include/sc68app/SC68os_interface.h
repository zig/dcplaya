/**
 * @ingroup   sc68app_devel
 * @file      SC68os_interface.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1999/05/13
 * @brief     sc68 - graphic interface
 * @version   $Id: SC68os_interface.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *                        sc68 - graphic interface
 *         Copyright (C) 2001 Ben(jamin) Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#ifndef _SC68OS_INTERFACE_H_
#define _SC68OS_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sc68app/SC68app.h"
#include "sc68app/SC68command.h"

typedef int (*SC68os_updatecaller_t)(void *);

/**  Open SC68 interface : OS specific interface ...
 *
 *  -Use app->data for what you want
 *  @return status-code
 *  @retval <0  Error
 *  @retval 0   Not first instance
 *  @retval >0  instance Id
 */
int SC68os_interface_open(char *name, SC68app_t *app);

/**  Close SC68 interface
 *  @param id see SC68os_interface_open
 */
int SC68os_interface_close(SC68app_t *app);

/** Update interface.
 *
 *  Call at each sound update by a timer or whatever you want.
 *
 * @return status-code
 * @retval  <0 error
 * @retval  0  continue
 * @retval  >0 exit-request
 */
int SC68os_interface_update(SC68os_updatecaller_t, SC68app_t *app);

/*  Send a command to this OR another SC68 application.
 */
int SC68os_interface_send(SC68app_t *app, SC68msg_t *sc68msg);

/** Receive a command from this or another SC68 application.
 *
 * @return status-code
 * @retval 0 no error, sc68msg->type updated (SC68COM_nop=no message)
*/
int SC68os_interface_receive(SC68app_t *sc68app, SC68msg_t *sc68msg);

/** Callbacks.
 * @{
 */
int SC68os_interface_load_cb(SC68app_t *sc68app);
int SC68os_interface_eject_cb(SC68app_t *sc68app);
int SC68os_interface_play_cb(SC68app_t *sc68app);
int SC68os_interface_pause_cb(SC68app_t *sc68app);
int SC68os_interface_stop_cb(SC68app_t *sc68app);
int SC68os_interface_ffwd_cb(SC68app_t *sc68app);
int SC68os_interface_track_cb(SC68app_t *sc68app);
/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68OS_INTERFACE_H_ */
