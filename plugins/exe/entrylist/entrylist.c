/**
 * @ingroup  exe_plugin
 * @file     entrylist_driver.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist.c,v 1.9 2003-01-11 07:45:00 zigziggy Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "driver_list.h"
#include "entrylist_driver.h"
#include "entrylist_loader.h"
#include "entrylist_path.h"
#include "filetype.h"

#include "sysdebug.h"

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
  int i,n;

  entrylist_lock(el);
  for (i=0, n=el->a.n; i<n; ++i) {
	el_entry_t *e = el->a.elt[i].addr;
	if (e) {
	  elpath_del(e->path);
	}
  }
  elpath_del(el->path);
  entrylist_unlock(el);
  iarray_destroy(&el->a);
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
  int err, n, i;

  entrylist_lock(el);
  for (i=0, n=el->a.n; i<n; ++i) {
	el_entry_t *e = el->a.elt[i].addr;
	if (e) {
	  elpath_del(e->path);
	}
  }
  err = iarray_clear(&el->a);
  entrylist_unlock(el);
  /* Keep path and loading flag */
  return err;
}

int entrylist_remove(el_list_t * el, int idx)
{
  int err = -1;
  iarray_elt_t *ae;

  entrylist_lock(el);
  ae = iarray_addrof(&el->a,idx);
  if (ae) {
	el_entry_t *e = ae->addr;
	if (e) {
	  elpath_del(e->path);
	}
	err = iarray_remove(&el->a, idx);
  }
  entrylist_unlock(el);
  return err;
}

