/**
 * @ingroup    dcplaya
 * @file       dynshell.c
 * @author     vincent penne <ziggy@sashipa.com>
 * @date       2002/11/09
 * @brief      Dynamic LUA shell
 *
 * @version    $Id: dynshell.c,v 1.18 2002-09-24 13:47:03 vincentp Exp $
 */

#include <stdio.h>
#include <kos.h>

#include "filetype.h"

#include "lua.h"
#include "lualib.h"

#include "console.h"
#include "shell.h"
#include "luashell.h"

#include "plugin.h"
#include "dcar.h"
#include "playa.h"

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

/*  printf("CATCHING EXCEPTION IN SHELL !\n");
  irq_dump_regs(0, 0); */

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
    driver = driver_list_search(table, name);
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


#define MAX_DIR 32

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

static int copyfile(const char *fn1, const char *fn2)
     /* $$$ Give it a try */
{
  file_t fd, fd2;

  //SDDEBUG("%s [%s] <- [%s]\n", __FUNCTION__, fn1, fn2);
  //SDINDENT;

  fd  = fs_open(fn1, O_WRONLY);
  fd2 = fs_open(fn2, O_RDONLY);

  if (fd && fd2) {
    char buf[256];
    int n;

    do {
      n = fs_read(fd2, buf, 256);
      if (n > 0) {
	fs_write(fd,buf,n);
      }
    } while (n > 0);
  }

  if (fd) fs_close(fd);
  if (fd2) fs_close(fd2);

  //SDUNINDENT;
  return 0;
}

static int createdir(const char *fn)
{
  file_t fd;

  //SDDEBUG("%s [%s]\n", __FUNCTION__, fn);
  //SDINDENT;

  fd  = fs_open(fn, O_WRONLY|O_DIR);
  if (fd) {
    fs_close(fd);
  } else {
    return -1;
  }
  //SDUNINDENT;
  return 0;
}

static int lua_mkdir(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i;

  for (i=1; i<= nparam; i++) {
    if (createdir(lua_tostring(L, i)))
      printf("mkdir : failed creating directory '%s'\n", lua_tostring(L, i));
  }

  return 0;
}

static int lua_unlink(lua_State * L)
{
  int nparam = lua_gettop(L);
  int i;

  for (i=1; i<= nparam; i++) {
    if (fs_unlink(lua_tostring(L, i)))
      printf("unlink : failed deleting '%s'\n", lua_tostring(L, i));
  }

  return 0;
}

static int lua_dcar(lua_State * L)
{
  int nparam = lua_gettop(L);
  int count=-1, com; 
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
    case 'c': case 's': case 'x': case 't':
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
      if (c>='0' && c<='9') {
	opt.in.compress = c - '0';
      } else {
	error = "Invalid command";
	goto error;
      }
    }
  }
  
  switch(com) {
  case 'c':
    if (archive && path) {
      count = dcar_archive(archive, path, &opt);
    }
    break;

  case 't':
    error = "not implemented";
    break;
  case 's':
    if (path=archive, path) {
      count = dcar_simulate(path, &opt);
    }
    break;

  case 'x':
    error = "not implemented";
    break;
  default:
    error = "bad command : try help dcar";
    break;
  }

 error:  
  if (count < 0) {
    printf("dcar : %s\n", error);
  } else {
    printf("dcar := %d\n", count);
    if (opt.out.level)
      printf(" level       : %d\n",opt.out.level);
    if (opt.out.entries)
      printf(" entries     : %d\n",opt.out.entries);
    if (opt.out.bytes)
      printf(" bytes       : %d\n",opt.out.bytes);
    if (opt.out.ubytes && opt.out.cbytes)
      printf(" compression : %d%%\n",opt.out.cbytes*100/opt.out.ubytes);
  }

  return count;
}

static int lua_play(lua_State * L)
{
  int nparam = lua_gettop(L);
  const char * file = 0, *error = 0;
  unsigned int imm = 1;

  /* Get lua parms */
  if (nparam >= 1) {
    file = lua_tostring(L, 1);
  }
  if (nparam >= 2) {
    imm = lua_tonumber(L, 2);
  }
  
  if (!file) {
    error = "missing music file argument";
  } else if (imm<2) {
    error = "boolean expected";
  } else {
    if (playa_loaddisk(file, imm) < 0) {
      error = "invalid music file";
    }
  }

  if (error) {
    printf("play : %s\n", error);
    return -1;
  }
  return 0;
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

  if (imm < 2) {
    error = "boolean expected";
  }
   
  if (!error) {
    playa_loaddisk(0,imm);
  } else {
    printf("stop : %s\n", error);
    return -1;
  }
  return 0;
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
    "play",
    "pl",

    "print([["
    "play(music-file [,immediat]) : play a music file\n"
    "]])",

    SHELL_COMMAND_C, lua_play
  },

  {
    "stop",
    "st",

    "print([["
    "stop([immediat]) : stop current music\n"
    "]])",

    SHELL_COMMAND_C, lua_stop
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
      dynshell_command("addhelp (%s, [[%s]])", commands[i].name, commands[i].usage);
      if (commands[i].short_name)
	dynshell_command("addhelp (%s, [[%s]])", commands[i].short_name, commands[i].usage);
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
