/* Adapted to have key repeats ! */

/* KallistiOS 1.1.5

   keyboard.c
   (C)2000-2001 Jordan DeLong and Dan Potter
*/

#include <dc/maple.h>
#include <dc/keyboard.h>
#include <string.h>


static int kbd_frame_matrix[256] = {0};

/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. This needs to be called
   fairly periodically if you're expecting keyboard input. */
int kbd_poll_repeat(uint8 addr, int elapsed_frame) {
	kbd_cond_t cond;
	int i;
	
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
	
	/* Now normalize the key matrix */
	for (i=0; i<256; i++) {
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
