/**
 * @ingroup   sc68app_devel
 * @file      SC68.h
 * @author    Ben(jamin) Gerard <ben@sashipa.com>
 * @date      1998/10/07
 * @brief     Main definitions
 * @version   $Id: SC68.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *                         sc68 - main definitions
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

#ifndef _SC68_H_
#define _SC68_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Version */
#define SC68_VERH 0x01
#define SC68_VERL 0x00

#define SC68_DEFAULT_SAMPLING_RATE  44100

#define SC68_DEFAULT_BIDB_FILE      "BIDB"
#define SC68_DEFAULT_CONFIG_FILE    "config"

#define SC68_MAX_TIME               (99*60+59)

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _SC68_H_ */
