/**
 * @ingroup  dcplaya_draw
 * @file     ta.h
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @date     2002/11/22
 * @brief    draw tile accelerator interface.
 *
 * $Id: ta.h,v 1.2 2002-11-28 04:22:44 ben Exp $
 */

#ifndef _DRAW_TA_H_
#define _DRAW_TA_H_

#include <arch/types.h>
#include <dc/ta.h>

#include "draw/ta_defines.h"

/** @name  Tile accelerator interface.
 */

typedef struct {
  volatile uint32 word1;
  volatile uint32 word2;
  volatile uint32 word3;
  volatile uint32 word4;
  volatile uint32 words[4];
} ta_hw_poly_t;

typedef struct {
  volatile uint32 flags;
  volatile float x;
  volatile float y;
  volatile float z;
  volatile float a;
  volatile float r;
  volatile float g;
  volatile float b;
} ta_hw_col_vtx_t;

typedef struct {
  volatile uint32 flags;
  volatile float x;
  volatile float y;
  volatile float z;
  volatile float u;
  volatile float v;
  volatile uint32 col;
  volatile uint32 addcol;
} ta_hw_tex_vtx_t;

/** Tile accelarator hardware color (no texture) vertex. */
#define HW_COL_VTX ((ta_hw_col_vtx_t *)(0xe0<<24))

/** Tile accelarator hardware textured vertex. */
#define HW_TEX_VTX ((ta_hw_tex_vtx_t *)(0xe0<<24))

/** Tile accelarator hardware polygon. */
#define HW_POLY    ((ta_hw_poly_t    *)(0xe0<<24))

/** Build a TA poly header. */
/* void draw_poly_hdr(ta_hw_poly_t * poly, int flags); */


/** Initialize rendering system. */
int draw_init_render(void);

unsigned int draw_open_render(void);

void draw_translucent_render(void);

void draw_close_render(void);


/** Set current drawing mode. */
void draw_set_flags(int flags);

/** Current drawing flags (mode). */
extern int draw_current_flags;

/** Current frame counter. */
extern unsigned int draw_frame_counter;

#define DRAW_SET_FLAGS(F) if (draw_current_flags != (F)) { draw_set_flags(F); } else

#endif /* #define _DRAW_TA_H_ */
