/* Adapted to have key repeats ! */

/* KallistiOS 1.1.5

keyboard.c
(C)2000-2001 Jordan DeLong and Dan Potter
*/

#include <dc/maple.h>
#include <dc/keyboard.h>
#include <string.h>

#include "dcplaya/config.h"
#include "controler.h"
#include "sysdebug.h"

/* in controler.c */
cont_cond_t * controler_get_cond(int idx);

//extern cont_cond_t controler_cond[4]; /* all joystick status */
//extern int controler_cond_connected[4];


static int kbd_frame_matrix[256] = {0};

/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. This needs to be called
   fairly periodically if you're expecting keyboard input. */
int kbd_poll_repeat(uint8 addr, int elapsed_frame)
{
  kbd_cond_t cond;
  int i, j;
  static int kbd_ok = -1;
	
  /* Poll the keyboard for currently pressed keys */
  if (!kbd_get_cond(addr, &cond)) {
    if (kbd_ok != 1) {
      SDNOTICE("[kbd_poll_repeat] : keyboard detected\n");
      kbd_ok = 1;
    }
    /* Check the shift state */
    kbd_shift_keys = cond.modifiers;
	
    /* Process all pressed keys */
    for (i=0; i<6; i++) {
      int k = cond.keys[i];
      if (k != 0) {
	int p = kbd_matrix[k];
	kbd_matrix[k] = 2;	/* 2 == currently pressed */
	if (!p || kbd_frame_matrix[k] <= 0) {
	  kbd_enqueue(k);
	  kbd_frame_matrix[k] = p ? kbd_frame_matrix[k]+3 : 20;
	}
      }
    }

  } else {
    if (kbd_ok != 0) {
      SDNOTICE("[kbd_poll_repeat] : no keyboard\n");
      kbd_ok = 0;
    }
    kbd_shift_keys = 0;
  }

  /* Process all joystick */
#define CONTROLER_JOY_DEAD          48
#define CONTROLER_TRIG_DEAD         15
  for (j=0; j<4; j++) {
    int bits;
    cont_cond_t * cond = controler_get_cond(j);

    if (!cond)
      continue;

    bits = cond->buttons;
    for (i=0; i<16; i++, bits>>=1) {

#define O(e) ( offsetof(cont_cond_t, e) )

      static struct {
	int sign, addr, offset;
	int dead;
	int coef;
      } d[] = {
	{ +1, O(rtrig), 0, CONTROLER_TRIG_DEAD, 1.0f * (1<<16) },  // 0 : RTRIG
	{ 0 }, // 1
	{ 0 }, // 2
	{ 0 }, // 3
	{ 0 }, // 4
	{ 0 }, // 5
	{ 0 }, // 6
	{ 0 }, // 7
	{ 0 }, // 8
	{ 0 }, // 9
	{ 0 }, // 10
	{ +1, O(ltrig), 0, CONTROLER_TRIG_DEAD, 1.0f * (1<<16) },  // 11 : LTRIG
	{ -1, O(joyy), 128, CONTROLER_JOY_DEAD, 1.0f * (1<<16) }, // 12 : PAD2_UP
	{ +1, O(joyy), 128, CONTROLER_JOY_DEAD, 1.0f * (1<<16) }, // 13 : PAD2_DOWN
	{ -1, O(joyx), 128, CONTROLER_JOY_DEAD, 1.0f * (1<<16) }, // 14 : PAD2_LEFT
	{ +1, O(joyx), 128, CONTROLER_JOY_DEAD, 1.0f * (1<<16) }, // 15 : PAD2_RIGHT
      };

      int v = !(bits&1)? 128 : 0;
      int s = d[i].sign;
      if (s) {
	uint8 * p = (uint8 *) (cond);
	v = (s * ( ( (int)p[d[i].addr] ) - d[i].offset) - d[i].dead) * d
	  [i].coef >> 16;
      }
#if 0
      switch (i) {
      case 0:
	v = controler_cond[j].rtrig;
	break;
      case 11:
	v = controler_cond[j].ltrig;
	break;
	/*	    case 4:
		    v = 128 - controler_cond[j].joy2y;
		    break;
		    case 5:
		    v = controler_cond[j].joy2y - 128;
		    break;
		    case 6:
		    v = 128 - controler_cond[j].joy2x;
		    break;
		    case 7:
		    v = controler_cond[j].joy2x - 128;
		    break;*/
      case 12:
	v = 128 - controler_cond[j].joyy;
	break;
      case 13:
	v = controler_cond[j].joyy - 128;
	break;
      case 14:
	v = 128 - controler_cond[j].joyx;
	break;
      case 15:
	v = controler_cond[j].joyx - 128;
	break;
      }
#endif
      if (v > 0) {
	int k = 128 + j*16 + i;
	int p = kbd_matrix[k];
	kbd_matrix[k] = 2;	/* 2 == currently pressed */
	if (!p || kbd_frame_matrix[k] <= 0) {
	  kbd_enqueue(k);
		
	  kbd_frame_matrix[k] = p? kbd_frame_matrix[k]+3 : 20;
	}
      }
    }
  }
	
  /* Now normalize the key matrix */
  for (i=0; i<sizeof(kbd_frame_matrix)/sizeof(kbd_frame_matrix[0]); i++) {
    if (kbd_matrix[i] == 1)
      kbd_matrix[i] = 0;
    else if (kbd_matrix[i] == 2) {
      if (kbd_frame_matrix[i] > 0)
	kbd_frame_matrix[i] -= elapsed_frame;
      kbd_matrix[i] = 1;
    }

    /*		if (kbd_frame_matrix[i])
		printf("%d %d\n", i, kbd_frame_matrix[i]);*/
  }
	
  return 0;
}
