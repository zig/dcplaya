/**
 * @ingroup  dcplaya_exe_plugin_devel
 * @file     display_box.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, box functions
 * 
 * $Id: display_box.c,v 1.4 2003-03-17 05:05:59 ben Exp $
 */

#include "draw/box.h"
#include "display_driver.h"

struct box1_command {
  dl_command_t uc;

  float x1, y1, x2, y2, z;
  float a, r, g, b;
};

typedef void (*dl_render_box2_t)(float, float, float, float, float,
				 float, float, float, float,
				 float, float, float, float);

struct box2_command {
  dl_command_t uc;

  //  int type; /* Gradiant type : 0:diagonal 1:horizontal 2:vertical */
  dl_render_box2_t f;
  float x1, y1, x2, y2, z;
  float a1, r1, g1, b1;
  float a2, r2, g2, b2;
};

struct box4_command {
  dl_command_t uc;

  float x1, y1, x2, y2, z;
  float a1, r1, g1, b1;
  float a2, r2, g2, b2;
  float a3, r3, g3, b3;
  float a4, r4, g4, b4;
};

static dl_code_e box1_render(const struct box1_command * c,
			     const dl_context_t * context,
			     const float a)
{
  draw_box1(context->trans[0][0] * c->x1 + context->trans[3][0],
	    context->trans[1][1] * c->y1 + context->trans[3][1],
	    context->trans[0][0] * c->x2 + context->trans[3][0],
	    context->trans[1][1] * c->y2 + context->trans[3][1],
	    context->trans[2][2] * c->z  + context->trans[3][2],
	    a, context->color.r * c->r,
	    context->color.g * c->g, context->color.b * c->b);
  return DL_COMMAND_OK;
}

static dl_code_e box1_render_transparent(void * pcom, dl_context_t * context)
{
  struct box1_command * c = pcom;
  const float a = context->color.a * c->a;
  if (a < 1.0) {
    return box1_render(c,context,a);
  }
  return DL_COMMAND_OK;
}

static dl_code_e box1_render_opaque(void * pcom, dl_context_t * context)
{
  struct box1_command * c = pcom;
  if (context->color.a * c->a >= 1.0) {
    return box1_render(c,context,1.0f);
  }
  return DL_COMMAND_OK;
}


static dl_code_e box2_render(const struct box2_command * c,
			     const dl_context_t * context,
			     const float a1, const float a2)
{
  c->f(context->trans[0][0] * c->x1 + context->trans[3][0], 
       context->trans[1][1] * c->y1 + context->trans[3][1], 
       context->trans[0][0] * c->x2 + context->trans[3][0], 
       context->trans[1][1] * c->y2 + context->trans[3][1], 
       context->trans[2][2] * c->z + context->trans[3][2], 
       a1, context->color.r * c->r1, 
       context->color.g * c->g1, context->color.b * c->b1, 
       a2, context->color.r * c->r2, 
       context->color.g * c->g2, context->color.b * c->b2);
  
  return DL_COMMAND_OK;
}


static dl_code_e box2_render_transparent(void * pcom, dl_context_t * context)
{
  struct box2_command * c = pcom;  
  const float a1 = context->color.a * c->a1;
  const float a2 = context->color.a * c->a2;
  if (a1 < 1 || a2 < 1) {
    return box2_render(c,context,a1,a2);
  }
  return DL_COMMAND_OK;
}

static dl_code_e box2_render_opaque(void * pcom, dl_context_t * context)
{
  struct box2_command * c = pcom;  
  const float a1 = context->color.a * c->a1;
  const float a2 = context->color.a * c->a2;
  if (a1 >= 1 && a2 >= 1) {
    return box2_render(c,context,1,1);
  }
  return DL_COMMAND_OK;
}

static dl_code_e box4_render(const struct box4_command * c,
			     const dl_context_t * context,
			     const float a1, const float a2,
			     const float a3, const float a4)
{
  draw_box4(context->trans[0][0] * c->x1 + context->trans[3][0], 
	    context->trans[1][1] * c->y1 + context->trans[3][1], 
	    context->trans[0][0] * c->x2 + context->trans[3][0], 
	    context->trans[1][1] * c->y2 + context->trans[3][1], 
	    context->trans[2][2] * c->z  + context->trans[3][2], 
	    a1, context->color.r * c->r1, 
	    context->color.g * c->g1, context->color.b * c->b1, 
	    a2, context->color.r * c->r2, 
	    context->color.g * c->g2, context->color.b * c->b2,
	    a3, context->color.r * c->r3, 
	    context->color.g * c->g3, context->color.b * c->b3, 
	    a4, context->color.r * c->r4, 
	    context->color.g * c->g4, context->color.b * c->b4);
  return DL_COMMAND_OK;
}


