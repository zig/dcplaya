/**
 * @ingroup    dcplaya
 * @file       dynshell.c
 * @author     vincent penne <ziggy@sashipa.com>
 * @author     benjamin gerard <ben@sashipa.com>
 * @date       2002/11/09
 * @brief      Dynamic LUA shell
 *
 * @version    $Id: dynshell.c,v 1.51 2002-12-18 06:30:30 ben Exp $
 */

#include <stdio.h>
#include <kos.h>

#include "sysdebug.h"

#include "filetype.h"

#include "lua.h"
#include "luadebug.h"
#include "lualib.h"

#include "console.h"
#include "shell.h"
#include "luashell.h"

#include "file_utils.h"
#include "filename.h"
#include "plugin.h"
#include "dcar.h"
#include "gzip.h"
#include "playa.h"
#include "draw/texture.h"
#include "translator/translator.h"
#include "translator/SHAtranslator/SHAtranslatorBlitter.h"

#include "exceptions.h"

#include "ds.h"

lua_State * shell_lua_state;

typedef void (* shutdown_func_t)();

static shell_command_func_t old_command_func;

static int song_tag;

static const char * home = "/pc"  DREAMMP3_HOME;
static const char * initfile = "/pc"  DREAMMP3_HOME "/lua/init.lua";


#define lua_assert(L, test) if (!(test)) lua_assert_func(L, #test); else
static void lua_assert_func(lua_State * L, const char * msg)
{
  lua_error(L, msg);
}

static int dynshell_command(const char * fmt, ...)
{
  char com[1024];
  int result = -1;
  //printf("COMMAND <%s>\n", com);

  EXPT_GUARD_BEGIN;

  if (*fmt) {
    va_list args;
    va_start(args, fmt);
    vsprintf(com, fmt, args);
    va_end(args);
      
    //result = lua_dostring(shell_lua_state, com);
    lua_getglobal(shell_lua_state, "doshellcommand");
    lua_pushstring(shell_lua_state, com);
    lua_call(shell_lua_state, 1, 0);
    result = 0;
  } else
    result = 0;

  EXPT_GUARD_CATCH;

  shell_showconsole(); /* make sure console is visible */

  /*  printf("CATCHING EXCEPTION IN SHELL !\n");
	  irq_dump_regs(0, 0); */

  {
    lua_Debug ar;
    lua_State * L = shell_lua_state;
    int n = 0;
    while (lua_getstack(L, n, &ar)) {
      lua_getinfo (L, "lnS", &ar);
      printf("[%d] "
	     "currentline = %d, "
	     "name = %s, "
	     "namewhat = %s, "
	     "nups = %d, "
	     "linedefined = %d, "
	     "what = %s, "
	     "source = %s, "
	     "short_src = %s\n",
	     n,
	     ar.currentline,
	     ar.name,
	     ar.namewhat,
	     ar.nups,
	     ar.linedefined,
	     ar.what,
	     ar.source,
	     ar.short_src);
      n++;
      break; /* crash if going too far ... */
    }
  }


  EXPT_GUARD_END;

  
  return result;
  
}


/* dynamic structure lua support */
#define lua_push_entry(L, desc, data, entry) lua_push_entry_func((L), &desc##_DSD, (data), (entry) )

static int lua_push_entry_func(lua_State * L, ds_structure_t * desc, void * data, const char * entryname)
{
  ds_entry_t * entry;

  entry = ds_find_entry_func(desc, entryname);

  if (entry) {
    switch (entry->type) {
    case DS_TYPE_INT:
      lua_pushnumber(L, * (int *) ds_data(data, entry));
      return 1;
    case DS_TYPE_STRING:
      lua_pushstring(L, * (char * *) ds_data(data, entry));
      return 1;
    case DS_TYPE_LUACFUNCTION:
      lua_pushcfunction(L, * (lua_CFunction *) ds_data(data, entry));
      return 1;
    }
  }

  return 0;
}



/* driver type */

#include "driver_list.h"


static
#include "luashell_ds.h"

static int shellcd_tag;

static int lua_shellcd_gettable(lua_State * L)
{
  const char * field;
  luashell_command_description_t * cd;

  cd = lua_touserdata(L, 1);
  field = lua_tostring(L, 2);

  if (cd->type == SHELL_COMMAND_LUA && !strcmp(field, "function")) {
    /* cast to lua function */

    // need to read the doc ...
    
    return 0;
  }

  lua_assert(L, field);

  return lua_push_entry(L, luashell_command_description_t, cd, field);
}

static int lua_shellcd_settable(lua_State * L)
{
  const char * field;
  luashell_command_description_t * cd;

  cd = lua_touserdata(L, 1);
  field = lua_tostring(L, 2);

  if (!strcmp(field, "registered")) {
    cd->registered = lua_tonumber(L, 3);
  }

  return 0;
}



static
#include "any_driver_ds.h"

static int driver_tag;
static int driverlist_tag;

static int lua_driverlist_gettable(lua_State * L)
{
  driver_list_t * table = lua_touserdata(L, 1);
  any_driver_t * driver = NULL;
  if (lua_isnumber(L, 2)) {
    // access by number
    int n = lua_tonumber(L, 2);
    lua_assert(L, n>=1 && n<=table->n);
    driver = table->drivers;
    while (--n)
      driver = driver->nxt;
  } else {
    // access by name
    const char * name = lua_tostring(L, 2);

    lua_assert(L, name);
    if (!strcmp(name, "n")) {
      /* asked for number of entry */
      lua_settop(L, 0);
      lua_pushnumber(L, table->n);
      return 1;
    } else {
      driver = driver_list_search(table, name);
    }
  }

  lua_assert(L, driver);
  
  lua_settop(L, 0);
  lua_pushusertag(L, driver, driver_tag);

  return 1;
}

static int lua_driver_gettable(lua_State * L)
{
  const char * field;
  any_driver_t * driver;

  driver = lua_touserdata(L, 1);
  field = lua_tostring(L, 2);

  lua_assert(L, driver);
  lua_assert(L, field);

  if (driver->luacommands && !strcmp(field, "luacommands")) {
    /* special case for luacommands field : 
       return table with all lua commands */
    int i;

    lua_settop(L, 0);
    lua_newtable(L);

    for (i=0; driver->luacommands[i].name; i++) {
      lua_pushnumber(L, i+1);
      lua_pushusertag(L, driver->luacommands+i, shellcd_tag);
      lua_settable(L, 1);
    }
    
    return 1;
  }

  return lua_push_entry(L, any_driver_t, driver, field);
}

static int lua_driver_settable(lua_State * L)
{
  return 0;
}

static void register_driver_type(lua_State * L)
{

  shellcd_tag = lua_newtag(L);
  lua_pushcfunction(L, lua_shellcd_gettable);
  lua_settagmethod(L, shellcd_tag, "gettable");

  lua_pushcfunction(L, lua_shellcd_settable);
  lua_settagmethod(L, shellcd_tag, "settable");


  driverlist_tag = lua_newtag(L);

  lua_pushcfunction(L, lua_driverlist_gettable);
  lua_settagmethod(L, driverlist_tag, "gettable");

  lua_pushusertag(L, &inp_drivers, driverlist_tag);
  lua_setglobal(L, "inp_drivers");

  lua_pushusertag(L, &obj_drivers, driverlist_tag);
  lua_setglobal(L, "obj_drivers");

  lua_pushusertag(L, &vis_drivers, driverlist_tag);
  lua_setglobal(L, "vis_drivers");

  lua_pushusertag(L, &exe_drivers, driverlist_tag);
  lua_setglobal(L, "exe_drivers");


  driver_tag = lua_newtag(L);

  lua_pushcfunction(L, lua_driver_gettable);
  lua_settagmethod(L, driver_tag, "gettable");

  lua_pushcfunction(L, lua_driver_settable);
  lua_settagmethod(L, driver_tag, "settable");
}


