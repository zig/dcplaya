/**
 * @ingroup   sc68app_devel
 * @file      SC68config.h
 * @author    Ben(jamin) Gerard<ben@sashipa.com>
 * @date      1999/07/27
 * @brief     sc68 - config file
 * @version   $Id: SC68config.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *                            sc68 - Config file
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

#ifndef _SC68CONFIG_H_
#define _SC68CONFIG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Config file entry structure. */
typedef struct
{
  char *name;    /**< Config entry name */
  char *comment; /**< No comment */
  int  min,max;  /**< min and max value */
  int  def;      /**< default value */
} SC68config_entry_t;

/** Config array. */
typedef int SC68config_t[64];

/** Check config values and correct invalid ones.
 */
int SC68config_valid();

/** Get indice of named field in SC68config_t array.
 *  @retval <0 field doesn't exist
 */
int SC68config_get_id(char *name);

/** Output config to FILE.
 */
int SC68config_write(FILE *f, SC68config_t *conf);

/** Output config to stdout.
 */
int SC68config_display(SC68config_t *conf);

/** Load config from file.
 */
int SC68config_load(SC68config_t *conf, char *name);

/** Save config into file.
 */
int SC68config_save(SC68config_t *conf, char *name);

/** Fill config struct with default value.
 */
int SC68config_default(SC68config_t *conf);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68CONFIG_H_ */
