/* 2002/02/13  */

#include <kos.h>
#include <dc/maple.h>
#include <dc/cdrom.h>

#include "dc/ta.h"

#include "dcplaya/config.h"
#include "controler.h"
#include "sysdebug.h"

#include "priorities.h"

#define CONTROLER_NO_SMOOTH_FRAMES  50      /* > 2 */
#define CONTROLER_SMOOTH_FACTOR     0x6000  /* [0..65536] */
#define CONTROLER_JOY_DEAD          15
#define CONTROLER_TRIG_DEAD         15

#define MAX_CONTROLER 4u
typedef struct {
  cont_cond_t cond;
  cont_cond_t oldcond;
  int connected;
  int port;
  int unit;
  uint32 last_frame;

  controler_state_t state;

} controler_t;

static controler_t controlers[MAX_CONTROLER];
static int controler_connected, cond_connected_mask;

static uint32 last_frame;

static const uint32 controler_smooth_mult =
CONTROLER_SMOOTH_FACTOR / (CONTROLER_NO_SMOOTH_FRAMES-2);
//  ((CONTROLER_NO_SMOOTH_FRAMES-1)<<16) / CONTROLER_SMOOTH_FACTOR;

/* Controler thread globales */
static kthread_t * controler_thd;
static spinlock_t controler_mutex;
enum { RUNNING, QUIT, ZOMBIE };
static int status;

static void clear_cond(cont_cond_t * cond)
{
  cond->buttons = ~0;
  cond->rtrig = cond->ltrig = 0;
  cond->joyx = cond->joyy = cond->joy2x = cond->joy2y = 128;
}

static void clear_all_cond(void)
{
  controler_t * cont;
  for (cont=controlers; cont<controlers+MAX_CONTROLER; ++cont) {
    clear_cond(&cont->cond);
  }
}

/* try to get controler cond or clear it. */
static void controler_get(void)
{
  int port, unit, cur_cont;

  controler_connected = 0;

  /* Scanning mapple bus until max controler found. */
  for (controler_connected =cur_cont = port = 0; port < 4; ++port) {
    for (unit=0; unit<6 && cur_cont < MAX_CONTROLER; ++unit) {
      int func = maple_device_func(port, unit);
      if ((func & (MAPLE_FUNC_CONTROLLER|MAPLE_FUNC_KEYBOARD)) ==
	  MAPLE_FUNC_CONTROLLER) {
	int adr = maple_create_addr(port, unit);
	cont_get_cond(adr, &controlers[cur_cont].cond);
	controlers[cur_cont].connected = 1;
	controlers[cur_cont].port = port;
	controlers[cur_cont].unit = unit;
	controler_connected |= 1<<cur_cont;
	if (++cur_cont >= MAX_CONTROLER) {
	  goto out;
	}
      }
    }
  }
 out:  
  /* Disconect others controlers. */
  while (cur_cont < MAX_CONTROLER) {
    controlers[cur_cont].connected = 0;
    clear_cond(&controlers[cur_cont].cond);
    ++cur_cont;
  }
}

static int rescale(int v, const int dead, const int shift)
{
  v -= dead;
  if (v < 0) {
    v = 0;
  } else {
    v = (v << shift) / ((1<<shift) - dead);
  }
  return v;
}

static int rescale2(int v, const int dead, const int shift)
{
  int v2;
  const int max = 1 << shift;

  v -= max; /* [ -128 127 ] */
  v2 = v - (dead ^ (v>>31)); /* -127 - 127 */
  if ((v^v2) < 0) {
    v = 0;
  } else {
    v = (v2<<shift) / (max - 1 - dead);
  }
  return v;
}