////////////////
// LUA commands
////////////////
static int lua_malloc_stats(lua_State * L)
{
  malloc_stats();

  // Testing exception handling ...
  //* (int *) 1 = 0xdeadbeef;

  return 0; // 0 return values
}

/* Return 2 lists
   1st has been filled with directory entries,
   2nd has been filled with file entries.
   Each list has been sorted according to sortdir function.
*/
static int push_dir_as_2_tables(lua_State * L, fu_dirent_t * dir, int count,
								fu_sortdir_f sortdir)
{
  int k,j,i;

  lua_settop(L,0);
  /*   if (!dir) { */
  /*     return 0; */
  /*   } */
  if (sortdir) {
	fu_sort_dir(dir, count, sortdir);
  }

  for (k=0; k<2; ++k) {
    lua_newtable(L);
    for (j=0, i=0; i<count; ++i) {
      if ( !(k ^ (dir[i].size==-1)) ) continue;
      lua_pushnumber(L, ++j);
      lua_pushstring(L, dir[i].name);
      lua_rawset(L, k+1);
    }
	lua_pushstring(L, "n");
	lua_pushnumber(L, j);
	lua_settable(L, k+1);
  }
  return lua_gettop(L);
}

/* Return a list of struct {name, size} sorted according to the sortdir
   function. */
static int push_dir_as_struct(lua_State * L, fu_dirent_t * dir, int count,
							  fu_sortdir_f sortdir)
{
  int i, table;

  lua_settop(L,0);
  /*   if (!dir) { */
  /*     return 0; */
  /*   } */
  if (sortdir) {
	fu_sort_dir(dir, count, sortdir);
  }

  lua_newtable(L);
  table = lua_gettop(L);


  for (i=0; i<count; ++i) {
    int entry;

    lua_pushnumber(L, i+1);
    lua_newtable(L);
    entry=lua_gettop(L);

    lua_pushstring(L, "name");
    lua_pushstring(L, dir[i].name);
    lua_settable(L, entry);
    lua_pushstring(L, "size");
    lua_pushnumber(L, dir[i].size);
    lua_settable(L, entry);
      
    lua_rawset(L, table);
  }
  lua_pushstring(L, "n");
  lua_pushnumber(L, i);
  lua_settable(L, table);
  return lua_gettop(L);
}

/* Directory listing
 * usage : dirlist [switches] path
 *
 *   Get sorted listing of a directory. There is to possible output depending
 *   on the -2 switch.
 *   If -2 is given, the function returns 2 lists, one for directory, the other
 *   for files. This list contains file name only.
 *   If -2 switch is ommitted, the function returns one list which contains 
 *   one structure by file. Each structure as two fields: "name" and "size"
 *   which contains respectively the file or directory name, and the size in
 *   bytes of files or -1 for directories.
 *
 * switches:
 *  -2 : returns 2 separate lists.
 *  -S : sort by descending size
 *  -s : sort by ascending size
 *  -n : sort by name
 */
static int lua_dirlist(lua_State * L)
{
  char rpath[2048];
  const char * path = 0;
  int nparam = lua_gettop(L);
  int count, i;
  fu_dirent_t * dir;
  int two = 0, sort = 0;
  fu_sortdir_f sortdir;

  /* Get parameters */
  for (i=1; i<=nparam; ++i) {
	const char *s = lua_tostring(L, i);
	if (!s) {
	  printf("dirlist : Bad parameters #%d\n", i);
	  return -1;
	}
	if (s[0] == '-') {
	  int j;
	  for (j=1; s[j]; ++j) {
		switch(s[j]) {
		case '2':
		  two = s[j];
		  break;
		case 's': case 'S': case 'n':
		  if (!sort || sort==s[j]) {
			sort = s[j];
		  } else {
			printf("dirlist : '%c' switch incompatible with '%c'.\n",
				   s[j], sort);
			return -1;
		  }
		  break;
		default:
		  printf("dirlist : invalid switch '%c'.\n", s[j]);
		  return -1;
		}
	  }
	} else if (path) {
	  printf("dirlist : Only one path allowed. [%s].\n", s);
	  return -1;
	} else {
	  path = s;
	}
  }
  if (!path) {
	printf("dirlist : Missing <path> parameter.\n");
	return -1;
  }

  if (!fn_get_path(rpath, path, sizeof(rpath), 0)) {
    printf("dirlist : path to long [%s]\n", path);
    return -2;
  }
  if (!rpath[0]) {
	rpath[0] = '/';
	rpath[1] = 0;
  }

  count = fu_read_dir(rpath, &dir, 0);
  if (count < 0) {
    printf("dirlist : %s [%s] \n", fu_strerr(count), rpath);
    return -3;
  }

  switch(sort) {
  case 's':
	/* 	printf("sort by > size\n"); */
	sortdir = fu_sortdir_by_ascending_size;
	break;
  case 'S':
	/* 	printf("sort by < size\n"); */
	sortdir = fu_sortdir_by_descending_size;
	break;
  case 'n':
	/* 	printf("sort by name\n"); */
	sortdir = fu_sortdir_by_name_dirfirst;
	break;
  default:
	sortdir = 0;
  }

  if (two) {
	/* 	printf("Get 2 lists [%d]\n", count); */
	count = push_dir_as_2_tables(L, dir, count, sortdir);
  } else {
	/* 	printf("Get 1 list [%d]\n", count); */
	count = push_dir_as_struct(L, dir, count, sortdir);
  }
  /*   printf("->%d\n", count); */

  if (dir) free(dir);
  return count;
}

#define MAX_DIR 32

extern int filetype_lef;

static int r_path_load(lua_State * L, char *path, unsigned int level, const char * ext, int count)
{
  dirent_t *de;
  int fd = 0;
  char *path_end = 0;
  char dirs[MAX_DIR][32];
  int ndirs;

  //dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%2d,[%s])\n", level, path);

  ndirs = 0;

  if (level == 0) {
    goto error;
  }


  fd = fs_open(path, O_RDONLY | O_DIR);
  if (!fd) {
    count = -1;
    goto error;
  }

  path_end = path + strlen(path);
  path_end[0] = '/';
  path_end[1] = 0;

  while (de = fs_readdir(fd), de) {
    int type;

    type = filetype_get(de->name, de->size);
    if (type == filetype_dir) {
      strcpy(dirs[ndirs++], de->name);
      if (!ext)
		continue;
    }

    if (ext) {
      int l = strlen(de->name);
      if (ext[0] && stricmp(ext, de->name + l - strlen(ext))) {
		continue;
      }
    } else {
      if (type != filetype_lef) {
		continue;
      }
    }

    strcpy(path_end+1, de->name);
    count++;
    //printf("%d %s\n", count, path);
    lua_pushnumber(L, count);
    lua_pushstring(L, path);
    lua_settable(L, 1);
    //lua_pop(L, 1);

  }

 error:
  if (fd) {
    fs_close(fd);
  }

  // go to subdirs if any
  while (ndirs--) {
    int cnt;
    strcpy(path_end+1, dirs[ndirs]);
    cnt = r_path_load(L, path, level-1, ext, count);
    count = (cnt > 0) ? cnt : 0;
  }

  if (path_end) {
    *path_end = 0;
  }
  /*  dbglog(DBG_DEBUG, "<< " __FUNCTION__ "(%2d,[%s]) = %d\n",
	  level, path, count);*/
  return count;
}


