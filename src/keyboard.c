/* Adapted to have key repeats ! */

/* KallistiOS 1.1.5

   keyboard.c
   (C)2000-2001 Jordan DeLong and Dan Potter
*/

#include <dc/maple.h>
#include <dc/keyboard.h>
#include <string.h>

#include "controler.h"


/* in controler.c */
extern cont_cond_t controler_cond[4]; /* all joystick status */
extern int controler_cond_connected[4];


static int kbd_frame_matrix[256] = {0};

/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. This needs to be called
   fairly periodically if you're expecting keyboard input. */
int kbd_poll_repeat(uint8 addr, int elapsed_frame) {
	kbd_cond_t cond;
	int i;
	int j;
	
	/* Poll the keyboard for currently pressed keys */
	if (kbd_get_cond(addr, &cond)) {
		return -1;
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

				kbd_frame_matrix[k] = p? kbd_frame_matrix[k]+3 : 20;
			}
		}
	}

	/* Process all joystick */
#define CONTROLER_DEAD          15
	for (j=0; j<4; j++) {
	  int bits;

	  if (!controler_cond_connected[j])
	    continue;

	  bits = controler_cond[j].buttons;
	  for (i=0; i<16; i++, bits>>=1) {
	    int v = !(bits&1)? 64 : 0;
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
	    if (v > CONTROLER_DEAD) {
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
