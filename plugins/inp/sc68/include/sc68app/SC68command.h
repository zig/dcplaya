/**
 * @ingroup   sc68app_devel
 * @file      SC68command.h
 * @author    Ben(jamin) Gerard<ben@sashipa.com>
 * @date      1999/05/15
 * @brief     Commands transfert protocol and definition
 * @version   $Id: SC68command.h,v 1.1 2003-03-08 09:54:15 ben Exp $
 */

/*
 *            sc68 - Commands transfert protocol and definition
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

#ifndef _SC68COMMAND_H_
#define _SC68COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SC68_COMMAND_DATA_SIZE 1024

/** Command definition (for application communication)
 */
typedef enum
{
  SC68COM_nop = 0,      /**< No Data */
  SC68COM_comline,      /**< Data is command line : char[0]=argc ... */
  SC68COM_play,		      /**< No Data */
  SC68COM_pause,        /**< No Data */
  SC68COM_stop,         /**< No Data */
  SC68COM_ffwd,         /**< No Data */
  SC68COM_ntrk,         /**< No Data */
  SC68COM_ptrk,         /**< No Data */
  SC68COM_load,         /**< Data is filename */
  SC68COM_eject,        /**< No Data */
  SC68COM_text,         /**< Data is text to display */
  SC68COM_kill,         /**< No Data */
  SC68COM_generateBIDB, /**< No Data */
  SC68COM_displayBIDB,  /**< No Data */
} SC68command_t;

/** Message data structure. */
typedef union
{
  char *load_fname; /**< Filename to load $$$ Why pointers ??? */
  char *text;       /**< Pointer to text $$$ Why pointers ??? */
  struct
  {
    char argc;    /**< Argument count */
    char argv[1]; /**< Argument array */
  } comline;        /**< Command line message */
  char data[SC68_COMMAND_DATA_SIZE];  /**< General data */
} SC68comdata_t;

/** Message structure. */
typedef struct
{
  SC68command_t type; /**< Message type */
  int datasize;       /**< Size of message data in bytes */
  SC68comdata_t data; /**< Message data */
} SC68msg_t;

#ifdef __cplusplus
}
#endif

#endif /* End of file SC68command.h */
