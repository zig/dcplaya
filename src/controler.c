/* 2002/02/13  */

#include <stdio.h>
#include <kos.h>
#include <dc/maple.h>

#include "controler.h"

#define CONTROLER_NO_SMOOTH_FRAMES  50      /* > 2 */
#define CONTROLER_SMOOTH_FACTOR     0x6000  /* [0..65536] */
#define CONTROLER_JOY_DEAD          15
#define CONTROLER_TRIG_DEAD         15

static uint8 mcont;
static cont_cond_t cond;
static cont_cond_t oldcond;
static uint32 last_frame;
//static controler_state_t state;

static const uint32 controler_smooth_mult =
CONTROLER_SMOOTH_FACTOR / (CONTROLER_NO_SMOOTH_FRAMES-2);
//  ((CONTROLER_NO_SMOOTH_FRAMES-1)<<16) / CONTROLER_SMOOTH_FACTOR;

static spinlock_t controler_mutex;

enum { RUNNING, QUIT, ZOMBIE };
static int status;


static void clear_cond(void)
{
  cond.buttons = 0;
  cond.rtrig = cond.ltrig = 0;
  cond.joyx = cond.joyy = 
    cond.joy2x = cond.joy2y = 128;
}

/* try to get controler cond or clear it. */
static void controler_get()
{
  if (!mcont) {
    mcont = maple_first_controller();
    //if (!mcont) {
    //  mcont = maple_second_controller();
    //}
  }  
  if (!mcont || cont_get_cond(mcont, &cond)) {
    clear_cond();
  }
}

static void fill_controler_state(controler_state_t * state, uint32 elapsed)
{
  state->elapsed_frames = elapsed;
  state->buttons = ~cond.buttons;
  state->buttons_change = cond.buttons ^ oldcond.buttons;
  
  state->rtrig = (cond.rtrig < CONTROLER_TRIG_DEAD) ? 0 : cond.rtrig;
  state->ltrig = (cond.ltrig < CONTROLER_TRIG_DEAD) ? 0 : cond.ltrig;
  
  state->joyx = (cond.joyx > 128-CONTROLER_JOY_DEAD &&
		 cond.joyx < 128+CONTROLER_JOY_DEAD) ? 0 : (int)cond.joyx-128;
  state->joyy = (cond.joyy > 128-CONTROLER_JOY_DEAD &&
		 cond.joyy < 128+CONTROLER_JOY_DEAD) ? 0 : (int)cond.joyy-128;

  state->joy2x = (cond.joy2x > 128-CONTROLER_JOY_DEAD &&
		  cond.joy2x < 128+CONTROLER_JOY_DEAD) ? 0 : (int)cond.joy2x-128;
  state->joy2y = (cond.joy2y > 128-CONTROLER_JOY_DEAD &&
		  cond.joy2y < 128+CONTROLER_JOY_DEAD) ? 0 : (int)cond.joy2y-128;
}

static void controler_smooth(uint32 factor)
{
  uint32 oofactor = 65536 - factor;
  cond.rtrig = (cond.rtrig * factor + oldcond.rtrig * oofactor) >> 16;
  cond.ltrig = (cond.ltrig * factor + oldcond.ltrig * oofactor) >> 16;
  cond.joyx  = (cond.joyx  * factor + oldcond.joyx  * oofactor) >> 16;
  cond.joyy  = (cond.joyy  * factor + oldcond.joyy  * oofactor) >> 16;
  cond.joy2x = (cond.joy2x * factor + oldcond.joy2x * oofactor) >> 16;
  cond.joy2y = (cond.joy2y * factor + oldcond.joy2y * oofactor) >> 16;
}


