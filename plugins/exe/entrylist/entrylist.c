/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist.c,v 1.5 2002-10-30 20:01:19 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "driver_list.h"
#include "entrylist_driver.h"
#include "entrylist_loader.h"

el_list_t * entrylist_create(void)
{
  el_list_t * el;

  el = allocator_alloc(lists, sizeof(*el));
  if (el) {
	memset(el, 0, sizeof(*el));
	iarray_create(&el->a, 0, 0, lists);
	driver_reference(&entrylist_driver);
  }
  return el;
}

void entrylist_destroy(el_list_t * el)
{
  iarray_destroy(&el->a);
  if (el->path) {
	free((char *)el->path);
	el->path = 0;
  }
  allocator_free(lists, el);
  driver_dereference(&entrylist_driver);
}

void entrylist_lock(el_list_t * el)
{
  iarray_lock(&el->a);
}

void entrylist_unlock(el_list_t * el)
{
  iarray_unlock(&el->a);
}

int entrylist_trylock(el_list_t * el)
{
  return iarray_trylock(&el->a);
}

int entrylist_lockcount(el_list_t * el)
{
  return iarray_lockcount(&el->a);
}

int entrylist_clear(el_list_t * el)
{
  /* Keep path and loading flag */
  return iarray_clear(&el->a);
}

int entrylist_remove(el_list_t * el, int idx)
{
  return iarray_remove(&el->a, idx);
}

int entrylist_set(el_list_t * el, int idx, el_entry_t * e, int eltsize)
{
/*   printf("setting list:%p idx:%d elt:%p size:%d\n", */
/* 		 el,idx,e,eltsize); */
  return iarray_set(&el->a, idx, e, eltsize);
}

int entrylist_insert(el_list_t * el, int idx, el_entry_t * e, int eltsize)
{
  return iarray_insert(&el->a, idx, e, eltsize);
}



EL_FUNCTION_DECLARE(new)
{
  el_list_t * el;

  if (!lists) {
	lua_entrylist_init(L);
	if (!entrylist_tag || !lists) {
      printf("%s : driver not initialized !", __FUNCTION__);
      return 0;
    }
  }
  lua_settop(L, 0);
  el = entrylist_create();
  if (el) {
	printf("%s : entrylist [%p,%d] created.\n", __FUNCTION__,
		   el, allocator_index(lists,el));
	lua_pushusertag(L, el, entrylist_tag);
    return 1;
  }
  return 0;
}


EL_FUNCTION_START(lock)
{
  entrylist_lock(el);
  return 0;
}
EL_FUNCTION_END()

EL_FUNCTION_START(unlock)
{
  entrylist_unlock(el);
  return 0;
}
EL_FUNCTION_END()

EL_FUNCTION_START(gettable)
{
  int type = lua_type(L,2);
  const char * field;

  if (type == LUA_TNUMBER) {
	iarray_elt_t * ea;
	int n = lua_tonumber(L,2) - 1;
	lua_settop(L,0);
	ea = iarray_dup(&el->a, n);
	if (!ea) {
	  printf("%s : index #%d out of range\n", __FUNCTION__, n+1);
	  return 0;
	}
	lua_newtable(L);
	if (ea->addr) {
	  el_entry_t * e = (el_entry_t *)ea->addr;
	  lua_pushstring(L,"type");
	  lua_pushnumber(L,e->type);
	  lua_rawset(L, 1);
	  lua_pushstring(L,"size");
	  lua_pushnumber(L,e->size);
	  lua_rawset(L, 1);
	  lua_pushstring(L,"name");
	  lua_pushstring(L,e->buffer+e->iname);
	  lua_rawset(L, 1);
	  lua_pushstring(L,"file");
	  lua_pushstring(L,e->buffer+e->ifile);
	  lua_rawset(L, 1);
	}
	free(ea);
  } else if (field = lua_tostring(L,2), field) {
	lua_settop(L,0);
	if (!strcmp(field, "n")) {
	  lua_pushnumber(L, el->a.n);
	} else if (!strcmp(field, "path")) {
	  lua_pushstring(L, el->path);
	} else if (!strcmp(field, "loading")) {
	  int loading = el->loading;
	  /* Reading loading stat reset it if loading was finish */
	  if (loading != 1) {
		el->loading = 0;
	  }
	  if (loading) lua_pushnumber(L, loading);
	}

  }
  return lua_gettop(L);
}
EL_FUNCTION_END()