static int lua_path_load(lua_State * L)
{
  int max_recurse;
  char rpath[2048];
  char ext[32];
  int use_ext;

  int nparam = lua_gettop(L);

  int i;

  // first parameter is path to search into
  //strcpy(rpath, home);
  if (nparam >= 1)
    strcpy(rpath, lua_tostring(L, 1)/*, sizeof(rpath)*/);
  else {
    strcpy(rpath, home);
    strcat(rpath, "/plugins");
  }

  // default parameters
  max_recurse = 10;
  ext[0] = 0;
  use_ext = 0;

  // get possibly next parameters and guess their meaning
  i = 2;
  if (i<=nparam && lua_isstring(L, i)) {
    strcpy(ext, lua_tostring(L, i)/*, sizeof(ext)*/);
    use_ext = 1;
  } 

  i = 3;
  if (i<=nparam && lua_isnumber(L, i)) {
    max_recurse = lua_tonumber(L, i);
    //printf("i = %d\n", i);
  }

  lua_settop(L, 0);
  lua_newtable(L);

  //max_recurse -= !max_recurse;

  //printf("max_recurse = %d\n", max_recurse);

  r_path_load(L, rpath, max_recurse, use_ext? ext : 0, 0);

  return 1;
}


static int lua_driver_load(lua_State * L)
{
  char rpath[2048];

  int nparam = lua_gettop(L);
  int i;

  for (i=1; i<=nparam; i++) {
    //strcpy(rpath, home);
    strcpy(rpath, lua_tostring(L, i));
    
    plugin_load_and_register(rpath);
  }

  return 0;
}

static int lua_thd_pass(lua_State * L)
{
  thd_pass();

  return 0;
}

static int lua_framecounter(lua_State * L)
{
  int nparam = lua_gettop(L);
  static int offset;
  int ofc;
  int fc;

  for (;;) {
    ofc = ta_state.frame_counter;
    fc = ofc - offset;

    if (fc)
      break;

    thd_pass();
  }

  if (nparam && !lua_isnil(L, 1)) {
    /* reset framecounter */
    offset = ofc;
  }

  lua_settop(L, 0);
  lua_pushnumber(L, fc);

  return 1;
}

/* we use different keycodes than kos :
 * raw key codes are simply translated by 256, instead of behing MULTIPLIED !
 * this save a lot of other key code possibilites for later use ... */
static int convert_key(int k)
{
  if (k >= 256)
    return (k>>8) + 256;
  else
    return k;
}

static int lua_peekchar(lua_State * L)
{
  int k;

  /* ingnore any parameters */
  lua_settop(L, 0);

  k = csl_peekchar();

  if (k >= 0) {
    lua_pushnumber(L, convert_key(k));
    return 1;
  }

  return 0;
}

static int lua_getchar(lua_State * L)
{
  int k;

  /* ingnore any parameters */
  lua_settop(L, 0);

  k = csl_getchar();

  lua_pushnumber(L, convert_key(k));

  return 1;
}


static int lua_rawprint(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i;

  for (i=1; i<= nparam; i++) {
    csl_putstring(csl_main_console, lua_tostring(L, i));
  }

  return 0;
}

static int lua_consolesize(lua_State * L)
{
  lua_pushnumber(L, csl_main_console->w);
  lua_pushnumber(L, csl_main_console->h);

  return 2;
}

static int lua_toggleconsole(lua_State * L)
{
  lua_pushnumber(L, shell_toggleconsole());

  return 1;
}

static int lua_showconsole(lua_State * L)
{
  lua_pushnumber(L, shell_showconsole());

  return 1;
}

static int lua_hideconsole(lua_State * L)
{
  lua_pushnumber(L, shell_hideconsole());

  return 1;
}

static int copyfile(const char *dst, const char *src,
					int force, int unlink, int verbose)
{
  int err;
  char *fct;

  SDDEBUG("[%s] : [%s] [%s] %c%c%c\n", __FUNCTION__,  dst, src,
		  'f' ^ (force<<5), 'u' ^ (unlink<<5), 'v' ^ (verbose<<5));

  if (unlink) {
    fct = "move";
    err = fu_move(dst, src, force);
  } else {
    fct = "copy";
    err = fu_copy(dst, src, force);
  }

  if (err < 0) {
    printf("%s : [%s] [%s]\n", fct, dst, fu_strerr(err));
    return -1;
  }

  if (err >= 0 && verbose) {
    printf("%s : [%s] -> [%s] (%d bytes)\n", unlink ? "move" : "copy",
		   src, dst, err);
  }
  
  return -(err < 0);
}

static int get_option(lua_State * L, const char *fct,
					  int * verbose, int * force)
{
  int i, nparam = lua_gettop(L), err = 0;

  for (i=1; i<nparam; ++i) {
    const char * fname = lua_tostring(L, i);
    if (fname[0] == '-') {
      int j;
      for (j=1; fname[j]; ++j) {
		switch(fname[j]) {
		case 'v':
		  if (verbose) {
			*verbose = 1;
		  } else {
			++err;
		  }
		  break;
		case 'f':
		  if (force) {
			*force = 1;
		  } else {
			++err;
		  }
		  break;
		}
      }
    }
  }
  if (err) {
    printf("%s : [invalid option]\n", fct);
  }
  return err ? -1 : 0;
}

static int lua_mkdir(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i, err;
  int verbose = 0;

  err = get_option(L, "mkdir", &verbose, 0);
  if (err) {
    return -1;
  }
  for (i=1; i<= nparam; i++) {
    int e;
    const char * fname = lua_tostring(L, i);
    if (fname[0] == '-') continue;
    
    e = fu_create_dir(fname);
    if (e < 0) {
      printf("mkdir : [%s] [%s]\n", fname, fu_strerr(e));
      ++err;
    } else if (verbose) {
      printf("mkdir : [%s] created\n", fname);
    }
  }

  lua_settop(L,0);
  if (!err) {
	lua_settop(L,0);
	lua_pushnumber(L,1);
	return 1;
  }
  return 0;
}

static int lua_unlink(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i, err;
  int verbose = 0, force = 0;
  
  err = get_option(L, "unlink", &verbose, &force);
  if (err) {
    return 0;
  }
    
  for (i=1; i <= nparam; i++) {
    int e;
    const char *fname = lua_tostring(L, i);
    if (fname[0] == '-') continue;

    e = fu_remove(fname);
    if (e < 0) {
      if (!force) {
		printf("unlink : [%s] [%s].\n", fname, fu_strerr(e));
		++err;
      }
    } else if (verbose) {
      printf("unlink : [%s] removed.\n", fname);
    }
  }
  if (!err) {
	lua_pushnumber(L,1);
	return 1;
  }
  return 0;
}

