/**
 * @ingroup  exe_plugin
 * @file     display_texture.c
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, texture interface
 * 
 * $Id: display_texture.c,v 1.2 2002-10-23 02:07:44 benjihan Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include "display_driver.h"
#include "driver_list.h"
#include "texture.h"
#include "sysdebug.h"

#define GET_TEXID(v,i) \
  if (lua_type(L,i) == LUA_TNUMBER) v = lua_tonumber(L,1); \
  else v = texture_get(lua_tostring(L,1)); \
  if (v == -1) { \
	printf("%s : missing texture-name or texture-id\n", __FUNCTION__); \
	return 0; \
  }

/* Create a new texture. */
DL_FUNCTION_DECLARE(tex_new)
{
  const char * name, *type = 0;
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
	  printf("%s : unable to create texture from file %s\n", __FUNCTION__,
			 name);
	  return 0;
	}
  } else {
	w = lua_tonumber(L,2);
	h = lua_tonumber(L,3);
	type = lua_tostring(L,4);
	if (!type) {
	  type = "argb1555";
	}
	printf("%s : creating [%dx%dx%s] not implemented\n", __FUNCTION__,
		   w,h,type);
	return 0;
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

  GET_TEXID(texid,1);
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

  GET_TEXID(texid,1);
  lua_settop(L,0);
  lua_pushnumber(L,texid);
  return 1;
}

/* Get info on texture. */
DL_FUNCTION_DECLARE(tex_info)
{
  texture_t * t;
  texid_t texid;

  GET_TEXID(texid,1);

  if (t = texture_lock(texid), !t) {
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

  lua_pushstring(L,"w");
  lua_pushnumber(L,t->width);
  lua_settable(L,1);

  lua_pushstring(L,"h");
  lua_pushnumber(L,t->height);
  lua_settable(L,1);

  lua_pushstring(L,"format");
  lua_pushstring(L,texture_formatstr(t->format));
  lua_settable(L,1);

  lua_pushstring(L,"count");
  lua_pushnumber(L,t->ref);
  lua_settable(L,1);

  texture_release(t);

  return 1;
}
