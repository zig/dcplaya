/**
 * @ingroup  dcplaya_draw
 * @file     ta.c
 * @author   ben(jamin) gerard <ben@sashipa.com>
 * @date     2002/11/22
 * @brief    draw tile accelarator interface
 *
 * $Id: ta.c,v 1.1 2002-11-25 16:42:28 ben Exp $
 */

#include "draw/ta.h"
#include "draw/vertex.h"
#include "draw/texture.h"
#include <dc/ta.h>

/** TA polygon. */
typedef struct {
  uint32 word1;
  uint32 word2;
  uint32 word3;
  uint32 word4;
} ta_poly_t;

/* Harcoded values for TA polygon command. */
static const ta_poly_t poly_table[4] = {
  /* TO   texture     opacity                        */
  /* ----------------------------------------------- */
  /* 00   no         transparent                     */
  { 0x82840012, 0x90800000, 0x949004c0, 0x00000000 },
  /* 01   no         opaque                          */
  { 0x80840012, 0x90800000, 0x20800440, 0x00000000 },
  /* 10   yes        transparent                     */
  { 0x8284000a, 0x92800000, 0x949004c0, 0x00000000 },
  /* 11   yes        opaque                          */
  { 0x8084000a, 0x90800000, 0x20800440, 0x00000000 },
};

/** Current value for TA polygon commands */
static ta_hw_poly_t cur_poly;
int draw_current_flags = DRAW_INVALID_FLAGS;

static const int draw_zwrite  = TA_ZWRITE_ENABLE;
static const int draw_ztest   = TA_DEPTH_GREATER;
static const int draw_culling = TA_CULLING_CCW;

static void make_poly_hdr(ta_hw_poly_t * poly, int flags)
{
  const ta_poly_t * p;
  texture_t * t;
  texid_t texid;
  int idx;
  uint32 word3, word4;

  t = 0;
  texid = DRAW_TEXTURE(flags);
  if (texid != DRAW_NO_TEXTURE) {
	t = texture_lock(texid);
  }

  idx = 0
	| ((t != 0) << 1)
	| ((flags >> DRAW_OPACITY_BIT) & 0x1);

  p = poly_table + idx;
  poly->word1 = p->word1;
  poly->word2 = p->word2;
  word3 = p->word3;
  word4 = 0;
  if (t) {
	/* $$$ does not match documentation !!!  */
	word3 |= ((DRAW_FILTER(flags)^DRAW_FILTER_MASK)) << (13-DRAW_FILTER_BIT);
	word3 |= (t->wlog2-3) << 3;
	word3 |= (t->hlog2-3);
	/* $$$ does not match documentation !! */
	word4 = (t->format<<26) | (t->ta_tex >> 3);

	texture_release(t);
  }
  poly->word3 = word3;
  poly->word4 = word4;
}

/* void draw_poly_hdr(ta_hw_poly_t * poly, int flags) */
/* { */
/*   make_poly_hdr(poly, draw_current_flags = flags); */
/* } */

void draw_set_flags(int flags)
{
  if (flags != draw_current_flags) {
	make_poly_hdr(&cur_poly, draw_current_flags = flags);
	ta_commit32_inline(&cur_poly);
  }
}
