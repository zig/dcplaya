/* Dynamic LUA shell */

#include <stdio.h>
#include <kos.h>

#include "lua.h"
#include "lualib.h"

#include "shell.h"

lua_State * shell_lua_state;

typedef void (* shutdown_func_t)();

static shell_command_func_t old_command_func;








////////////////
// LUA commands
////////////////
static int lua_malloc_stats(lua_State * L)
{
  malloc_stats();

  return 0; // 0 return values
}


static void shell_register_lua_commands()
{
  lua_register(shell_lua_state, "malloc_stats", lua_malloc_stats);
}






#if 0
void shell_lua_fputs(const char * s)
{
  printf(s);
}
#endif





static int dynshell_command(const char * com)
{
  //printf("COMMAND <%s>\n", com);

  return lua_dostring(shell_lua_state, com);
}




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