int controler_thread(void * dummy)
{
  while (status != QUIT) {

    uint32 frame = ta_state.frame_counter;
    uint32 elapsed_frame = last_frame;

    elapsed_frame = frame - elapsed_frame;

    if (elapsed_frame) {
      spinlock_lock(&controler_mutex);

      /* pad */
      controler_get();

      if (elapsed_frame < CONTROLER_NO_SMOOTH_FRAMES) {
	int i;
	uint32 factor = 65536 - CONTROLER_SMOOTH_FACTOR;
	
	for (i=1; i<elapsed_frame; ++i) {
	  factor = (factor * factor) >> 16;
	}
	factor = 65536 - factor;
	controler_smooth(factor);
      }


      /* keyboard */
      kbd_poll_repeat(maple_first_kb(), ta_state.frame_counter - last_frame);


      spinlock_unlock(&controler_mutex);

      last_frame = frame;
    }

    thd_pass();
  }

  status = ZOMBIE;
}


int controler_init(uint32 frame)
{
  int err = 0;
  
  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "\n" );
  spinlock_init(&controler_mutex);
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : GetControler, frame=%u\n", frame);

  controler_get();
  oldcond = cond;

  status = RUNNING;
  thd_create(controler_thread, 0);
  
  dbglog(DBG_DEBUG, "<< " __FUNCTION__ " : return code [%d]\n", err);

  return err;
}

void controler_shutdown()
{
  status = QUIT;
  while (status != ZOMBIE)
    thd_pass();
}

int controler_read(controler_state_t * state, uint32 frame)
{
  uint32 elapsed_frame = last_frame;
  
  elapsed_frame = frame - elapsed_frame;

  spinlock_lock(&controler_mutex);

  fill_controler_state(state, elapsed_frame);
  oldcond = cond;

  spinlock_unlock(&controler_mutex);
  return 0;
}

int controler_pressed(const controler_state_t * state, uint32 mask)
{
  return state->buttons & state->buttons_change & mask;
} 

int controler_released(const controler_state_t * state, uint32 mask)
{
  return ~state->buttons & state->buttons_change & mask;
} 

void controler_print(void)
{
  dbglog(DBG_DEBUG, 
	 "%c%c%c%c%c%c%c %c%c%c%c %c%c%c%c "
	 "[r:%02x] [l:%02x] [x:%02x] [y:%02x] [x2:%02x] [y2:%02x]\n",

	 (cond.buttons & CONT_A) ? 'A' : 'a',
	 (cond.buttons & CONT_B) ? 'B' : 'b',
	 (cond.buttons & CONT_C) ? 'C' : 'c',
	 (cond.buttons & CONT_D) ? 'D' : 'd',
	 (cond.buttons & CONT_X) ? 'X' : 'x',
	 (cond.buttons & CONT_Y) ? 'Y' : 'y',
	 (cond.buttons & CONT_Z) ? 'Z' : 'z',

	 (cond.buttons & CONT_DPAD_UP) ? 'U' : 'u',
	 (cond.buttons & CONT_DPAD_DOWN) ? 'D' : 'd',
	 (cond.buttons & CONT_DPAD_LEFT) ? 'L' : 'l',
	 (cond.buttons & CONT_DPAD_RIGHT) ? 'R' : 'r',

	 (cond.buttons & CONT_DPAD2_UP) ? 'U' : 'u',
	 (cond.buttons & CONT_DPAD2_DOWN) ? 'D' : 'd',
	 (cond.buttons & CONT_DPAD2_LEFT) ? 'L' : 'l',
	 (cond.buttons & CONT_DPAD2_RIGHT) ? 'R' : 'r',
    
	 cond.rtrig, cond.ltrig,
	 cond.joyx, cond.joyy,
	 cond.joy2x, cond.joy2y);
}



int controler_getchar()
{
  int k;

  for ( ;; ) {
    
    k = controler_peekchar();
    if (k != -1)
      return k;

    thd_pass();
  }

}



// defined in src/keyboard.c
extern int kbd_poll_repeat(uint8 addr, int elapsed_frame);

int controler_peekchar()
{
  //static last_frame = -1;
  int k;


  //if (ta_state.frame_counter != last_frame) {
    spinlock_lock(&controler_mutex);
    k = kbd_get_key();
    spinlock_unlock(&controler_mutex);

    //last_frame = ta_state.frame_counter;
    
    if (k != -1)
      return k;
    //}

  return -1;
}