static int lua_copy(lua_State * L)
{
  char fulldest[1024] , *enddest;
  const int max = sizeof(fulldest);
  int nparam = lua_gettop(L);
  const char *dst = 0, *src = 0;;
  int i, err, cnt, slashed;
  int verbose = 0, force = 0;

  err = get_option(L, "copy", &verbose, &force);
  if (err) {
    return 0;
  }
  
  /* Count file parameters , get [first] and [last] file in [src] and [dst] */
  for (i=1, cnt=0; i <= nparam; i++) {
    const char *fname = lua_tostring(L, i);
    if (fname[0] == '-') continue;
    if (!src) {
      src = fname;
    } else {
      dst = fname;
    }
    ++cnt;
  }
  
  if (cnt < 2) {
    printf("copy : [missing parameter]\n");
    return 0;
  }
  
  /* Remove trialing '/' */
  enddest = fn_get_path(fulldest, dst, max, &slashed);
  if (!enddest) {
    printf("copy : [missing destination].\n");
    return 0;
  }
  
  if (cnt == 2) {
    if (fu_is_dir(fulldest)) {
      if (!fn_add_path(fulldest, enddest, fn_basename(src), max)) {
		printf("copy : [filename too long].\n");
		return 0;
      }
    } else if (slashed) {
      printf("copy : [%s] [not a directory].\n", dst);
      return 0;
    }
    err = copyfile(fulldest, src, force, 0, verbose);
  } else  {
    /* More than 2 files, destination must be a directory */
    if (!fu_is_dir(fulldest)) {
      printf("copy : [%s] [not a directory].\n", dst);
      return 0;
    }
	
    err = 0;
    for (i=1; i<=nparam; ++i) {
      const char *fname = lua_tostring(L, i);
      if (fname == dst) {
		break;
      }
      if (fname[i] == '-') {
		continue;
      }
      if (!fn_add_path(fulldest, enddest, fn_basename(fname), max)) {
		printf("copy : [filename too long].\n");
		err = -1;
      } else {
		err |= copyfile(fulldest, fname, force, 0, verbose);
      }
    }
  }
  if (!err) {
	lua_settop(L,0);
	lua_pushnumber(L,1);
	return 1;
  }
  return 0;
}

static int lua_dcar(lua_State * L)
{
  int nparam = lua_gettop(L);
  int count=-1, com, value = -1; 
  const char * command=0, *archive=0, *path=0, *error="bad arguments";
  dcar_option_t opt;

  /* Get lua parms */
  if (nparam >= 1) {
    command = lua_tostring(L, 1);
  }
  if (nparam >= 2) {
    archive = lua_tostring(L, 2);
  }
  if (nparam >= 3) {
    path = lua_tostring(L, 3);
  }

  dcar_default_option(&opt);
  opt.in.verbose = 0;
  
  if (!command) {
    command = "?";
  }
  
  for (com = 0; *command; ++command) {
    int c = (*command) & 255;
	
    switch(c) {
    case 'a': case 'c': case 's': case 'x': case 't':
      if (com) {
		error = "Multiple commands";
		goto error;
      }
      com = c;
      break;
    case 'v':
      opt.in.verbose = 1;
      break;
    case 'f':
      break;
    default:
      if (c>='0' && c<='9' && value==-1) {
		value = c - '0';
		while ( (c = command[1] & 255), (c>='0' && c<='9')) {
		  value = value * 10 + (c-'0');
		  ++command;
		}
      } else {
		error = "Invalid command";
		goto error;
      }
    }
  }

  
  switch(com) {
  case 'c': case 'a':
    if (archive && path) {
	  int size;
	  if (value >= 0) opt.in.compress = value % 10u;
	  if (com == 'a' && (size=fu_size(archive), size > 0)) {
		opt.in.skip = size;
	  }
      count = dcar_archive(archive, path, &opt);
      if (count < 0) {
		error = opt.errstr;
      }
    }
    break;
	
  case 't':
    error = "not implemented";
    break;
  case 's':
    if (path=archive, path) {
      count = dcar_simulate(path, &opt);
      if (count < 0) {
		error = opt.errstr;
      }
    }
    break;
	
  case 'x':
    if (path && archive) {
	  if (value > 0) opt.in.skip = value;
      count = dcar_extract(archive, path, &opt);
      if (count < 0) {
		error = opt.errstr;
      }
    }
    break;
  default:
    error = "bad command : try help dcar";
    break;
  }
  
 error:  
  if (count < 0) {
    printf("dcar : %s\n", error);
	return 0;
  } else if (opt.in.verbose) {
    printf("dcar := %d\n", count);
    if (opt.out.level)
      printf(" level        : %d\n",opt.out.level);
    if (opt.out.entries)
      printf(" entries      : %d\n",opt.out.entries);
    if (opt.out.bytes)
      printf(" data bytes   : %d\n",opt.out.bytes);
    if (opt.out.ubytes && opt.out.cbytes) {
      printf(" compressed   : %d\n", opt.out.cbytes);
      printf(" uncompressed : %d\n", opt.out.ubytes);
      printf(" compression  : %d\n",opt.out.cbytes*100/opt.out.ubytes);
    }
  }
  lua_settop(L,0);
  lua_pushnumber(L,count);
  return 1;
}

static int lua_play(lua_State * L)
{
  int nparam = lua_gettop(L);
  const char * file = 0, *error = 0;
  unsigned int imm = 1;
  int track = 0;
  int isplaying;

  /* Get lua parms */
  if (!nparam) {
	goto ok;
  }

  if (nparam >= 1) {
    file = lua_tostring(L, 1);
  }
  if (nparam >= 2) {
    track = lua_tonumber(L, 2);
  }
  if (nparam >= 3) {
    imm = lua_tonumber(L, 3);
  }
  
  if (!file) {
    error = "missing music file argument";
  } else if (imm > 1) {
    error = "boolean expected";
  } else {
    if (playa_start(file, track-1, imm) < 0) {
      error = "invalid music file";
    }
  }

  if (error) {
    printf("play : %s\n", error);
    return 0;
  }

 ok:
  isplaying = playa_isplaying();
  lua_settop(L,0);
  lua_pushnumber(L,isplaying);
  return 1;
}

static int lua_pause(lua_State * L)
{
  int pause;

  if (lua_type(L, 1) != LUA_TNUMBER) {
	pause = playa_ispaused();
  } else {
	pause = playa_pause(lua_tonumber(L, 1) != 0);
  }
  lua_settop(L,0);
  lua_pushnumber(L,pause);
  return 1;
}

static int lua_fade(lua_State * L)
{
  int nparam = lua_gettop(L);
  int ms = 0;

  if (nparam>=1) {
	ms = 1024.0f * lua_tonumber(L, 1);
  }
  ms = playa_fade(ms);
  lua_settop(L,0);
  lua_pushnumber(L, (float)ms * (1.0f/1024.0f));
  return 1;
}

static int lua_stop(lua_State * L)
{
  int nparam = lua_gettop(L);
  const char * error = 0;
  unsigned int imm = 1;

  /* Get lua parms */
  if (nparam >= 1) {
    imm = lua_tonumber(L, 1);
  }

  if (imm > 1) {
    error = "boolean expected";
  }
   
  if (!error) {
    playa_stop(imm);
  } else {
    printf("stop : %s\n", error);
    return 0;
  }
  lua_settop(L,0);
  lua_pushnumber(L,1);
  return 1;
}

