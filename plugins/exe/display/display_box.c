/**
 * @ingroup  exe_plugin
 * @file     display_box.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, box functions
 * 
 * $Id: display_box.c,v 1.2 2002-11-25 16:56:09 ben Exp $
 */

#include "draw/box.h"
#include "display_driver.h"

struct dl_draw_box1_command {
  dl_command_t uc;

  float x1, y1, x2, y2, z;
  float a, r, g, b;
};

struct dl_draw_box2_command {
  dl_command_t uc;

  int type; /* Gradiant type : 0:diagonal 1:horizontal 2:vertical */
  float x1, y1, x2, y2, z;
  float a1, r1, g1, b1;
  float a2, r2, g2, b2;
};

struct dl_draw_box4_command {
  dl_command_t uc;

  float x1, y1, x2, y2, z;
  float a1, r1, g1, b1;
  float a2, r2, g2, b2;
  float a3, r3, g3, b3;
  float a4, r4, g4, b4;
};


static void dl_draw_box1_render_transparent(void * pcom)
{
  struct dl_draw_box1_command * c = pcom;

  draw_box1(dl_trans[0][0] * c->x1 + dl_trans[3][0], 
			dl_trans[1][1] * c->y1 + dl_trans[3][1], 
			dl_trans[0][0] * c->x2 + dl_trans[3][0], 
			dl_trans[1][1] * c->y2 + dl_trans[3][1], 
			dl_trans[2][2] * c->z  + dl_trans[3][2], 
			dl_color[0] * c->a, dl_color[1] * c->r, 
			dl_color[2] * c->g, dl_color[3] * c->b);
}

static void dl_draw_box2_render_transparent(void * pcom)
{
  struct dl_draw_box2_command * c = pcom;  
  void (*f)(float, float, float, float, float, float, float, float, float,
			float, float, float, float) = draw_box2d;

  switch(c->type) {
  case 1:
	f = draw_box2h;
	break;
  case 2:
	f = draw_box2v;
	break;
  }
  f(dl_trans[0][0] * c->x1 + dl_trans[3][0], 
	dl_trans[1][1] * c->y1 + dl_trans[3][1], 
	dl_trans[0][0] * c->x2 + dl_trans[3][0], 
	dl_trans[1][1] * c->y2 + dl_trans[3][1], 
	dl_trans[2][2] * c->z + dl_trans[3][2], 
	dl_color[0] * c->a1, dl_color[1] * c->r1, 
	dl_color[2] * c->g1, dl_color[3] * c->b1, 
	dl_color[0] * c->a2, dl_color[1] * c->r2, 
	dl_color[2] * c->g2, dl_color[3] * c->b2);
}

static void dl_draw_box4_render_transparent(void * pcom)
{
  struct dl_draw_box4_command * c = pcom;

  draw_box4(dl_trans[0][0] * c->x1 + dl_trans[3][0], 
			dl_trans[1][1] * c->y1 + dl_trans[3][1], 
			dl_trans[0][0] * c->x2 + dl_trans[3][0], 
			dl_trans[1][1] * c->y2 + dl_trans[3][1], 
			dl_trans[2][2] * c->z  + dl_trans[3][2], 
			dl_color[0] * c->a1, dl_color[1] * c->r1, 
			dl_color[2] * c->g1, dl_color[3] * c->b1, 
			dl_color[0] * c->a2, dl_color[1] * c->r2, 
			dl_color[2] * c->g2, dl_color[3] * c->b2,
			dl_color[0] * c->a3, dl_color[1] * c->r3, 
			dl_color[2] * c->g3, dl_color[3] * c->b3, 
			dl_color[0] * c->a4, dl_color[1] * c->r4, 
			dl_color[2] * c->g4, dl_color[3] * c->b4);
}


void dl_draw_box1(dl_list_t * dl,
				  float x1, float y1, float x2, float y2, float z,
				  float a, float r, float g, float b)
{
  struct dl_draw_box1_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x1 = x1; c->y1 = y1; c->x2 = x2; c->y2 = y2; c->z = z;
    c->a = a;   c->r = r;   c->g = g;   c->b = b;
    c->a = a;   c->r = r;   c->g = g;   c->b = b;
    dl_insert(dl, c, 0, dl_draw_box1_render_transparent);
  }
}

