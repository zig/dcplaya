/**
 * @file      shell.c
 * @author    vincent penne <ziggy@sashipa.com>
 * @date      2002/08/11
 * @brief     shell support for dcplaya
 * @version   $Id: shell.c,v 1.19 2004-06-30 15:17:36 vincentp Exp $
 */

#include <kos.h>
#include <ctype.h>

#include "dcplaya/config.h"
#include "shell.h"
#include "console.h"

#include "lef.h"
#include "sysdebug.h"

#include "priorities.h"

static char input[256];
static int input_pos;

#define CONSOLE_Y 20
#define CONSOLE_OUT_Y -600
static int show_console = 0;
static float console_y = CONSOLE_OUT_Y;

lef_prog_t * shell_lef;
shell_shutdown_func_t shell_lef_shutdown_func;


char * shell_user_lef_fname = "/ram/dcplaya/dynshell.lez";
char * shell_lef_fname = DCPLAYA_HOME "/dynshell/dynshell.lez";

static shell_command_func_t shell_command_func;



#define MAX_COMMANDS 128

static char * commands[MAX_COMMANDS];
static int write_command;
static int read_command;


// HISTORY NOT IMPLEMENTED YET HERE (but implemented in LUA shell so who cares)
/* static char * history[MAX_COMMANDS]; */
/* static int write_history; */
/* static int read_history; */




static void shell_update_keyboard()
{

  int key;

  key = csl_peekchar();

/*  if (key != -1)
    printf(" --> %d\n", key);*/
  
  switch (key) {
  case -1:
    return;

  case KBD_KEY_F1<<8: // F1
    /* first try the user shell */
    shell_load(shell_user_lef_fname);
    /* if fail, load the standard shell */
    if (!shell_lef)
      shell_load(shell_lef_fname);
    break;

  case KBD_KEY_F2<<8: // F2
    csl_putstring(csl_main_console, "dofile(home..[[autorun.lua]])\n");
    shell_command("dofile(home..[[autorun.lua]])");
    break;

  case 8:
    if (input_pos > 0) {
      csl_putchar(csl_main_console, key);
      input_pos--;
    }
    break;

  case 13:
    csl_putchar(csl_main_console, '\n');

    input[input_pos] = 0;
    shell_command(input);

    input_pos = 0;

    break;

  case '`':
    show_console = !show_console;
    break;

  default:
    if (key == ' ' || isprint(key)) {
      csl_putchar(csl_main_console, key);
      input[input_pos++] = key;
    }
    break;
  }

}

static spinlock_t shellmutex;

static void lockshell(void)
{
  spinlock_lock(&shellmutex);
}

void unlockshell(void)
{
  spinlock_unlock(&shellmutex);
}

void shell_load(const char * fname)
{
  /* better to make sure all previous commands are finished */
  shell_wait();

  lockshell();

  /* Shutdown previously opened shell */
  if (shell_lef) {
    if (shell_lef_shutdown_func) {
      shell_lef_shutdown_func();
      shell_lef_shutdown_func = 0;
    }
    lef_free(shell_lef);
    shell_lef = 0;
  }

  /* Load and launch new shell */
  shell_lef = lef_load(fname);

  if (shell_lef) {
    shell_lef_shutdown_func = (shell_shutdown_func_t) shell_lef->main(0, 0);
    SDDEBUG("[shell_load] : [%s] := [OK]\n", fname);
  } else {
    SDERROR("[shell_load] : [%s] := [FAILED]\n", fname);
  }

  unlockshell();
}


static int hack;
static void shell_thread(void * param)
{
  /* VP : set the prio2 to 2 for the shell, so that lua will not get too 
     slow */
  thd_current->prio2 = LUA_THREAD_PRIORITY;

  for (;;) {
    char * com;

    while (write_command == read_command) {
      thd_pass();

      /* Keyboard update */
      shell_update_keyboard();
    }

    lockshell();

    hack = 1;

    com = commands[read_command];

    if (shell_command_func) {
      shell_command_func(com);
    } else {
      printf("shell: don't know what to do with command '%s' (no shell loaded !)\n", com);
    }

    free(commands[read_command]);
    read_command = (read_command+1)%MAX_COMMANDS;

    hack = 0;

    unlockshell();

 }
}



int shell_init()
{
#ifdef DEBUG
  char tmp[256];
#endif
  uint32 old = thd_default_stack_size;
  //thd_default_stack_size = 1024*1024;
  thd_default_stack_size = 256*1024;

  SDDEBUG("[shell_init] : dynshell [%s]\n",shell_lef_fname);

  /* first try the user shell */
  shell_load(shell_user_lef_fname);
  /* if fail, load the standard shell */
  if (!shell_lef)
    shell_load(shell_lef_fname);

  thd_create(shell_thread, 0);
  thd_default_stack_size = old;

  // Setup some environmental variables
  shell_command("setglobal([[__VERSION]],[[" DCPLAYA_VERSION_STR "]])");
#ifdef DEBUG
  sprintf(tmp,"setglobal([[__DEBUG]],%d)",DEBUG_LEVEL);
  shell_command(tmp);
#endif

#ifdef RELEASE
  shell_command("setglobal([[__RELEASE]],1)");
#endif

#ifdef DCPLAYA_URL
  shell_command("setglobal([[__URL]],[[" DCPLAYA_URL "]])");
#endif

  SDDEBUG("[shell_init] := [0]\n");
  
  return 0;
}

void shell_shutdown()
{
#warning "TODO"
}

void shell_update(float frameTime)
{

  /* Console movement handling */
  if (show_console/* || read_command != write_command*/) {
    console_y += 5 * frameTime * (CONSOLE_Y - console_y);
  } else {
    console_y += 5 * frameTime * (CONSOLE_OUT_Y - console_y);
  }

  if (console_y < CONSOLE_OUT_Y + 5)
    csl_disable_render_mode(csl_main_console, CSL_RENDER_WINDOW);
  else
    csl_enable_render_mode(csl_main_console, CSL_RENDER_WINDOW);
      
  csl_main_console->window.y = console_y;
}


static spinlock_t commandmutex;

static void lockcommand(void)
{
  spinlock_lock(&commandmutex);
}

void unlockcommand(void)
{
  spinlock_unlock(&commandmutex);
}


int shell_command(const char * com)
{
  //printf("COMMAND <%s>\n", com);

  if (hack) {
    if (shell_command_func) {
      shell_command_func(com);
    } else {
      printf("shell: don't know what to do with command '%s' (no shell loaded !)\n", com);
    }
    return 0;
  }

  lockcommand();

  if ((write_command+1) % MAX_COMMANDS != read_command) {
    commands[write_command] = strdup(com);
    write_command = (write_command+1) % MAX_COMMANDS;
  } else {
    printf("shell : command buffer full ! could not enqueue command '%s'\n", 
	   com);
  }

  unlockcommand();

  return 0;

}

void shell_wait()
{
  while (write_command != read_command)
    thd_pass();
}

shell_command_func_t shell_set_command_func(shell_command_func_t func)
{
  shell_command_func_t old = shell_command_func;

  shell_command_func = func;

  return old;
}

int shell_toggleconsole()
{
  int old = show_console;
  show_console = !show_console;
  return old;
}

int shell_showconsole()
{
  int old = show_console;
  show_console = 1;
  return old;
}

int shell_hideconsole()
{
  int old = show_console;
  show_console = 0;
  return old;
}

