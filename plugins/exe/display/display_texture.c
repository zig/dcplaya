/**
 * @ingroup  exe_plugin
 * @file     display_texture.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, texture interface
 * 
 * $Id: display_texture.c,v 1.10 2003-03-18 16:11:10 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "dcplaya/config.h"
#include "display_driver.h"
#include "driver_list.h"
#include "draw/texture.h"
#include "sysdebug.h"

#define GET_TEXID(v,i,verb) \
  if (lua_type(L,i) == LUA_TNUMBER) {\
    int w = lua_tonumber(L,1);\
    v = (w <= 0) ? -1 : texture_exist(w);\
    if (v<0) { \
      if (verb) printf("%s : invalid texture-id [%d]\n", __FUNCTION__, w);\
      return 0;\
    }\
  } else {\
    const char * name = lua_tostring(L,1);\
    v = texture_get(name); \
    if (v<0) { \
      if (verb) printf("%s : invalid texture-name [%s]\n",\
             __FUNCTION__,name?name:"<null>");\
      return 0;\
    }\
  }


/* Create a new texture. */
/* Syntaxes :
 * 1) "filename" [, "type"]
 * 2) "name", w, h, a, r, g, b
 */
DL_FUNCTION_DECLARE(tex_new)
{
  const char * name, * type = 0;
  int w,h;
  int n = lua_gettop(L);
  texid_t texid = -1;

  if (n<1 || (name = lua_tostring(L,1), !name)) {
	printf("%s : bad arguments\n", __FUNCTION__);
	return 0;
  }

  if (n < 3) {
	/* Create from file */
	if (n == 2 && lua_type(L,2) == LUA_TSTRING) {
	  type = lua_tostring(L,2);
	} 
	texid = texture_create_file(name, type);
	if (texid == -1) {
	  printf("%s : unable to create texture from file [%s]\n", __FUNCTION__,
			 name);
	  return 0;
	}
  } else {
	draw_color_t color;
	draw_argb_t argb;

	w = (lua_type(L,2) == LUA_TNUMBER) ? lua_tonumber(L,2) : 0;
	h = (lua_type(L,3) == LUA_TNUMBER) ? lua_tonumber(L,3) : 0;
	color.a = (lua_type(L,4) == LUA_TNUMBER) ? lua_tonumber(L,4) : 1;
	color.r = (lua_type(L,5) == LUA_TNUMBER) ? lua_tonumber(L,5) : 1;
	color.g = (lua_type(L,6) == LUA_TNUMBER) ? lua_tonumber(L,6) : 1;
	color.b = (lua_type(L,7) == LUA_TNUMBER) ? lua_tonumber(L,7) : 1;
	argb = draw_color_float_to_argb(&color);
	texid = texture_create_flat(name, w, h, argb);
	if (texid == -1) {
	  printf("%s : unable to create flat texture [%s] [%dx%dx%08X]\n",
			 __FUNCTION__, name, w, h, argb);
	  return 0;
	}
  }
  
  lua_settop(L,0);
  lua_pushnumber(L,texid);
  return 1;
}

/* Create a new texture. */
DL_FUNCTION_DECLARE(tex_destroy)
{
  texid_t texid;
  int err;
  const char *reason;

  GET_TEXID(texid,1,1);
  err = texture_destroy(texid, lua_tonumber(L,2));
  switch (err) {
  case 0:
	reason = "destroyed";
	break;
  case -1:
	reason = "not found";
	break;
  case -2:
	reason = "locked";
	break;
  case -3:
	reason = "referenced";
	break;
  default:
	reason = "unexpected error";
	break;
  }
  printf("%s : texture-id %d %s.\n", __FUNCTION__, texid, reason);

  lua_settop(L,0);
  if (err = !err, err) {
	lua_pushnumber(L,1);
  }
  return err;
}

/* Get texture from name */
DL_FUNCTION_DECLARE(tex_get)
{
  texid_t texid;

  GET_TEXID(texid,1,1);
  lua_settop(L,0);
  lua_pushnumber(L,texid);
  return 1;
}

/* Like tex_get, exept it does not write error message. */
DL_FUNCTION_DECLARE(tex_exist)
{
  texid_t texid;

  GET_TEXID(texid,1,0);
  lua_settop(L,0);
  lua_pushnumber(L,texid);
  return 1;

}

/* Get info on texture. */
DL_FUNCTION_DECLARE(tex_info)
{
  texture_t * t;
  texid_t texid;

  GET_TEXID(texid,1,1);

  if (t = texture_fastlock(texid), !t) {
	printf("%s : invalid texture\n", __FUNCTION__);
	return 0;
  }
  lua_settop(L,0);
  lua_newtable(L);

  lua_pushstring(L,"id");
  lua_pushnumber(L,texid);
  lua_settable(L,1);

  lua_pushstring(L,"name");
  lua_pushstring(L,t->name);
  lua_settable(L,1);

  lua_pushstring(L,"orig_w");
  lua_pushnumber(L,t->width);
  lua_settable(L,1);

  lua_pushstring(L,"orig_h");
  lua_pushnumber(L,t->height);
  lua_settable(L,1);

  lua_pushstring(L,"w");
  lua_pushnumber(L,1<<t->wlog2);
  lua_settable(L,1);

  lua_pushstring(L,"h");
  lua_pushnumber(L,1<<t->hlog2);
  lua_settable(L,1);

  lua_pushstring(L,"format");
  lua_pushstring(L,texture_formatstr(t->format));
  lua_settable(L,1);

  if (t->twiddled) {
    lua_pushstring(L,"twiddled");
    lua_pushnumber(L,t->twiddled);
    lua_settable(L,1);
  }

  if (t->twiddlable) {
    lua_pushstring(L,"twiddlable");
    lua_pushnumber(L,t->twiddlable);
    lua_settable(L,1);
  }

  lua_pushstring(L,"count");
  lua_pushnumber(L,t->ref);
  lua_settable(L,1);

  texture_release(t);

  return 1;
}