static void fill_controler_state(controler_state_t * state, controler_t * cont,
				 uint32 elapsed)
{
  const cont_cond_t * cond = &cont->cond;
  cont_cond_t * oldcond = &cont->oldcond;

  state->elapsed_frames = elapsed;
  state->buttons = ~cond->buttons;
  state->buttons_change = cond->buttons ^ oldcond->buttons;
  
  /*   state->rtrig = (cond->rtrig < CONTROLER_TRIG_DEAD) ? 0 : cond->rtrig; */
  /*   state->ltrig = (cond->ltrig < CONTROLER_TRIG_DEAD) ? 0 : cond->ltrig; */
  state->rtrig = rescale(cond->rtrig,CONTROLER_TRIG_DEAD,8);
  state->ltrig = rescale(cond->ltrig,CONTROLER_TRIG_DEAD,8);

  state->joyx  = rescale2(cond->joyx,CONTROLER_JOY_DEAD,7);
  state->joyy  = rescale2(cond->joyy,CONTROLER_JOY_DEAD,7);
  state->joy2x = rescale2(cond->joy2x,CONTROLER_JOY_DEAD,7);
  state->joy2y = rescale2(cond->joy2y,CONTROLER_JOY_DEAD,7);

  *oldcond = *cond;
  cont->last_frame += elapsed;
}

static void controler_smooth(uint32 factor,
			     cont_cond_t * cond,
			     const cont_cond_t * oldcond)
{
  uint32 oofactor = 65536 - factor;
  cond->rtrig = (cond->rtrig * factor + oldcond->rtrig * oofactor) >> 16;
  cond->ltrig = (cond->ltrig * factor + oldcond->ltrig * oofactor) >> 16;
  cond->joyx  = (cond->joyx  * factor + oldcond->joyx  * oofactor) >> 16;
  cond->joyy  = (cond->joyy  * factor + oldcond->joyy  * oofactor) >> 16;
  cond->joy2x = (cond->joy2x * factor + oldcond->joy2x * oofactor) >> 16;
  cond->joy2y = (cond->joy2y * factor + oldcond->joy2y * oofactor) >> 16;
}

// defined in src/keyboard.c
extern int kbd_poll_repeat(int elapsed_frame);

// from dreamcast68.c 
extern controler_state_t controler68;

static void controler_thread(void * dummy)
{
  while (status != QUIT) {

    uint32 frame = ta_state.frame_counter;
    uint32 elapsed_frame = last_frame;
    controler_t * cont;
    uint32 factor = 65536;
	
    elapsed_frame = frame - elapsed_frame;

    if (elapsed_frame) {
      static int report = 0;
      int oldfunc;

      spinlock_lock(&controler_mutex);


      if ((controler68.buttons & (CONT_START|CONT_A|CONT_Y)) == 
	  (CONT_START|CONT_A|CONT_Y))
	panic("Quick exit (controler_thread) !\n");


#ifdef NOTYET
      /* rescan one unit per frame */
      oldfunc = maple_device_func(report, 0);
      /* first unit of given port */
      maple_rescan_unit(0, report, 0);
      //printf("func %d = %d\n", report, oldfunc);
      if (maple_device_func(report, 0) != oldfunc) {
	int unit;
	/* rescan also other units of same port if some change happened */
	for (unit=1; unit<6; unit++)
	  maple_rescan_unit(0, report, unit);
      }

      if (++report >= 4) {
	report = 0;
      }
#endif
	  
      /* Get controler cond, scan for current number of controler ... */
      controler_get();
	  
      if (elapsed_frame < CONTROLER_NO_SMOOTH_FRAMES) {
	int i;
	factor = 65536 - CONTROLER_SMOOTH_FACTOR;
	/* $$$ ben : this this a crap method to perform a power ! */
	for (i=1; i<elapsed_frame; ++i) {
	  factor = (factor * factor) >> 16;
	}
	factor = 65536 - factor;
      }

      /* Fill controler state */
      for (cont=controlers; cont<controlers+MAX_CONTROLER; ++cont) {
	if (!cont->connected) continue;
	if (factor < 65536) {
	  controler_smooth(factor, &cont->cond, &cont->oldcond);
	}
	fill_controler_state(&cont->state, cont, elapsed_frame);
      }
	  
      /* keyboard emulation */
      kbd_poll_repeat(ta_state.frame_counter - last_frame);
	  
      last_frame = frame;
      spinlock_unlock(&controler_mutex);
    }
	
    //thd_pass();
    usleep(1000000/60/2);
  }
  
  status = ZOMBIE;
}


