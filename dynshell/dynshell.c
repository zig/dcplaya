/* Dynamic LUA shell */

#include <stdio.h>
#include <kos.h>

#include "lua.h"
#include "lualib.h"

#include "shell.h"

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
  //printf("COMMAND <%s>\n", com);

  if (*fmt) {
    va_list args;
    va_start(args, fmt);
    vsprintf(com, fmt, args);
    va_end(args);

    return lua_dostring(shell_lua_state, com);
  } else
    return 0;
}




////////////////
// LUA commands
////////////////
static int lua_malloc_stats(lua_State * L)
{
  malloc_stats();

  return 0; // 0 return values
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
  {0},
};



static void shell_register_lua_commands()
{
  int i;

  lua_dobuffer(shell_lua_state, shell_basic_lua_init, sizeof(shell_basic_lua_init) - 1, "init");

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
  //lua_iolibopen (shell_lua_state);
  lua_strlibopen (shell_lua_state);
  lua_mathlibopen (shell_lua_state);
  lua_dblibopen (shell_lua_state);

  shell_register_lua_commands();

  old_command_func = shell_set_command_func(dynshell_command);

  // Return pointer on shutdown function
  return shutdown;
}
