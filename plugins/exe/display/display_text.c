/**
 * @ingroup  exe_plugin
 * @file     display_text.c
 * @author   Vincent Penne
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, text interface
 * 
 * $Id: display_text.c,v 1.8 2003-03-17 18:50:11 ben Exp $
 */

#include <string.h>

#include "dcplaya/config.h"
#include "draw/text.h"
#include "display_driver.h"

#include "sysdebug.h"

struct text_command {
  dl_command_t uc;
  float x, y, z, a, r, g, b;
  char text[1];
};

struct scrolltext_command
{
  dl_command_t uc;
  float x, y, z, a, r, g, b;
  int len;
  float w, h;
  float xs;
  float spd;
  float window;
  int pingpong; /* 0:no ping-pong 1:ping-pong out 2:ping-pong:in */
  char text[1];
};

struct textprop_command {
  dl_command_t uc;
  fontid_t font;
  float size;
  float aspect;
  int filter;
};

static dl_code_e text_render_transparent(void * pcom,
					 dl_context_t * context)
{
  struct text_command * c = pcom;  

  ///$$$
  //  text_set_properties(0,16,1);

  text_set_color(c->a * context->color.a, c->r * context->color.r,
		 c->g * context->color.g, c->b * context->color.b);
  text_draw_str(context->trans[0][0] * c->x + context->trans[3][0],
		context->trans[1][1] * c->y + context->trans[3][1],
		context->trans[2][2] * c->z + context->trans[3][2],
		c->text);
  return DL_COMMAND_OK;
}

static dl_code_e scrolltext_render_transparent(void * pcom,
					       dl_context_t * context)
{
  struct scrolltext_command * c = pcom;
  float x, xs;

  /* Set text color */
  text_set_color(c->a * context->color.a, c->r * context->color.r,
		 c->g * context->color.g, c->b * context->color.b);

  xs = c->xs;
  x = c->x + xs;
  text_draw_str(context->trans[0][0] *    x + context->trans[3][0],
		context->trans[1][1] * c->y + context->trans[3][1],
		context->trans[2][2] * c->z + context->trans[3][2],
		c->text);

  /* Scroll */
  if (c->spd) {
    xs -= c->spd;

    switch (c->pingpong) {

      /* ping-pong when text is outside window */
    case 1: 
      if (c->spd > 0) {
	/* scrolling to the left */
	float xlim = xs + c->w;
	if (xlim < 0) {
	  c->spd = -c->spd; /* Change dir */
	  xs -= xlim;
	}
      } else {
	/* scrolling to the right */
	float xlim = xs - c->window;
	if (xlim > 0) {
	  c->spd = -c->spd; /* Change dir */
	  xs -= xlim;
	}
      }
      break;

      /* ping-pong keep text inside window */
    case 2:
      if (c->spd > 0) {
	/* scrolling to the left */
	float xlim = xs + c->w - c->window;
	if (xlim < 0) {
	  c->spd = -c->spd; /* Change dir */
	  xs -= xlim;
	}
      } else {
	/* scrolling to the right */
	float xlim = xs;
	if (xlim > 0) {
	  c->spd = -c->spd; /* Change dir */
	  xs = 0;
	}
      }
      break;

      /* Loop at start */
    default:
      if (c->spd > 0) {
	/* scrolling to the left */
	float xlim = xs + c->w;
	if (xlim < 0) {
	  xs = c->window + xlim; /* reset outside to the right */
	}
      } else {
	/* scrolling to the right */
	float xlim = xs - c->window;
	if (xlim > 0) {
	  xs = -c->w + xlim; /* reset outside to the left */
	}
      }
      break;
    }

    c->xs = xs;
  }

  return DL_COMMAND_OK;
}



static dl_code_e properties_render_transparent(void * pcom,
					       dl_context_t * context)
{
  struct textprop_command * c = pcom;
  float size = c->size;
  float aspect = c->aspect;

  if (size > 0) {
    size *= context->trans[0][0];
  }
  
  if (aspect > 0 && context->trans[0][0] > 1E-5) {
    aspect *= context->trans[1][1] / context->trans[0][0];
  }

  text_set_properties(c->font, size, aspect, c->filter);
  return DL_COMMAND_OK;
}

static void properties(dl_list_t * dl,
		       fontid_t fontid, const float size, const float aspect,
		       int filter)
{
  struct textprop_command * c;

  if (fontid < 0 || size <= 0) {
    return;
  }
  if (c = dl_alloc(dl, sizeof(*c)), c) {
    c->font = fontid;
    c->size = size;
    c->aspect = aspect;
    c->filter = filter;
    dl_insert(dl, c, 0, properties_render_transparent);
  }
}