int controler_init(void)
{
  controler_t * cont;
  int err = 0;
  
  SDDEBUG("[%f]\n", __FUNCTION__ );
  SDINDENT;

  spinlock_init(&controler_mutex);
  /*   SDDEBUG("GetControler, frame=%u\n", frame); */

  cond_connected_mask = -1;
  last_frame = ta_state.frame_counter;
  controler_get();
  for (cont=controlers; cont<controlers+MAX_CONTROLER; ++cont) {
    cont->last_frame = last_frame;
    cont->oldcond = cont->cond;
  }

  status = RUNNING;
  controler_thd = thd_create(controler_thread, 0);
  controler_thd->prio2 = MAPLE_THREAD_PRIORITY;
  if (controler_thd) {
    thd_set_label(controler_thd, "Controler-thd");
  }
  
  SDUNINDENT;
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, err);

  return err;
}

void controler_shutdown()
{
  status = QUIT;
  while (status != ZOMBIE)
    thd_pass();
}

int controler_read(controler_state_t * state, unsigned int idx)
{
  controler_t * cont;
  uint32 elapsed_frame;
  int connected;
  
  if (idx >= MAX_CONTROLER) {
    return -1;
  }

  cont = controlers + idx;
  spinlock_lock(&controler_mutex);
  elapsed_frame = ta_state.frame_counter - cont->last_frame;
  *state = cont->state;
  connected = cont->connected;
  spinlock_unlock(&controler_mutex);
  return !connected;
}

int controler_pressed(const controler_state_t * state, uint32 mask)
{
  return state->buttons & state->buttons_change & mask;
} 

int controler_released(const controler_state_t * state, uint32 mask)
{
  return ~state->buttons & state->buttons_change & mask;
}

/* void controler_print(void) */
/* { */
/*   dbglog(DBG_DEBUG,  */
/* 	 "%c%c%c%c%c%c%c %c %c%c%c%c %c%c%c%c " */
/* 	 "[r:%02x] [l:%02x] [x:%02x] [y:%02x] [x2:%02x] [y2:%02x]\n", */

/* 	 (cond.buttons & CONT_A) ? 'A' : 'a', */
/* 	 (cond.buttons & CONT_B) ? 'B' : 'b', */
/* 	 (cond.buttons & CONT_C) ? 'C' : 'c', */
/* 	 (cond.buttons & CONT_D) ? 'D' : 'd', */
/* 	 (cond.buttons & CONT_X) ? 'X' : 'x', */
/* 	 (cond.buttons & CONT_Y) ? 'Y' : 'y', */
/* 	 (cond.buttons & CONT_Z) ? 'Z' : 'z', */

/* 	 (cond.buttons & CONT_START) ? 'S' : 's', */

/* 	 (cond.buttons & CONT_DPAD_UP) ? 'U' : 'u', */
/* 	 (cond.buttons & CONT_DPAD_DOWN) ? 'D' : 'd', */
/* 	 (cond.buttons & CONT_DPAD_LEFT) ? 'L' : 'l', */
/* 	 (cond.buttons & CONT_DPAD_RIGHT) ? 'R' : 'r', */

/* 	 (cond.buttons & CONT_DPAD2_UP) ? 'U' : 'u', */
/* 	 (cond.buttons & CONT_DPAD2_DOWN) ? 'D' : 'd', */
/* 	 (cond.buttons & CONT_DPAD2_LEFT) ? 'L' : 'l', */
/* 	 (cond.buttons & CONT_DPAD2_RIGHT) ? 'R' : 'r', */
    
/* 	 cond.rtrig, cond.ltrig, */
/* 	 cond.joyx, cond.joyy, */
/* 	 cond.joy2x, cond.joy2y); */
/* } */

int controler_getchar(void)
{
  int k;
  for ( ;; ) {
    k = controler_peekchar();
    if (k != -1) {
      return k;
    }
    thd_pass();
  }
}

int controler_peekchar(void)
{
  int k;
  //spinlock_lock(&controler_mutex);
  k = dcp_kbd_get_key();
  //spinlock_unlock(&controler_mutex);
  return k;
}

cont_cond_t * controler_get_cond(int idx)
{
  if ( (unsigned int)idx >= MAX_CONTROLER ||
       !(controler_connected & cond_connected_mask & (1<<idx))) {
    return 0;
  }
  return &controlers[idx].cond;
}

int controler_binding(int clear, int modify)
{
  int old = cond_connected_mask;
  cond_connected_mask = (cond_connected_mask & ~clear) ^ modify;
  SDDEBUG("[%s] : clear = [%d], modify = [%d], old = [%d]\n", __FUNCTION__, clear, modify, old);
  return old;
}
