/**
 * @ingroup    dcplaya
 * @file       dynshell.c
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Dynamic LUA shell
 *
 * @version    $Id: dynshell.c,v 1.11 2002-09-16 07:06:19 zig Exp $
 */

#include <stdio.h>
#include <kos.h>

#include "filetype.h"

#include "lua.h"
#include "lualib.h"

#include "console.h"
#include "shell.h"

#include "plugin.h"

#include "exceptions.h"



lua_State * shell_lua_state;

typedef void (* shutdown_func_t)();

static shell_command_func_t old_command_func;



enum shell_command_type_t {
  SHELL_COMMAND_LUA,
  SHELL_COMMAND_C,
};

typedef struct shell_command_description {
  char * name;       ///< long name of the command
  char * short_name; ///< short name of the command
  char * usage;      ///< lua command that should print usage of the command
  int  type;         ///< type of command : LUA or C
  lua_CFunction function; ///< function to call (or pointer on lua function in string format)
} shell_command_description_t;


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
      
    result = lua_dostring(shell_lua_state, com);
  } else
    result = 0;

  EXPT_GUARD_CATCH;

/*  printf("CATCHING EXCEPTION IN SHELL !\n");
  irq_dump_regs(0, 0); */

  EXPT_GUARD_END;

  
  return result;
  
}




////////////////
// LUA commands
////////////////
static int lua_malloc_stats(lua_State * L)
{
  malloc_stats();

  // Testing exception handling ...
  * (int *) 1 = 0xdeadbeef;

  return 0; // 0 return values
}


#define MAX_DIR 32

static const char * home = "/pc"  DREAMMP3_HOME;

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

    if (type == FILETYPE_DIR) {
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
      if (type != FILETYPE_LEF) {
	continue;
      }
    }

    strcpy(path_end+1, de->name);
    count++;
    printf("%d %s\n", count, path);
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
  for (i=2; i<=nparam; i++) {
    if (lua_isstring(L, i)) { // this is probably the extension parameter
      strcpy(ext, lua_tostring(L, i)/*, sizeof(ext)*/);
      use_ext = 1;
    } else { // otherwise we assume max_recurse parameter
      max_recurse = lua_tonumber(L, i);
    }
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

  for (i=1; i<= nparam; i++) {
    //strcpy(rpath, home);
    strcpy(rpath, lua_tostring(L, i));
    
    plugin_load_and_register(rpath);
  }

  return 0;
}

static int lua_getchar(lua_State * L)
{
  int k;

  /* ingnore any parameters */
  lua_settop(L, 0);

  k = csl_getchar();

  lua_pushnumber(L, k);

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
"\n     h = shell_help_array[fname]"
"\n   end"
"\n   if h then"
"\n     dostring (h)"
"\n   else"
"\n     print [[commands are:]]"
"\n     local t = { } local n = 1"
"\n     for i, v in shell_help_array do"
"\n       t[n] = getinfo(i).name .. [[, ]]"
"\n       n = n+1"
"\n     end"
"\n     call (print, t)"
"\n--     for i, v in shell_help_array do"
"\n--       print ([[  ]], getinfo(i).name, [[,]])"
"\n--     end"
"\n   end"
"\n end"
"\n "
"\n usage=help"
"\n "
"\n addhelp(help, [[print [[help(command) : show information about a command\n]]]])"
"\n addhelp(addhelp, [[print [[addhelp(command, string_to_execute) : add usage information about a command\n]]]])"
;

static shell_command_description_t commands[] = {
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
    "dir_load(path, [max_recurse], [extension]) : enumerate all .lef file in given path with max_recurse recursion, return an array of file names\n"
    "]])",

    SHELL_COMMAND_C, lua_path_load
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
    "getchar",
    "gc",

    "print([["
    "getchar() : wait for an input key on keyboard and return its value\n"
    "]])",

    SHELL_COMMAND_C, lua_getchar
  },
  { 
    "rawprint",
    "rp",

    "print([["
    "rawprint( ... ) : raw print on console (no extra linefeed like with print)\n"
    "]])",

    SHELL_COMMAND_C, lua_rawprint
  },
  { 
    "consolesize",
    "cs",

    "print([["
    "consolesize( ... ) : return width and height of the console in character\n"
    "]])",

    SHELL_COMMAND_C, lua_consolesize
  },
  {0},
};



static void shell_register_lua_commands()
{
  int i;

  lua_dobuffer(shell_lua_state, shell_basic_lua_init, sizeof(shell_basic_lua_init) - 1, "init");

  dynshell_command("home = [[%s]]", home);

  for (i=0; commands[i].name; i++) {
    lua_register(shell_lua_state, 
		 commands[i].name, commands[i].function);
    if (commands[i].short_name) {
      lua_register(shell_lua_state, 
		   commands[i].short_name, commands[i].function);
    }

    if (commands[i].usage) {
      dynshell_command("addhelp (%s, [[%s]])", commands[i].name, commands[i].usage);
      if (commands[i].short_name)
	dynshell_command("addhelp (%s, [[%s]])", commands[i].short_name, commands[i].usage);
    }
  }

  
  //lua_register(shell_lua_state, "malloc_stats", lua_malloc_stats);
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

  shell_set_command_func(old_command_func);

  lua_close(shell_lua_state);
}

shutdown_func_t lef_main()
{
  printf("shell: Initializing dynamic shell\n");

  //luaB_set_fputs(shell_lua_fputs);

  shell_lua_state = lua_open(0);  
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