int entrylist_set(el_list_t * el, int idx, el_entry_t * e, int eltsize)
{
  iarray_elt_t *ae;
  entrylist_lock(el);
  ae = iarray_addrof(&el->a, idx);
  if (ae) {
	el_entry_t *e = ae->addr;
	if (e) {
	  elpath_del(e->path);
	}
  }
  idx = iarray_set(&el->a, idx, e, eltsize);
  entrylist_unlock(el);
  return idx;
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
	lua_pushusertagsz(L, el, entrylist_tag, sizeof(*el));
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
	//	iarray_elt_t * ea;
	el_entry_t * e;
	int n = lua_tonumber(L,2) - 1;
	lua_settop(L,0);
	iarray_lock(&el->a);
	e = iarray_addrof(&el->a, n);
	if (!e) {
	  printf("%s : index #%d out of range\n", __FUNCTION__, n+1);
	} else {
	  lua_newtable(L);
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
	  if (e->path) {
		lua_pushstring(L,"path");
		lua_pushstring(L,e->path->path);
		lua_rawset(L, 1);
		/* $$$ */
		lua_pushstring(L,"pathref");
		lua_pushnumber(L,e->path->refcount);
		lua_rawset(L, 1);
	  }
	}
	iarray_unlock(&el->a);
  } else if (field = lua_tostring(L,2), field) {
	lua_settop(L,0);
	if (!strcmp(field, "n")) {
	  lua_pushnumber(L, el->a.n);
	} else if (!strcmp(field, "path")) {
	  if (el->path) {
		  lua_pushstring(L, el->path->path);
	  }
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
/*   int idx2; */
  el_entry_t *e;
  int namelen,filelen;
  unsigned int eltsize;
  int type, size;
  const char * name, * file;
/*   el_list_t * el2; */
/*   iarray_elt_t *ae; */

  if (type2 != LUA_TNUMBER) {
	printf("%s : assignment invalid index type <%s>.\n", __FUNCTION__,
		   lua_typename(L,type2));
	return 0;
  }
  idx = (int)lua_tonumber(L,2) - 1;

  switch (type3) {

  case LUA_TNIL:
	if (!entrylist_remove(el,idx)) {
	  printf("%s : Removing entry #%d\n", __FUNCTION__, idx+1);
	} else {
	  printf("%s : can't remove entry #%d\n", __FUNCTION__, idx+1);
	}
	break;
#if 0
  case LUA_TUSERDATA:
	GET_ENTRYLIST(el2,3);
	if (type3=lua_type(L,4), type3 != LUA_TNUMBER) {
	  printf("%s : parameter #4, invalid index type <%s>.\n", __FUNCTION__,
			 lua_typename(L,type3));
	  break;
	}
	idx2 = (int)lua_tonumber(L,4) - 1;
	entrylist_lock(el2);
	ae = iarray_dup(&el2->a,idx2);
	if (!ae) {
	  entrylist_unlock(el2);
	  printf("%s : get entry (%p,%d) failed.\n", __FUNCTION__, el2, idx2+1);
	  break;
	}
	e = ae->addr;
	elpath_addref(e->path);
	entrylist_unlock(el2);

	printf("set (%p,%d) from (%p,%d)\n", el,idx+1, el2,idx2+1);
	printf("name:%s\n", e->buffer+e->iname);
	printf("file:%s\n", e->buffer+e->ifile);
	printf("path:%s\n", e->path ? e->path->path : 0);
	printf("size:%d\n", e->size);
	printf("type:%d\n", e->type);

	if (entrylist_set(el, idx, e, ae->size) < 0) {
	  printf("%s : invalid index %d\n", __FUNCTION__,idx+1);
	  elpath_del(e->path);
	}
	free(ae);
	break;
#endif
  case LUA_TTABLE:
	/* From this point stack element 3 is the source table.. */
	type = 0;
	size = 0;
	name = 0;
	file = 0;

  /* get type */
	lua_pushstring(L,"type");
	lua_gettable(L,3);
	type = lua_tonumber(L,-1);

	/* get size */
	lua_pushstring(L,"size");
	lua_gettable(L,3);
	size = lua_tonumber(L,-1);
  
	/* get name */
	lua_pushstring(L,"name");
	lua_gettable(L,3);
	name = (char *)lua_tostring(L,-1);

	/* get filename */
	lua_pushstring(L,"file");
	lua_gettable(L,3);
	file =  (char *)lua_tostring(L,-1);

	/* calculate string lengths */
	namelen = (!name ? 0 : strlen(name)) + 1;
	filelen = (!file ? 0 : strlen(file)) + 1;

	eltsize = sizeof(*e) - sizeof(e->buffer) + namelen + filelen;
	if (eltsize > sizeof(tmp)) {
	  e = malloc(eltsize);
	  if (!e) {
		printf("%s : temporary element allocation failed !\n", __FUNCTION__);
		return 0;
	  }
	} else {
	  e = (el_entry_t *)tmp;
	}

	e->type = type;
	e->size = size;
	e->buffer[e->iname = 0] = 0;
	if (name) {
	  memcpy(e->buffer, name, namelen);
	}
	e->buffer[e->ifile = namelen] = 0;
	if (file) {
	  memcpy(e->buffer+namelen, file, filelen);
	}

	if (entrylist_set(el, idx, e, eltsize) < 0) {
	  printf("%s : set entry failed.\n", __FUNCTION__);
	}

	if (e != (el_entry_t *)tmp) {
	  printf("free temporary \n");
	  free(e);
	}
	break;

  default:
	printf("%s : bad type assignement <%s>\n", __FUNCTION__,
		   lua_typename(L,type3));
  }

  lua_settop(L,0);
  return 0;
}
EL_FUNCTION_END()

EL_FUNCTION_START(gc)
{
  int n;

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
  const char * path, * filterstr;
  int type, filter ,i, c;

  static struct {
	char c;            /* control char */
	const char * name; /* corresponding major name */
	int type;          /* result major type number. */
  } table[] = {
	{ 'd', "dir" },
	{ 'f', "file" },
	{ 'x', "plugin" },
	{ 'i', "image" },
	{ 'm', "music" },
	{ 'p', "playlist" },
	{ 'l', "lua" },
	{ 0,0 }
  };

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

  /* Init opcode table. */
  for (i=0; table[i].c; ++i) {
	int type = filetype_major(table[i].name);
	if (type >= 0) {
	  type = FILETYPE_MAJOR_NUM(type);
	  SDDEBUG("table : '%c' [%s] %04x\n", table[i].c, table[i].name, type);
	}
	table[i].type = type;
  }

  filterstr = 0;
  if (lua_type(L,3) == LUA_TSTRING) {
	filterstr = lua_tostring(L,3);
  }
  if (!filterstr) {
	filterstr = "DPM";
  }

  filter = 0;
  while (c = *filterstr++, c) {
	for (i=0; table[i].c; ++i) {
	  if (table[i].type < 0) {
		SDDEBUG("Unknown type [%s]\n", table[i].name);
		continue;
	  }
	  if (c == table[i].c) {
		SDDEBUG("reject type [%s]\n", table[i].name);
		filter &= ~(1<<table[i].type);
		break;
	  } else if ((c ^ 32) == table[i].c) {
		SDDEBUG("accept type [%s]\n", table[i].name);
		filter |= 1<<table[i].type;
		break;
	  }
	}
  }

  SDDEBUG("FINAL FILTER = %x\n", filter);

  if (el_loader_loaddir(el,  path, filter) < 0) {
	printf("%s : failed\n", __FUNCTION__);
	return 0;
  }

  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}
EL_FUNCTION_END()
