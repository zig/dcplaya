/* 2002/02/20 */

#include <string.h>
#include <stdio.h>

#include "config.h"


#include "draw/text.h"
#include "option.h"
#include "controler.h"
#include "file_wrapper.h"
#include "driver_list.h"
#include "sysdebug.h"

#define OPTION_LATCH_FRAMES_MAX 20
#define OPTION_LATCH_FRAMES_MIN 6

typedef enum {
  OPTION_VOLUME=0,
  OPTION_FILTER,
  OPTION_SHUFFLE,
  OPTION_VISUAL,
  OPTION_LCD_VISUAL,
  //  OPTION_TRACKS,
  //  OPTION_TIME,
  //  OPTION_DELAY,
  //  OPTION_DELAY_STRENGTH,
  //  OPTION_DELAY_TIME,
  //  OPTION_AMIGA_BLEND,
  //  OPTION_JOY_FX,
  OPTION_MAX
} option_e;

#define OPTION_AT_INIT OPTION_VOLUME

static int latch_counter;
static int latch_frames;
static option_e cur_option;
static int track_offset;
static int volume;
static int filter;
static int shuffle;
static vis_driver_t * visual;
static int lcd_visual;

/* static int joyfx_onoff, joyfx_strength, joyfx_time, joyfx_lr; */

char option_str[16];

extern controler_state_t controler68;
extern unsigned int sqrt68(unsigned int);
extern int playa_volume(int);

void lockapp(void);
void unlockapp(void);

int option_setup(void)
{
  latch_frames = OPTION_LATCH_FRAMES_MAX;
  latch_counter = 0;
  option_str[0]=0;
  cur_option = OPTION_AT_INIT;
  volume = playa_volume(-1);
  filter = 1;
  shuffle = 0;

  driver_list_lock(&vis_drivers);

  SDDEBUG("++ VISUALS = %d\n", vis_drivers.n);
  visual = (vis_driver_t *) vis_drivers.drivers;
  driver_reference(&visual->common);
  if (visual) {
    SDDEBUG("++ OPTION VISUAL = %s\n", visual->common.name);
    if (visual->start() < 0) {
      driver_dereference(&visual->common);
      visual = 0;
    }
  } else {
    SDDEBUG("++ NO OPTION VISUAL\n");
  }
  driver_list_unlock(&vis_drivers);

  lcd_visual = OPTION_LCD_VISUAL_FFT;
  track_offset = 0;
  /*   joyfx_onoff = -1; */
  return 0;
}

int option_volume()
{
  return volume&255;
}

int option_filter()
{
  return filter;
}

int option_shuffle()
{
  return shuffle;
}

vis_driver_t * option_visual()
{
  return visual;
}

void option_no_visual()
{
  driver_dereference(&visual->common);
  visual = 0;
}

int option_lcd_visual()
{
  return lcd_visual;
}

static int get_latch_frames()
{
  if (--latch_frames < OPTION_LATCH_FRAMES_MIN) {
    latch_frames = OPTION_LATCH_FRAMES_MIN;
  }
  return latch_frames;
}

/* static void joyfx_in() */
/* { */
/*   if (joyfx_onoff == -1) { */
/*   } */
/* } */

/* static void joyfx_out() */
/* { */
/*   if (joyfx_onoff != -1) { */
/*   } */
/*   joyfx_onoff = -1; */
/* } */