static int lua_playtime(lua_State * L)
{
  unsigned int ms = playa_playtime();

  lua_pushnumber(L, ms / 1024.0f);
  lua_pushstring(L, playa_info_make_timestr(0, ms));
  return 2;
}

static int convert_info(lua_State * L, playa_info_t * info, int update)
{
  char * r = (char *)-1;
  int mask;

  if (!info || !info->valid) {
	return 0;
  }

  mask = info->update_mask | -!!update;
  
  
  lua_settop(L,0);
  lua_newtable(L);

  lua_pushstring(L,"valid");
  lua_pushnumber(L, info->valid);
  lua_settable(L,1);

  lua_pushstring(L,"update");
  lua_pushnumber(L, info->update_mask);
  lua_settable(L,1);
  
  if (mask & (1<<PLAYA_INFO_BITS)) {
	lua_pushstring(L,"bits");
	lua_pushnumber(L, 1<<(playa_info_bits(info, -1)+3));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_STEREO)) {
	lua_pushstring(L,"stereo");
	lua_pushnumber(L, playa_info_stereo(info, -1));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_FRQ)) {
	lua_pushstring(L,"frq");
	lua_pushnumber(L, playa_info_frq(info, -1));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TIME)) {
	lua_pushstring(L,"time_ms");
	lua_pushnumber(L, (float)playa_info_time(info, -1) / 1024.0f);
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_BPS)) {
	lua_pushstring(L,"bps");
	lua_pushnumber(L, playa_info_bps(info, -1));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_BYTES)) {
	lua_pushstring(L,"bytes");
	lua_pushnumber(L, playa_info_bytes(info, -1));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_DESC)) {
	lua_pushstring(L,"description");
	lua_pushstring(L,playa_info_desc(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_ARTIST)) {
	lua_pushstring(L,"artist");
	lua_pushstring(L,playa_info_artist(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_ALBUM)) {
	lua_pushstring(L,"album");
	lua_pushstring(L,playa_info_album(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TRACK)) {
	lua_pushstring(L,"track");
	lua_pushstring(L,playa_info_track(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TITLE)) {
	lua_pushstring(L,"title");
	lua_pushstring(L,playa_info_title(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_YEAR)) {
	lua_pushstring(L,"year");
	lua_pushstring(L,playa_info_year(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_GENRE)) {
	lua_pushstring(L,"genre");
	lua_pushstring(L,playa_info_genre(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_COMMENTS)) {
	lua_pushstring(L,"comments");
	lua_pushstring(L,playa_info_comments(info, r));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_FORMAT)) {
	lua_pushstring(L,"format");
	lua_pushstring(L,playa_info_format(info));
	lua_settable(L,1);
  }

  if (mask & (1<<PLAYA_INFO_TIME)) {
	lua_pushstring(L,"time");
	lua_pushstring(L,playa_info_timestr(info));
	lua_settable(L,1);
  }

  return 1;
}

/* [file, [track]] */ 
static int lua_music_info(lua_State * L)
{
  int err;
  playa_info_t * info;

  info = playa_info_lock();
  err = convert_info(L,info, lua_tonumber(L,1));
  playa_info_release(info);

  return err;
}

static int lua_music_info_id(lua_State * L)
{
  playa_info_t * info;
  int id = 0;

  info = playa_info_lock();
  if (info) {
	id = info->valid;
	playa_info_release(info);
  }
  lua_settop(L,0);
  lua_pushnumber(L,id);
  return 1;
}


/* THIS IS REALLY DIRTY AND TEMPORARY :)) */
#include "controler.h"
/* defined in controler.c */
extern int cond_disconnected;
static int lua_cond_connect(lua_State * L)
{
  controler_state_t s;
  int was_connected = !cond_disconnected;
  cond_disconnected = !lua_tonumber(L, 1);

  /* read state to avoid unwanted button */
  controler_read(&s, 1);
  if (was_connected) {
	lua_pushnumber(L,1);
	return 1;
  }
  return 0;
}

/* vmu_tools : 2 commands
 * 1) [ b , vmupath, file ] backup vmu to file ]
 * 2) [ r , vmupath, file ] restore file into vmu ]
 */
static int lua_vmutools(lua_State * L)
{
  char success[32];
  char *buffer = 0;
  char * err = 0;
  int len;
  int nparam = lua_gettop(L);
  const char * command, * vmupath, * file;

  if (nparam != 3) {
    err = "Bad number of argument";
    goto error;
  }
  
  command = lua_tostring(L, 1);
  vmupath = lua_tostring(L, 2);
  file    = lua_tostring(L, 3);

  if (!command || !vmupath || !file) {
    err = "Bad argument";
    goto error;
  }

  if ((command[0] != 'b' && command[0] != 'r') || command[1]) {
    err = "Bad <command> argument";
    goto error;
  }

  if (command[0] == 'r') {
    /* Restore */
    printf("vmu_tools : Restoring [%s] from [%s]...\n", vmupath, file);

    buffer = gzip_load(file, &len);
    if (!buffer) {
      err = "File read error";
      goto error;
    }
    if (len != (128<<10)) {
      err = "Bad file size (not 128Kb)";
      goto error;
    }
    if (fs_vmu_restore(vmupath, buffer)) {
      err = "Restore failure";
      goto error;
    }

  } else {
    printf("vmu_tools : Backup [%s] into [%s]...\n",
		   vmupath, file);

    buffer = malloc(128<<10);
    if (!buffer) {
      err = "128K buffer allocation failure";
      goto error;
    }
    if (fs_vmu_backup(vmupath, buffer)) {
      err = "Backup failure";
      goto error;
    }
    if (len = gzip_save(file, buffer, 128<<10), len < 0) {
      err = "File write error";
      goto error;
    }
  }

  sprintf(success, "compression: %d", (len * 100) >> 17);

 error:
  if(buffer) {
    free(buffer);
  }
  printf("vmu_tools : [%s].\n", err ? err : success);

  return err ? -1 : 0;
}

static int lua_vcolor(lua_State * S)
{
  vid_border_color(lua_tonumber(S, 1), lua_tonumber(S, 1), lua_tonumber(S, 1));

  return 0;
}

static int lua_clear_cd_cache(lua_State * L)
{
  iso_ioctl(0,0,0);  /* clear CD cache ! */
  return 0;
}

static int lua_canonical_path(lua_State * L)
{
  const char * path;
  char buffer[1024], *res;

  if (lua_type(L,1) != LUA_TSTRING) {
	printf("canonical_path : bad argument\n");
	return 0;
  }
  path = lua_tostring(L,1);

  /*   printf("canonical [%s]\n", path); */
  res = fn_canonical(buffer, path, sizeof(buffer));
  if (!res) {
	printf("canonical_path : path [%s] too long\n", path);
	return 0;
  }

  /*   printf("--> [%s]\n", res); */
  lua_settop(L,0);
  lua_pushstring(L,res);
  return 1;
}

static int lua_test(lua_State * L)
{
  int result = -1;
  int n = lua_gettop(L);
  const char * test, * fname;

  if (n != 2 ||
	  lua_type(L,1) != LUA_TSTRING ||
	  lua_type(L,2) != LUA_TSTRING) {
	printf("test : bad arguments\n");
	return 0;
  }

  test = lua_tostring(L,1);
  fname = lua_tostring(L,2);

  if (test[0] == '-' && test[1] && !test[2]) {
	switch(test[1]) {
	case 'e':
	  /* Exist */
	  result = fu_exist(fname);
	  break;
	case 'd':
	  /* Directory */
	  result = fu_is_dir(fname);
	  break;
	case 'f': case 's':
	  result = fu_is_regular(fname);
	  if (result && test[1] == 's') {
		/* Empty */
		result = fu_size(fname) > 0;
	  }
	  break;
	}
  }

  if (result < 0) {
	printf("test : invalid test [%s]\n",test);
	result = 0;
  }

  lua_settop(L,0);
  if (result) {
	lua_pushnumber(L,1);
  }
  return lua_gettop(L);
}

extern void vmu_set_text(const char *s); /* vmu68.c */
extern int vmu_set_visual(int visual);

static int lua_vmu_set_text(lua_State * L)
{
  vmu_set_text(lua_tostring(L,1));
  return 0;
}

static int lua_vmu_set_visual(lua_State * L)
{
  int v = (lua_type(L,1) == LUA_TNIL) ? -1 : lua_tonumber(L,1);
  lua_pushnumber(L, vmu_set_visual(v));
  return 1;
}

static int lua_filetype(lua_State * L)
{
  const char * major_name, * minor_name;
  int type;
  const char * fname;

  fname = lua_tostring(L,1);
  if (!fname) {
	return 0;
  }

  if (fu_is_dir(fname)) {
	type = filetype_directory(fname);
  } else {
	type = filetype_regular(fname);
  }
  if (type < 0) {
	return 0;
  }

  if (filetype_names(type, &major_name, &minor_name) < 0) {
	return 0;
  }

  lua_settop(L,0);
  lua_pushnumber(L,type);
  lua_pushstring(L,major_name);
  lua_pushstring(L,minor_name);
  return lua_gettop(L);
}

static int lua_filetype_add(lua_State * L)
{
  const char * major_name = 0, * minor_name = 0, * ext_list;
  int type;

  type = lua_type(L,1);
  if (type == LUA_TNUMBER) {
	major_name = filetype_major_name(lua_tonumber(L,1));
  } else if (type == LUA_TSTRING) {
	major_name = lua_tostring(L,1);
  }

  if (!major_name || !major_name[0]) {
	printf("filetype_add : bad or missing major type.\n");
	return 0;
  }

  type = filetype_major_add(major_name);
  if (type < 0) {
	printf("filetype_add : error creating major type [%s]\n", major_name);
	return 0;
  }

  minor_name = lua_tostring(L,2);
  ext_list = lua_tostring(L,3);

  if (minor_name || ext_list) {
	type = filetype_add(type, minor_name, ext_list);
	if (type < 0) {
	  printf("filetype_add : error creating minor type\n");
	  return 0;
	}
  }

  lua_settop(L,0);
  lua_pushnumber(L,type);
  return 1;
}

static int greaterlog2(int v)
{
  int i;
  for (i=0; i<(sizeof(int)<<3); ++i) {
	if ((1<<i) >= v) {
	  return i;
	}
  }
  return -1;
}

static int lua_load_background(lua_State * L)
{
  texid_t texid;
  texture_t tmp, * btexture, * stexture = 0;
  SHAwrapperImage_t * img = 0;
  int i;
  int type;
  const char * typestr;
  float dw, dh, orgRatio, finalRatio, u1, v1, w, h;
  int smodulo = 0;

  struct {
	float x,y,u,v;
  } vdef[2];

  texid = texture_get("background");
  if (texid < 0) {
	texid = texture_create_flat("background",1024,512,0xFFFFFFFF);
  }
  if (texid < 0) {
	printf("load_background : no [background] texture.\n");
	return 0;
  }

  btexture = texture_lock(texid);
  if (!btexture) {
	/* Safety net ... */
	return 0;
  }

  /* Little dangerous things ... But avoid to lock rendering */
  texture_release(btexture);

  switch(lua_type(L,1)) {
  case LUA_TNUMBER:
	stexture = texture_lock(lua_tonumber(L,1));
	if (stexture) {
	  texture_release(stexture);
	  smodulo = (1 << stexture->wlog2) - stexture->width;
	}
	break;
  case LUA_TSTRING:
	img = LoadImageFile(lua_tostring(L,1));
	if (img) {
	  tmp.width = img->width;
	  tmp.height = img->height;
	  tmp.wlog2 = greaterlog2(img->width);
	  tmp.hlog2 = greaterlog2(img->height);
	  tmp.addr = img->data;
	  stexture = &tmp;
	  ARGB32toRGB565(tmp.addr, tmp.addr, tmp.width * tmp.height);
	  stexture->format = texture_strtoformat("0565");
	}
	break;
  }

  if (!stexture) {
	if (img) free(img);
	printf("load_background : invalid source image.\n");
	return 0;
  }

  type = 0;
  typestr = lua_tostring(L,2);
  if (typestr) {
	if (!stricmp(typestr,"center")) {
	  type = 1;
	} else if (!stricmp(typestr,"tile")) {
	  type = 2;
	}
  }

  /* Original aspect ratio */
  w = dw = stexture->width;
  h = dh = stexture->height;
  orgRatio = dh / dw;

  if (type == 2) {
	/* Tile needs power of 2 dimension */
	dw = (float)(1 << stexture->wlog2);
	dh = (float)(1 << stexture->hlog2);
  }

  if (dw > 1024) dw = 1024;
  if (dh > 512) dh = 512;
  btexture->width  = dw;
  btexture->height = dh;
  btexture->wlog2  = greaterlog2(btexture->width);
  btexture->hlog2  = greaterlog2(btexture->height);
  btexture->format = stexture->format;
  finalRatio = dh / dw;

/*   printf("type:[%s]\n", !type ? "scale" : (type==1?"center":"tile")); */
/*   printf("src : [%dx%d] [%dx%d] , modulo:%d, ratio:%0.2f\n", */
/* 		 stexture->width,stexture->height, */
/* 		 1<<stexture->wlog2, 1<<stexture->hlog2, */
/* 		 smodulo, orgRatio); */

/*   printf("bkg : [%dx%d] [%dx%d], modulo:%d\n", */
/* 		 btexture->width,btexture->height, */
/* 		 1<<btexture->wlog2,  1<<btexture->hlog2, */
/* 		 (1<<btexture->wlog2) - btexture->width); */

  /* $$$ Currently all texture are 16bit. Since blitz don't care about exact
     pixel format blitz is done with ARGB565 format. */
  Blitz(btexture->addr, btexture->width, btexture->height,
		SHAPF_RGB565, ((1<<btexture->wlog2) - btexture->width) * 2,
		stexture->addr, stexture->width, stexture->height,
		SHAPF_RGB565, smodulo * 2);
  if (img) {
	free(img);
  }

  vdef[0].u = vdef[0].v = 0;

  /* Number of pixel per U/V */
  u1 = 1.0f / (float) (1<<btexture->wlog2);
  v1 = 1.0f / (float) (1<<btexture->hlog2);
  
  if (type != 2) {
	vdef[1].u = (float)btexture->width * u1;
	vdef[1].v = (float)btexture->height * v1;

	if (!type) {
	  w = 1;
	  h = (640.0f / 480.0f) * orgRatio;
	  if (h > w) {
		w /= h;
		h = 1;
	  }
	} else {
	  w = dw / 640.0f;
	  h = dh / 480.0f;
	}

	vdef[0].x = (1 - w) * 0.5;
	vdef[0].y = (1 - h) * 0.5;
	vdef[1].x = 1 - vdef[0].x;
	vdef[1].y = 1 - vdef[0].y;

  } else {
	u1 = 1.0f / w;
	v1 = 1.0f / h;

	vdef[1].u = 640.0f * u1;
	vdef[1].v = 480.0f * v1;
	vdef[0].x = vdef[0].y = 0;
	vdef[1].x = vdef[1].y = 1;
  }

  lua_settop(L, 0);
  lua_newtable(L);
  for (i=0; i<4; ++i) {
    lua_newtable(L);

	lua_pushnumber(L, vdef[i&1].x);      /* X */
	lua_rawseti(L, 2, 1);

	lua_pushnumber(L, vdef[(i&2)>>1].y); /* Y */
	lua_rawseti(L, 2, 2);

	lua_pushnumber(L, vdef[i&1].u);      /* U */
	lua_rawseti(L, 2, 3);

	lua_pushnumber(L, vdef[(i&2)>>1].v); /* V */
	lua_rawseti(L, 2, 4);

	lua_rawseti(L, 1, i+1);
  }
  return 1;
}

#if 0
static char shell_basic_lua_init[] = 
"\n shell_help_array = {}"
"\n "
"\n function addhelp(fname, help_func)"
"\n   shell_help_array[fname] = help_func"
"\n end"
"\n "
"\n function help(fname)"
"\n   local h = nil"
"\n   if fname then"
"\n     if type(fname)==[[string]] then"
"\n 	  fname = getglobal(fname)"
"\n 	end"
"\n     h = shell_help_array[fname]"
"\n   end"
"\n   if h then"
"\n     dostring (h)"
"\n   else"
"\n     print [[commands are:]]"
"\n     local t = { } local n = 1"
"\n     for i, v in shell_help_array do"
"\n       local name=getinfo(i).name"
"\n       if not name then name = [[(?)]] end"
"\n       t[n] = name .. [[, ]]"
"\n       n = n+1"
"\n     end"
"\n     call (print, t)"
"\n--     for i, v in shell_help_array do"
"\n--       print ([[  ]], getinfo(i).name, [[,]])"
"\n--     end"
"\n   end"
"\n end"
"\n "
"\n function doshellcommand(string)"
"\n   dostring(string)"
"\n end"
"\n "
"\n usage=help"
"\n "
"\n addhelp(help, [[print [[help(command) : show information about a command\n]]]])"
"\n addhelp(addhelp, [[print [[addhelp(command, string_to_execute) : add usage information about a command\n]]]])"
;
#endif

static luashell_command_description_t commands[] = {
  { 
    "malloc_stats",
    "ms",

    "print([["
    "mallocs_stats : display memory usage\n"
    "]])",

    SHELL_COMMAND_C, lua_malloc_stats
  },
  { 
    "dir_load",
    "dirl",

    "print([["
    "dir_load(path, [max_recurse], [extension]) : enumerate all files (by default all .lef and .lez files) in given path with max_recurse (default 10) recursion, return an array of file names\n"
    "]])",

    SHELL_COMMAND_C, lua_path_load
  },

  { 
    "dirlist",
    0,
    "print([["
    "dirlist [switches] <path>)\n"
	" switches:\n"
	"  -2 : returns 2 separate lists (see below)\n"
	"  -S : sort by descending size\n"
	"  -s : sort by ascending size\n"
	"  -n : sort by name\n"
	"\n"
	"Get sorted listing of a directory. There is to possible output depending "
	"on the '-2' switch.\n"
	"\n"
	"If -2 is given, the function returns 2 lists, one for directory,"
	"the other for files. This list contains file name only.\n"
	"\n"
	"If -2 switch is ommitted, the function returns one list which contains "
	"one structure by file. Each structure as two fields: \"name\" and "
	"\"size\" which contains respectively the file or directory name, "
	"and the size in bytes of files or -1 for directories.\n"
    "]])",

    SHELL_COMMAND_C, lua_dirlist
  },


  { 
    "driver_load",
    "dl",

    "print([["
    "driver_load(filename) : read a plugin\n"
    "]])",

    SHELL_COMMAND_C, lua_driver_load
  },
  { 
    "peekchar",
    0,

    "print([["
    "peekchar() : return input key on keyboard if available and return its value, or nil if none\n"
    "]])",

    SHELL_COMMAND_C, lua_peekchar
  },
  { 
    "getchar",
    "gc",

    "print([["
    "getchar() : wait for an input key on keyboard and return its value\n"
    "]])",

    SHELL_COMMAND_C, lua_getchar
  },
  {
    "thd_pass",
    0,

    "print([["
    "thd_pass() : sleep for a little while, not consuming CPU time"
    "]])",

    SHELL_COMMAND_C, lua_thd_pass
  },
  {
    "framecounter",
    0,

    "print([["
    "framecounter(clear) : return the screen refresh frame counter, "
    "if clear is set, then clear the framecounter for next time.\n"
    "since there is 60 frame per second, it is important to clear the "
    "framcounter often to avoid numerical imprecision due to use of floating "
    "point number ! (become imprecise after about 24 hours ...)\n"
    "also, this function never returns zero, it will do thd_pass() until "
    "the framecounter is not zero ..."
    "]])",

    SHELL_COMMAND_C, lua_framecounter
  },
  { 
    "rawprint",
    "rp",

    "print([["
    "rawprint( ... ) : "
	"raw print on console (no extra linefeed like with print)\n"
    "]])",

    SHELL_COMMAND_C, lua_rawprint
  },
  { 
    "consolesize",
    "cs",

    "print([["
    "consolesize() : return width and height of the console in character\n"
    "]])",

    SHELL_COMMAND_C, lua_consolesize
  },
  { 
    "toggleconsole",
    "tc",

    "print([["
    "toggleconsole() : toggle console visibility, return old state\n"
    "]])",

    SHELL_COMMAND_C, lua_toggleconsole
  },
  { 
    "showconsole",
    "sc",

    "print([["
    "showconsole() : show console, return old state\n"
    "]])",

    SHELL_COMMAND_C, lua_showconsole
  },
  { 
    "hideconsole",
    "hc",

    "print([["
    "hideconsole() : hide console, return old state\n"
    "]])",

    SHELL_COMMAND_C, lua_hideconsole
  },
  { 
    "mkdir",
    "md",

    "print([["
    "mkdir(dirname) : create new directory\n"
    "]])",

    SHELL_COMMAND_C, lua_mkdir
  },
  { 
    "unlink",
    "rm",

    "print([["
    "unlink(filename) : delete a file (or a directory ?)\n"
    "]])",

    SHELL_COMMAND_C, lua_unlink
  },

  {
    "dcar",
    "ar",

    "print([["
    "dcar(command,archive[,path]) : works on archive\n"
    "  command:\n"
    "    c : create a new archive.\n"
    "    s : simulate a new archive.\n" 
    "    x : extract an existing archive\n"
    "    t : test an existing archive\n"
    "  options:\n"
    "    v : verbose.\n"
    "    f : ignored.\n"
    "]])",

    SHELL_COMMAND_C, lua_dcar
  },

  {
    "playa_play",
    "play",
    "print([["
    "play([music-file [,track, [,immediat] ] ]) : "
	"Play a music file or get play status.\n"
    "]])",

    SHELL_COMMAND_C, lua_play
  },

  {
    "playa_stop",
    "stop",
    "print([["
    "stop([immediat]) : stop current music\n"
    "]])",

    SHELL_COMMAND_C, lua_stop
  },
  {
    "playa_pause",
    "pause",
    "print([["
    "pause([pause_status]) : pause or resume current music or read status\n"
    "]])",
    SHELL_COMMAND_C, lua_pause
  },

  {
    "playa_fade",
    "fade",
    "print([["
    "fade([seconds]) : Music fade-in / fade-out.\n"
	" If seconds = 0 or no seconds is missing read current fade status.\n"
	" If seconds > 0 starts a fade-in.\n"
	" If seconds < 0 starts a fade-out.\n"
    "]])",
    SHELL_COMMAND_C, lua_fade
  },

  {
    "playa_playtime",
    "playtime",
    "print([["
    "seconds,str = playa_playtime() :\n"
	"Returns current playing time in seconds and "
	"into a hh:mm:ss formated string."
    "]])",
    SHELL_COMMAND_C, lua_playtime
  },

  {
    "playa_info",
    "info",
    "print([["
    "playa_info( [update | [filename [ ,track ] ] ]) :\n"
	"Get music information table."
    "]])",
    SHELL_COMMAND_C, lua_music_info
  },

  {
    "playa_info_id",
    "info_id",
    "print([["
    "playa_info_id() :\n"
	"Get current music identifier."
    "]])",
    SHELL_COMMAND_C, lua_music_info_id
  },

  { 
    "cond_connect",
    0,
    "print([["
    "cond_connect(state) : set the connected state of main controller,"
	"return old state.\n"
    "]])",
    SHELL_COMMAND_C, lua_cond_connect
  },
  {
    "vmu_tools",
    "vmu",
    "print([["
    "vmu_tools b vmupath file : Backup VMU to file.\n"
    "vmu_tools r vmupath file : Restore file into VMU.\n"
    "]])",
    SHELL_COMMAND_C, lua_vmutools
  },
  {
    "copy",
    "cp",
    "print([["
    "copy [options] <source-file> <target-file>\n"
    "copy [options] <file1> [<file2> ...] <target-dir>\n"
    "\n"
    "options:\n"
    "\n"
    " -f : force, overwrite existing file\n"
    " -v : verbose\n"
    "]])",
    SHELL_COMMAND_C, lua_copy
  },
  {
    "vcolor",
    0,
    "print([["
    "vcolor(r, g, b, a)\n"
    "]])",
    SHELL_COMMAND_C, lua_vcolor
  },
  {
	"clear_cd_cache",
	0,
    "print([["
    "clear_cd_cache()\n"
    "]])",
    SHELL_COMMAND_C, lua_clear_cd_cache
  },
  { 
	"canonical_path",
	"canonical",
    "print([["
    "canonical(path) : get canonical file path.\n"
    "]])",
    SHELL_COMMAND_C, lua_canonical_path
  },
  { 
	"test",
	0,
    "print([["
    "test(switch,file) : various file test.\n"
	"switch is one of :"
	" -e : file exist\n"
	" -d : file exist and is a directory\n"
	" -f : file exist and is a regular file\n"
	" -s : file is not an empty regular file\n"
    "]])",
    SHELL_COMMAND_C, lua_test
  },

  { 
	"vmu_set_text",
	0,
    "print([["
    "vmu_set_text(text) : set text to display on VMS lcd.\n"
    "]])",
    SHELL_COMMAND_C, lua_vmu_set_text
  },
  { 
	"vmu_set_visual",
	0,
    "print([["
    "vmu_set_visual(<effects-number>) : set/get vmu display effects.\n"
    "]])",
    SHELL_COMMAND_C, lua_vmu_set_visual
  },

  {
	"filetype",
	0,
    "print([["
    "filetype(filename) : get type, major-name, minor-name of given file.\n"
    "]])",
    SHELL_COMMAND_C, lua_filetype
  },

  {
	"filetype_add",
	0,
	"print([["
    "filetype_add(major [, minor ] ) : add and return a filetype.\n"
    "]])",
    SHELL_COMMAND_C, lua_filetype_add
  },

  {
	"load_background",
	0,
	"print([["
    "load_background(filename | texure-id [ , type ]) :"
	" load background image.\n"
	" type := [\"scale\" \"center\" \"tile\", default:\"scale\"\n"
	" Returns a table with 4 vertrices { {x,y,u,v} * 4 }.\n"
    "]])",
    SHELL_COMMAND_C, lua_load_background
  },

  {0},
};

static void shell_register_lua_commands()
{
  int i;
  lua_State * L = shell_lua_state;

  /* Create our user data type */

  register_driver_type(L);

  // song type
  song_tag = lua_newtag(L);


  lua_dostring(L, 
			   "\n function doshellcommand(string)"
			   "\n   dostring(string)"
			   "\n end");

  dynshell_command("home = [[%s]]", home);

  //lua_dobuffer(L, shell_basic_lua_init, sizeof(shell_basic_lua_init) - 1, "init");

  /* register functions */
  for (i=0; commands[i].name; i++) {
    lua_register(L, 
				 commands[i].name, commands[i].function);
    if (commands[i].short_name) {
      lua_register(L, 
				   commands[i].short_name, commands[i].function);
    }
  }

  /* luanch the init script */
  lua_dofile(L, initfile);

  /* register helps */
  for (i=0; commands[i].name; i++) {
    if (commands[i].usage) {
      dynshell_command("addhelp ([[%s]], [[%s]])", 
					   commands[i].name, commands[i].usage);
      if (commands[i].short_name)
		dynshell_command("addhelp ([[%s]], [[%s]])",
						 commands[i].short_name, commands[i].usage);
    }
  }

  
  //lua_register(L, "malloc_stats", lua_malloc_stats);
}







#if 0
void shell_lua_fputs(const char * s)
{
  printf(s);
}
#endif



static void shutdown()
{
  printf("shell: Shutting down dynamic shell\n");

  lua_dostring(shell_lua_state, "dynshell_shutdown()");

  shell_set_command_func(old_command_func);

  lua_close(shell_lua_state);

  cond_disconnected = 0;

}

shutdown_func_t lef_main()
{
  printf("shell: Initializing dynamic shell\n");

  //luaB_set_fputs(shell_lua_fputs);

  shell_lua_state = lua_open(10*1024);  
  if (shell_lua_state == NULL) {
    printf("shell: error initializing LUA state\n");
    return 0;
  }

  lua_baselibopen (shell_lua_state);
  lua_iolibopen (shell_lua_state);
  lua_strlibopen (shell_lua_state);
  lua_mathlibopen (shell_lua_state);
  lua_dblibopen (shell_lua_state);

  shell_register_lua_commands();

  old_command_func = shell_set_command_func(dynshell_command);

  // Return pointer on shutdown function
  return shutdown;
}
