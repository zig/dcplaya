/* $Id: libspc.h,v 1.2 2003-04-21 04:32:48 vincentp Exp $ */

/*
 * SPC library
 *
 * Copyright (C) 2000 AGAWA Koji <kaoru-k@self-core.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _LIBSPC_H_
#define _LIBSPC_H_

typedef struct SPC_Config
{
  int sampling_rate;
  int resolution;
  int channels;
  int is_interpolation;
  int is_echo;
} SPC_Config;

typedef enum
{
  SPC_EMULATOR_UNKNOWN = 0,
  SPC_EMULATOR_ZSNES,
  SPC_EMULATOR_SNES9X
} SPC_EmulatorType;

typedef struct SPC_ID666
{
  int valid;
  char songname[33];
  char gametitle[33];
  char dumper[17];
  char comments[33];
  char author[33];
  int playtime;
  int fadetime;
  SPC_EmulatorType emulator;
} SPC_ID666;

#ifdef __cplusplus
extern "C" {
#endif

int SPC_init(SPC_Config *cfg);
void SPC_close(void);
int SPC_set_state(SPC_Config *cfg);
int SPC_load(const char *fname, SPC_ID666 * id);
void SPC_update(unsigned char *buf);
int SPC_get_id666 (const char *filename, SPC_ID666 * id);
int SPC_write_id666(SPC_ID666 *id, const char *filename);


/*
 * VP : each time a timer is read, the SPC is slowdowned for 
 * SPC_slowdown_instructions instructions.
 * This should leverage host CPU usage without noticeable effect.
 * (the SPC get exactly 2^SPC_slowdown_cycle_shift times slower, 
 *  thus 2^SPC_slowdown_cycle_shift times faster to emulate in slowdown mode)
 * Usually, when the CPU tries to read the timer, it is in a loop  
 * for synchronisation with real world time, thus it should not need to consume
 * too much time to emulate at this moment ...
 *
 */
extern int SPC_debugcolor; /* Set this to one to display profiling border 
			      colors */
extern int SPC_slowdown_cycle_shift;  /* Default value : 5 */
extern int SPC_slowdown_instructions; /* Default value : 8 */



#ifdef __cplusplus
}
#endif

#endif /* _LIBSPC_H_ */