void option_render(unsigned int elapsed_frame)
{
  const float z = 100.0f;
  float x1 = 30.0f, x2 = 680.0f - x1;
  float y1 = 385.0f, y2 = 420.0f;
  int hmove = 0;
  int prev_option;
  
  option_str[0] = 0;
  /* Check controler */
  if (!(controler68.buttons&CONT_Y)) {
    /* Y not pressed */
    if (controler68.buttons_change&CONT_Y) {
      /* Y change state : just going out of options, restore some value */
 
#if 0     
      if (cur_option == OPTION_TRACKS && track_offset) {
        /* Leaving TRACK mode with a different track number 
           causes a change track ! */
        int cur_track = -1;
        track_offset = 0;
      } else if (cur_option == OPTION_JOY_FX) {
        joyfx_out();
      }
#endif  
    }
    track_offset = 0;
    return;
  }
  
  /* Just entering option mode : no previous option */
  prev_option = (controler68.buttons_change&CONT_Y) ? -1 : cur_option; 
  
  latch_counter -= elapsed_frame;
  if (latch_counter < 0) {
    latch_counter = 0;
  }
  
  if (controler68.buttons&CONT_DPAD_LEFT) {
    if (!latch_counter || (controler68.buttons_change&CONT_DPAD_LEFT)) {
      hmove = -1;
      latch_counter = get_latch_frames();
    }  
  } else if (controler68.buttons&CONT_DPAD_RIGHT) {
    if (!latch_counter || (controler68.buttons_change&CONT_DPAD_RIGHT)) {
      hmove = 1;
      latch_counter = get_latch_frames();
    }  
  } else if (controler68.buttons&CONT_DPAD_UP) {
    if (!latch_counter || (controler68.buttons_change&CONT_DPAD_UP)) {
      cur_option = (cur_option + OPTION_MAX - 1) % OPTION_MAX;
      latch_counter = get_latch_frames();
    }  
  } else if (controler68.buttons&CONT_DPAD_DOWN) {
    if (!latch_counter || (controler68.buttons_change&CONT_DPAD_DOWN)) {
      cur_option = (cur_option + 1) % OPTION_MAX;
      latch_counter = get_latch_frames();
    }  
  } else {
    latch_frames = OPTION_LATCH_FRAMES_MAX;
  }
  
  /* Change from OPTION_JOY_FX to another : restore delay settings */
#if 0  
  if (prev_option == OPTION_JOY_FX && cur_option != OPTION_JOY_FX) {
    joyfx_out();
  }
#endif

  text_set_argb(0xFF00FF00);
  switch(cur_option) {
  case OPTION_VOLUME:
    {
      char tmp[32];
      int v;
      float h;

      volume += (hmove << 3);
      if (volume < 0) {
        volume = 0;
      } else if (volume > 255) {
        volume = 255;
      }
      playa_volume(volume);

      strcpy(option_str, "Vol ");
      h = text_draw_str_inside(x1, y1, x2, y2, z, "VOLUME");
      y1 += h;
      y2 += h;
      v = volume * 100;
      sprintf (tmp, "%d.%d%%", v / 255, v % 255 * 9 / 255);
      strcat(option_str,tmp);
      h = text_draw_str_inside(x1, y1, x2, y2, z, tmp);
    } break;
    
  case OPTION_FILTER:
    {
      filter = (filter + hmove) & 1;
      strcpy(option_str, !filter ? "All files" : ".mp3 only"); 
      text_draw_str_inside(x1, y1, x2, y2, z, option_str);
    } break;
    
  case OPTION_SHUFFLE:
    {
      shuffle = (shuffle + hmove) & 1;
      strcpy(option_str,  !shuffle ? "Shuffle OFF" : "Shuffle ON");
      text_draw_str_inside(x1, y1, x2, y2, z, option_str);
    } break;

  case OPTION_VISUAL:
    {
      vis_driver_t * save = visual;
      char tmp [256];
      
      driver_list_lock(&vis_drivers);
      if (hmove > 0) {
	/* Find next visual */
	if (!visual) {
	  visual = (vis_driver_t *)vis_drivers.drivers;
	} else {
	  visual = (vis_driver_t *) visual->common.nxt;
	}
      } else if (hmove < 0) {
	/* Find previous visual */
	any_driver_t *p, *v;

	for (p=0, v=vis_drivers.drivers;
	     v && v != &visual->common;
	     p=v, v=v->nxt)
	  ;
	visual = (vis_driver_t *)p;
      }
      driver_reference(&visual->common);
      driver_list_unlock(&vis_drivers);

      /* Do plugin stop/start op */
      if (visual != save) {
	/* Only if visual changes */
	if (save) {
	  /* Stop previous ... if any */
	  save->stop();
	}
	if (visual) {
	  /* Start new one ... if any */
	  if (visual->start()) {
	    driver_dereference(&visual->common);
	    visual = 0;
	  }
	}
      }
      driver_dereference(&save->common);
	  
      sprintf(tmp,"Visual %s", !visual ? "OFF" : visual->common.name);
      option_str[sizeof(option_str)-1] = 0;
      strcpy(option_str, tmp);
      text_draw_str_inside(x1, y1, x2, y2, z, option_str);
	  
    } break;

  case OPTION_LCD_VISUAL:
    {
      static const char * str[] =
	{ "LCD OFF", "LCD Scope", "LCD FFT", "LCD FFT (x2)" };
      lcd_visual = (lcd_visual + hmove) & 3;
      strcpy(option_str,  str[lcd_visual]);
      text_draw_str_inside(x1, y1, x2, y2, z, option_str);
    } break;
  }
}
