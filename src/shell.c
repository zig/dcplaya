#include <kos.h>

#include <ctype.h>

#include "shell.h"
#include "console.h"

#include "lef.h"

static char input[256];
static int input_pos;

#define CONSOLE_Y 50
#define CONSOLE_OUT_Y -600
static int show_console;
static float console_y = CONSOLE_OUT_Y;

lef_prog_t * shell_lef;
shell_shutdown_func_t shell_lef_shutdown_func;

char * shell_lef_fname = "/pc" DREAMMP3_HOME "dynshell/dynshell.lef";

static shell_command_func_t shell_command_func;



static void shell_update_keyboard()
{

  int key;
  
  kbd_poll(maple_first_kb());
  key = kbd_get_key();

/*  if (key != -1)
    printf(" --> %d\n", key);*/
  
  switch (key) {
  case -1:
    return;

  case KBD_KEY_F1<<8: // F1
    shell_load(shell_lef_fname);
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
    show_console = ~show_console;
    break;

  default:
    if (key == ' ' || isprint(key)) {
      csl_putchar(csl_main_console, key);
      input[input_pos++] = key;
    }
    break;
  }

}


void shell_load(const char * fname)
{
  int fd;


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
  fd = fs_open(fname, O_RDONLY);
  if (fd) {
    
    /* $$$ After this call, fd is closed by lef_load !!! */
    shell_lef = lef_load(fd);
    fd = 0;
  }

  if (shell_lef) {
    shell_lef_shutdown_func = (shell_shutdown_func_t) shell_lef->main(0, 0);
  }
}

int shell_init()
{

  shell_load(shell_lef_fname);
  
  return 0;
}

void shell_shutdown()
{
}

void shell_update(float frameTime)
{

  /* Keyboard update */
  shell_update_keyboard();


  /* Console movement handling */
  if (show_console) {
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



int shell_command(const char * com)
{
  //printf("COMMAND <%s>\n", com);

  if (shell_command_func)
    return shell_command_func(com);
  else
    return 0;
}

shell_command_func_t shell_set_command_func(shell_command_func_t func)
{
  shell_command_func_t old = shell_command_func;

  shell_command_func = func;

  return old;
}
