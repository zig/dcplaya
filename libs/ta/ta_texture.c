/* KallistiOS 1.1.5

   ta_texture.c
   (c)2000-2001 Dan Potter
   
*/

static char id[] = "KOS $Id: ta_texture.c,v 1.1 2004-07-04 14:16:45 vincentp Exp $";

/* This module handles all texture allocation issues */

#include <stdio.h>
#include <arch/types.h>
#include <dc/video.h>
#include "dc/ta.h"
#include "dc/g2.h"
#include <arch/irq.h>
#include <kos/thread.h>
#include <dc/sq.h>

#define thd_enabled thd_mode

/* Texture allocation base */
static uint32 txr_alloc_base;

/* A very simple texture allocation API. This is not meant to be the final
   version (probably ^_^;;) but it will do for now to help with loading
   multiple textures, at least in one program. */

/* Release all allocated texture space (start at zero) */
void ta_txr_release_all() {
	txr_alloc_base = 0;
}

/* Allocate space for a texture of the given size; the returned value
   must still be run through ta_txr_map() to get the proper address. */
uint32 ta_txr_allocate(uint32 size) {
	uint32 rv;

	rv = txr_alloc_base;
	if (size % 32)
		size = (size & 0xffffffe0) + 0x20;
	txr_alloc_base += size;
	return rv;
}


/* Load texture data into the PVR ram */
void ta_txr_load(uint32 dest, void *src, int size) {
	uint32 *destl;
	uint32 *srcl;

	destl = ta_txr_map(dest);
	srcl = (uint32*)src;
	
	if (size % 4)
		size = (size & 0xfffffffc) + 4;

	sq_cpy(destl, srcl, size);
}

/* Return a pointer to write to the texture ram directly */
/* Danger, DANGER WILL ROBINSON: Compiling this with -O2 makes
   it "optimize out" the addition! Unless we take special steps.. 
   Bug in GCC? */
void *ta_txr_map(uint32 loc) {
	uint32 final;
	final = 0xa4000000 + loc + ta_state.texture_base;
	return (void *)final;
}




