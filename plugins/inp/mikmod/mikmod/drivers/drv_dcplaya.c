/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

$Id: drv_dcplaya.c,v 1.1 2002-09-21 09:53:43 benjihan Exp $

Driver for dcplaya output

==============================================================================*/

/*

Written by benjamin gerard <ben@sashipa.com>

*/

//#include "playa.h"
#include "inp_driver.h"
#include "fifo.h"

#include "mikmod_internals.h"

volatile int dcmikmod_status = 0;

static BOOL DC_IsThere(void)
{
  return 1;
}

static BOOL DC_Init(void)
{
  dcmikmod_status = INP_DECODE_ERROR;
  return VC_Init();
}

static void DC_Exit(void)
{
  dcmikmod_status = INP_DECODE_ERROR;
  VC_Exit();
}

static int dc_update(void)
{
  int n;
  int buffer[512];

  /* Number of bytes in fifo */
  n = fifo_free() << 2;
  if (n < 0) {
    return INP_DECODE_ERROR;
  }

  if (!n) {
    return 0;
  }

  /* Not to much please. */
  if (n > (int)sizeof(buffer)) {
    n = sizeof(buffer);
  }

  n = VC_WriteBytes((void*)buffer, n);
  if (n < 0) {
    return INP_DECODE_ERROR;
  }
  if (!n) {
    /* $$$ ben: no more data ? probably the end ! */
    return INP_DECODE_END;
  }

  /* Get it back to sample */
  n >>= 2;

  if (fifo_write(buffer, n) != n) {
    /* This should not happen since we check the fifo above and no other
       thread fill it. */
    return INP_DECODE_ERROR;
  }

  return INP_DECODE_CONT;
}

static void DC_Update(void)
{
  dcmikmod_status = dc_update();
}

MIKMODAPI MDRIVER drv_dcplaya={
  NULL,
  "dcplaya",
  "dcplaya Driver v1.0",
  0,255,
  "dcplaya",

  NULL,
  DC_IsThere,
  VC_SampleLoad,
  VC_SampleUnload,
  VC_SampleSpace,
  VC_SampleLength,
  DC_Init,
  DC_Exit,
  NULL,
  VC_SetNumVoices,
  VC_PlayStart,
  VC_PlayStop,
  DC_Update,
  NULL,
  VC_VoiceSetVolume,
  VC_VoiceGetVolume,
  VC_VoiceSetFrequency,
  VC_VoiceGetFrequency,
  VC_VoiceSetPanning,
  VC_VoiceGetPanning,
  VC_VoicePlay,
  VC_VoiceStop,
  VC_VoiceStopped,
  VC_VoiceGetPosition,
  VC_VoiceRealVolume
};


/* ex:set ts=4: */