static void text(dl_list_t * dl, 
		 float x, float y, float z,
		 float a, float r, float g, float b,
		 const char * text)
{
  struct text_command * c;
  int len;

  if (!text) {
    return;
  }
  if (len = strlen(text), len < 1) {
    return;
  }
  len += sizeof(*c);
  if (c = dl_alloc(dl, len), c) {
    c->x = x;    c->y = y;    c->z = z;
    c->a = a;    c->r = r;    c->g = g;    c->b = b;
    strcpy(c->text, text);
    /* 	SDDEBUG("[dltext: [%s]]\n",c->text); */
    dl_insert(dl, c, 0, text_render_transparent);
  }
}

static void scrolltext(dl_list_t * dl, 
		       float x, float y, float z,
		       float a, float r, float g, float b,
		       const char * text,
		       float w, float spd, int pingpong)
{
  struct scrolltext_command * c;
  int len;

  if (!text) {
    return;
  }
  if (len = strlen(text), len < 1) {
    return;
  }
  len += sizeof(*c);
  if (c = dl_alloc(dl, len), c) {
    c->x = x;    c->y = y;    c->z = z;
    c->a = a;    c->r = r;    c->g = g;    c->b = b;
    c->len = len;
    c->window = w > 0 ? w : 640;
    c->spd = (spd != 0) ? spd : 1;
    c->pingpong = pingpong;
    strcpy(c->text, text);
    text_size_str_prop(text, &c->w, &c->h, 0, 16, 1);
    if (pingpong == 2) {
      c->xs = 0;
    } else {
      c->xs = c->spd > 0 ? c->window : -c->w;
    }
    dl_insert(dl, c, 0, scrolltext_render_transparent);
  }
}

DL_FUNCTION_START(draw_text)
{
  text(dl, 
       lua_tonumber(L, 2), /* x */   
       lua_tonumber(L, 3), /* y */   
       lua_tonumber(L, 4), /* z */   
       lua_tonumber(L, 5), /* a */   
       lua_tonumber(L, 6), /* r */   
       lua_tonumber(L, 7), /* g */   
       lua_tonumber(L, 8), /* b */   
       lua_tostring(L, 9)  /* text */
       );
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(draw_scroll_text)
{
  scrolltext(dl, 
	     lua_tonumber(L, 2),  /* x */   
	     lua_tonumber(L, 3),  /* y */   
	     lua_tonumber(L, 4),  /* z */   
	     lua_tonumber(L, 5),  /* a */   
	     lua_tonumber(L, 6),  /* r */   
	     lua_tonumber(L, 7),  /* g */   
	     lua_tonumber(L, 8),  /* b */   
	     lua_tostring(L, 9),  /* text */
	     lua_tonumber(L, 10), /* window */
	     lua_tonumber(L, 11), /* speed */
	     lua_tonumber(L, 12)  /* ping pong */
       );
  return 0;
}
DL_FUNCTION_END()



DL_FUNCTION_DECLARE(measure_text)
{
  float w,h;
  const char * s;
  fontid_t font = 0;
  float size = 16;
  float aspect = 1;

  /* $$$ Ignore first parameters, since we don't need display list for
     measuring text. */
  s = lua_tostring(L,2);
  if (!s) {
    return 0;
  }
  if (lua_type(L,3) == LUA_TNUMBER) {
    font = lua_tonumber(L,3);
  } 
  if (lua_type(L,4) == LUA_TNUMBER) {
    size = lua_tonumber(L,4);
  } 
  if (lua_type(L,5) == LUA_TNUMBER) {
    aspect = lua_tonumber(L,5);
  } 
  text_size_str_prop(s, &w, &h, font, size, aspect);
  lua_pushnumber(L, w);
  lua_pushnumber(L, h);
  return 2;
}

DL_FUNCTION_START(text_prop)
{
  int fontid = -1;
  float size = -1;
  float aspect = -1;
  int filter = -1;

  if (lua_type(L,2) == LUA_TNUMBER) {
    fontid = (int)lua_tonumber(L, 2);
  }
  if (lua_type(L,3) == LUA_TNUMBER) {
    size = lua_tonumber(L, 3);
  }
  if (lua_type(L,4) == LUA_TNUMBER) {
    aspect = lua_tonumber(L, 4);
  }
  if (lua_type(L,5) == LUA_TNUMBER) {
    filter = lua_tonumber(L, 5);
  }

  properties(dl, fontid, size, aspect, filter);
  return 0;
}
DL_FUNCTION_END()