void dl_draw_box2(dl_list_t * dl, int type,
				  float x1, float y1, float x2, float y2, float z,
				  float a1, float r1, float g1, float b1,
				  float a2, float r2, float g2, float b2)
{
  struct dl_draw_box2_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
	c->type = type;
    c->x1 = x1;    c->y1 = y1;    c->x2 = x2;    c->y2 = y2;    c->z = z;
    c->a1 = a1;    c->r1 = r1;    c->g1 = g1;    c->b1 = b1;
    c->a2 = a2;    c->r2 = r2;    c->g2 = g2;    c->b2 = b2;
    dl_insert(dl, c, 0, dl_draw_box2_render_transparent);
  }
}

void dl_draw_box4(dl_list_t * dl,
				  float x1, float y1, float x2, float y2, float z,
				  float a1, float r1, float g1, float b1,
				  float a2, float r2, float g2, float b2,
				  float a3, float r3, float g3, float b3,
				  float a4, float r4, float g4, float b4)
{
  struct dl_draw_box4_command * c = dl_alloc(dl, sizeof(*c));

  if (c) {
    c->x1 = x1;    c->y1 = y1;    c->x2 = x2;    c->y2 = y2;    c->z = z;
    c->a1 = a1;    c->r1 = r1;    c->g1 = g1;    c->b1 = b1;
    c->a2 = a2;    c->r2 = r2;    c->g2 = g2;    c->b2 = b2;
    c->a3 = a3;    c->r3 = r3;    c->g3 = g3;    c->b3 = b3;
    c->a4 = a4;    c->r4 = r4;    c->g4 = g4;    c->b4 = b4;
    dl_insert(dl, c, 0, dl_draw_box4_render_transparent);
  }
}

DL_FUNCTION_START(draw_box1)
{
  dl_draw_box1(dl, 
			   lua_tonumber(L, 2), /* X1 */
			   lua_tonumber(L, 3), /* Y1 */
			   lua_tonumber(L, 4), /* X2 */
			   lua_tonumber(L, 5), /* Y2 */
			   lua_tonumber(L, 6), /* Z  */
			   lua_tonumber(L, 7), /* A  */
			   lua_tonumber(L, 8), /* R  */
			   lua_tonumber(L, 9), /* G  */
			   lua_tonumber(L, 10) /* B  */
			   );
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(draw_box2)
{
  dl_draw_box2(dl, 
			   lua_tonumber(L, 15), /* TYPE */
			   lua_tonumber(L, 2),  /* X1 */
			   lua_tonumber(L, 3),  /* Y1 */
			   lua_tonumber(L, 4),  /* X2 */
			   lua_tonumber(L, 5),  /* Y2 */
			   lua_tonumber(L, 6),  /* Z  */
			   lua_tonumber(L, 7),  /* A1 */
			   lua_tonumber(L, 8),  /* R1 */
			   lua_tonumber(L, 9),  /* G1 */
			   lua_tonumber(L, 10), /* B1 */
			   lua_tonumber(L, 11), /* A2 */
			   lua_tonumber(L, 12), /* R2 */
			   lua_tonumber(L, 13), /* G2 */
			   lua_tonumber(L, 14)  /* B2 */
			   );
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(draw_box4)
{
  dl_draw_box4(dl, 
			   lua_tonumber(L, 2),  /* X1 */
			   lua_tonumber(L, 3),  /* Y1 */
			   lua_tonumber(L, 4),  /* X2 */
			   lua_tonumber(L, 5),  /* Y2 */
			   lua_tonumber(L, 6),  /* Z  */
			   lua_tonumber(L, 7),  /* A1 */
			   lua_tonumber(L, 8),  /* R1 */
			   lua_tonumber(L, 9),  /* G1 */
			   lua_tonumber(L, 10), /* B1 */
			   lua_tonumber(L, 11), /* A2 */
			   lua_tonumber(L, 12), /* R2 */
			   lua_tonumber(L, 13), /* G2 */
			   lua_tonumber(L, 14), /* B2 */
			   lua_tonumber(L, 15), /* A3 */
			   lua_tonumber(L, 16), /* R3 */
			   lua_tonumber(L, 17), /* G3 */
			   lua_tonumber(L, 18), /* B3 */
			   lua_tonumber(L, 19), /* A4 */
			   lua_tonumber(L, 20), /* R4 */
			   lua_tonumber(L, 21), /* G4 */
			   lua_tonumber(L, 22)  /* B4 */
			   );
  return 0;
}
DL_FUNCTION_END()