static dl_code_e box4_render_transparent(void * pcom, dl_context_t * context)
{
  struct box4_command * c = pcom;
  const float a1 = context->color.a * c->a1;
  const float a2 = context->color.a * c->a2;
  const float a3 = context->color.a * c->a3;
  const float a4 = context->color.a * c->a4;

  if (a1 < 1 || a2 < 1 || a3 < 1 || a4 < 1) {
    return box4_render(c,context,a1, a2, a3, a4);
  }
  return DL_COMMAND_OK;
}

static dl_code_e box4_render_opaque(void * pcom, dl_context_t * context)
{
  struct box4_command * c = pcom;
  const float a1 = context->color.a * c->a1;
  const float a2 = context->color.a * c->a2;
  const float a3 = context->color.a * c->a3;
  const float a4 = context->color.a * c->a4;

  if (a1 >= 1 && a2 >= 1 && a3 >= 1 && a4 >= 1) {
    return box4_render(c,context, 1, 1, 1, 1);
  }
  return DL_COMMAND_OK;
}


static void box1(dl_list_t * dl,
		 float x1, float y1, float x2, float y2, float z,
		 float a, float r, float g, float b)
{
  if (a > 0) {
    struct box1_command * c = dl_alloc(dl, sizeof(*c));
    if (c) {
      c->x1 = x1; c->y1 = y1; c->x2 = x2; c->y2 = y2; c->z = z;
      c->a = a;   c->r = r;   c->g = g;   c->b = b;
      c->a = a;   c->r = r;   c->g = g;   c->b = b;
      dl_insert(dl, c, box1_render_opaque, box1_render_transparent);
    }
  }
}

static void box2(dl_list_t * dl, int type,
		 float x1, float y1, float x2, float y2, float z,
		 float a1, float r1, float g1, float b1,
		 float a2, float r2, float g2, float b2)
{
  if (a1 > 0 || a2 > 0) {
    struct box2_command * c = dl_alloc(dl, sizeof(*c));

    if (c) {
/*       c->type = type; */
      switch(type) {
      case 1:
	c->f = draw_box2h;
	break;
      case 2:
	c->f = draw_box2v;
	break;
      default:
	c->f = draw_box2d;
      }

      c->x1 = x1;    c->y1 = y1;    c->x2 = x2;    c->y2 = y2;    c->z = z;
      c->a1 = a1;    c->r1 = r1;    c->g1 = g1;    c->b1 = b1;
      c->a2 = a2;    c->r2 = r2;    c->g2 = g2;    c->b2 = b2;
      dl_insert(dl, c, box2_render_opaque, box2_render_transparent);
    }
  }
}

static void box4(dl_list_t * dl,
		 float x1, float y1, float x2, float y2, float z,
		 float a1, float r1, float g1, float b1,
		 float a2, float r2, float g2, float b2,
		 float a3, float r3, float g3, float b3,
		 float a4, float r4, float g4, float b4)
{
  if (a1 > 0 || a2 > 0 || a3 > 0 || a4 > 0) {
    struct box4_command * c = dl_alloc(dl, sizeof(*c));

    if (c) {
      c->x1 = x1;    c->y1 = y1;    c->x2 = x2;    c->y2 = y2;    c->z = z;
      c->a1 = a1;    c->r1 = r1;    c->g1 = g1;    c->b1 = b1;
      c->a2 = a2;    c->r2 = r2;    c->g2 = g2;    c->b2 = b2;
      c->a3 = a3;    c->r3 = r3;    c->g3 = g3;    c->b3 = b3;
      c->a4 = a4;    c->r4 = r4;    c->g4 = g4;    c->b4 = b4;
      dl_insert(dl, c, box4_render_opaque, box4_render_transparent);
    }
  }
}

DL_FUNCTION_START(draw_box1)
{
  box1(dl, 
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
  box2(dl, 
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
  box4(dl, 
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

     