EL_FUNCTION_START(settable)
{
  char tmp[1024];
  int type2 = lua_type(L,2);
  int type3 = lua_type(L,3);
  int idx;
  el_entry_t *e;
  int namelen,filelen;
  unsigned int eltsize;
  int type, size;
  const char * name, * file;

  if (type2 != LUA_TNUMBER) {
	printf("%s : assignment invalid index type <%s>.\n", __FUNCTION__,
		   lua_typename(L,type2));
	return 0;
  }
  idx = (int)lua_tonumber(L,2) - 1;
  printf("Got a number index : %d\n", idx+1);

  if (type3 != LUA_TNIL && type3 != LUA_TTABLE) {
	printf("%s : assignment invalid value type <%s>.\n", __FUNCTION__,
		   lua_typename(L,type3));
  }
  printf("Got a <%s> value\n", lua_typename(L,type3));

  if (type3 == LUA_TNIL) {
	if (!entrylist_remove(el,idx)) {
	  printf("%s : Removing entry #%d\n", __FUNCTION__, idx+1);
	} else {
	  printf("%s : can't remove entry #%d\n", __FUNCTION__, idx+1);
	}
	return 0;
  }

  /* From this point stack element 3 is the source table.. */
  type = 0;
  size = 0;
  name = 0;
  file = 0;

  /* get type */
  lua_pushstring(L,"type");
  lua_gettable(L,3);
  type = lua_tonumber(L,-1);
  printf("type = %d\n", type);

  /* get size */
  lua_pushstring(L,"size");
  lua_gettable(L,3);
  size = lua_tonumber(L,-1);
  printf("size = %d\n", size);
  
  /* get name */
  lua_pushstring(L,"name");
  lua_gettable(L,3);
  name = (char *)lua_tostring(L,-1);
  printf("name = %s\n", name);

  /* get filename */
  lua_pushstring(L,"file");
  lua_gettable(L,3);
  file =  (char *)lua_tostring(L,-1);
  printf("file = %s\n", file);

  /* calculate string lengths */
  namelen = (!name ? 0 : strlen(name)) + 1;
  filelen = (!file ? 0 : strlen(file)) + 1;

  eltsize = sizeof(*e) - sizeof(e->buffer) + namelen + filelen;
  printf("eltsize : %d\n", eltsize);
  if (eltsize > sizeof(tmp)) {
	e = malloc(eltsize);
	if (!e) {
	  printf("%s : temporary element allocation failed !\n", __FUNCTION__);
	  return 0;
	}
	printf("Use temporary malloc \n");
  } else {
	e = (el_entry_t *)tmp;
	printf("Use temporary local\n");
  }

  e->type = type;
  e->size = size;
  e->buffer[e->iname = 0] = 0;
  if (name) {
	printf("copy name [%s], %d\n", name, namelen);
	memcpy(e->buffer, name, namelen);
	printf("-> [%s]\n",  e->buffer+e->iname);
  }
  e->buffer[e->ifile = namelen] = 0;
  if (file) {
	memcpy(e->buffer+namelen, file, filelen);
	printf("-> [%s]\n", e->buffer+e->ifile);
  }

  printf("final : type:%d size:%d name:%s file:%s\n",
		 e->type, e->size, e->buffer+e->iname, e->buffer+e->ifile);

  if (entrylist_set(el, idx, e, eltsize) < 0) {
	printf("%s : set entry failed.\n", __FUNCTION__);
  }
  printf("set done\n");

  if (e != (el_entry_t *)tmp) {
	printf("free temporary \n");
	free(e);
  }
  lua_settop(L,0);
  return 0;
}
EL_FUNCTION_END()

EL_FUNCTION_START(gc)
{
  int n;

  /* $$$ */
  printf("%s : called  (%p) !\n", __FUNCTION__, el);
  if (n=entrylist_lockcount(el), n) {
	printf("%s : Destroying an entry-list (%p) locked %d times!\n",
		   __FUNCTION__, el, n);
  }
  
  entrylist_destroy(el);
  return 0;
}
EL_FUNCTION_END()

EL_FUNCTION_START(clear)
{
  entrylist_clear(el);
  return 0;
}
EL_FUNCTION_END()

EL_FUNCTION_START(load)
{
  const char * path;
  int type;

  type = lua_type(L,2);
  if (type != LUA_TSTRING ) {
	printf("%s : bad argument type <%s>\n", __FUNCTION__,
		   lua_typename(L,type));
	return 0;
  }
  path = lua_tostring(L,2);
  if (!path || !path[0]) {
	printf("%s : invalid path <%s>\n", __FUNCTION__, path);
	return 0;
  }

  if (el_loader_loaddir(el,  path, 0, -1) < 0) {
	printf("%s : failed\n", __FUNCTION__);
	return 0;
  }

  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}
EL_FUNCTION_END()
