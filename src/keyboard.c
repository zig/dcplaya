/* VP : Adapted to have key repeats ! */


/* KallistiOS ##version##

   keyboard.c
   (C)2002 Dan Potter
*/

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <dc/maple.h>
#include <dc/maple/keyboard.h>

CVSID("$Id: keyboard.c,v 1.9 2004-07-31 22:55:19 vincentp Exp $");

/*

This module is an (almost) complete keyboard system. It handles key
debouncing and queueing so you don't miss any pressed keys as long
as you poll often enough. The only thing missing currently is key
repeat handling.

*/


static int kbd_frame_matrix2[64] = {0};
static int kbd_frame_matrix[256] = {0};
static int kbd_matrix[256] = {0};
static int matrix[256];

int kbd_present = 2;


# define STOPIRQ \
 	if (!irq_inside_int()) { oldirq = irq_disable(); } else

# define STARTIRQ \
	if (!irq_inside_int()) { irq_restore(oldirq); } else


/* The keyboard queue (global for now) */
#define KBD_QUEUE_SIZE 16
static volatile int	kbd_queue_active = 1;
static volatile int	kbd_queue_tail = 0, kbd_queue_head = 0;
static volatile uint16	kbd_queue[KBD_QUEUE_SIZE];

/* Turn keyboard queueing on or off. This is mainly useful if you want
   to use the keys for a game where individual keypresses don't mean
   as much as having the keys up or down. Setting this state to
   a new value will clear the queue. */
void dcp_kbd_set_queue(int active) {
	if (kbd_queue_active != active) {
		kbd_queue_head = kbd_queue_tail = 0;
	}
	kbd_queue_active = active;
}

/* Take a key scancode, encode it appropriately, and place it on the
   keyboard queue. At the moment we assume no key overflows. */

static int kbd_enqueue2(uint16 keycode)
{
  kbd_queue[kbd_queue_head] = keycode;
  kbd_queue_head = (kbd_queue_head + 1) & (KBD_QUEUE_SIZE-1);
}


static int kbd_enqueue(kbd_state_t *state, uint8 keycode) {
	static char keymap_noshift[] = {
	/*0*/	0, 0, 0, 0, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
		'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
		'u', 'v', 'w', 'x', 'y', 'z',
	/*1e*/	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	/*28*/	13, 27, 8, 9, 32, '-', '=', '[', ']', '\\', 0, ';', '\'',
	/*35*/	'`', ',', '.', '/', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*46*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*53*/	0, '/', '*', '-', '+', 13, '1', '2', '3', '4', '5', '6',
	/*5f*/	'7', '8', '9', '0', '.', 0
	};
	static char keymap_shift[] = {
	/*0*/	0, 0, 0, 0, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
		'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
		'U', 'V', 'W', 'X', 'Y', 'Z',
	/*1e*/	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
	/*28*/	13, 27, 8, 9, 32, '_', '+', '{', '}', '|', 0, ':', '"',
	/*35*/	'~', '<', '>', '?', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*46*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/*53*/	0, '/', '*', '-', '+', 13, '1', '2', '3', '4', '5', '6',
	/*5f*/	'7', '8', '9', '0', '.', 0
	};
	uint16 ascii;

	/* If queueing is turned off, don't bother */
	if (!kbd_queue_active)
		return 0;

	/* Figure out its key queue value */	
	if (state && 
	    keycode < sizeof(keymap_noshift)/sizeof(keymap_noshift[0])) {
	  if (state->shift_keys & (KBD_MOD_LSHIFT|KBD_MOD_RSHIFT))
	    ascii = keymap_shift[keycode];
	  else
	    ascii = keymap_noshift[keycode];
	}
	  
	if (ascii == 0)
		ascii = ((uint16)keycode) << 8;
		
	/* Ok... now do the enqueue */
	kbd_queue[kbd_queue_head] = ascii;
	kbd_queue_head = (kbd_queue_head + 1) & (KBD_QUEUE_SIZE-1);

	return 0;
}	

/* Take a key off the key queue, or return -1 if there is none waiting */
int dcp_kbd_get_key() {
	int rv;

	/* If queueing isn't active, there won't be anything to get */
	if (!kbd_queue_active)
		return -1;
	
	/* Check available */
	if (kbd_queue_head == kbd_queue_tail)
		return -1;
	
	rv = kbd_queue[kbd_queue_tail];
	kbd_queue_tail = (kbd_queue_tail + 1) & (KBD_QUEUE_SIZE - 1);

	return rv;
}

/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. This needs to be called
   fairly periodically if you're expecting keyboard input. */
static void kbd_check_poll(maple_frame_t *frm) {
	kbd_state_t	*state;
	kbd_cond_t	*cond;
	int		i, p;

	state = (kbd_state_t *)frm->dev->status;
	cond = (kbd_cond_t *)&state->cond;

	/* Check the shift state */
	state->shift_keys = cond->modifiers;

    /* Process all pressed keys */
    for (i=0; i<6; i++) {
      int k = cond->keys[i];
      if (k != 0) {
	int p = matrix[k];
	matrix[k] = 2;	/* 2 == currently pressed */
	if (!p || kbd_frame_matrix[k] <= 0) {
	  kbd_enqueue(state, k);
	  kbd_frame_matrix[k] = p ? kbd_frame_matrix[k]+3 : 20;
	}
      }
    }

  /* Now normalize the key matrix */
  for (i=0; i<256; i++) {
    if (matrix[i] == 1)
      matrix[i] = 0;
    else if (matrix[i] == 2) {
      if (kbd_frame_matrix[i] > 0)
	kbd_frame_matrix[i] -= 1; //elapsed_frame;
      matrix[i] = 1;
    }

    /*		if (kbd_frame_matrix[i])
		printf("%d %d\n", i, kbd_frame_matrix[i]);*/
  }


#if 0
	/* Process all pressed keys */
	for (i=0; i<6; i++) {
		if (cond->keys[i] != 0) {
			p = state->matrix[cond->keys[i]];
			state->matrix[cond->keys[i]] = 2;	/* 2 == currently pressed */
			if (p == 0)
				kbd_enqueue(state, cond->keys[i]);
		}
	}
	
	/* Now normalize the key matrix */
	for (i=0; i<256; i++) {
		if (state->matrix[i] == 1)
			state->matrix[i] = 0;
		else if (state->matrix[i] == 2)
			state->matrix[i] = 1;
		else if (state->matrix[i] != 0) {
			assert_msg(0, "invalid key matrix array detected");
		}
	}
#endif
}

static void kbd_reply(maple_frame_t *frm) {
	maple_response_t	*resp;
	uint32			*respbuf;
	kbd_state_t		*state;
	kbd_cond_t		*cond;

	/* Unlock the frame (it's ok, we're in an IRQ) */
	maple_frame_unlock(frm);

	/* Make sure we got a valid response */
	resp = (maple_response_t *)frm->recv_buf;
	if (resp->response != MAPLE_RESPONSE_DATATRF)
		return;
	respbuf = (uint32 *)resp->data;
	if (respbuf[0] != MAPLE_FUNC_KEYBOARD)
		return;

	/* Update the status area from the response */
	if (frm->dev) {
		state = (kbd_state_t *)frm->dev->status;
		cond = (kbd_cond_t *)&state->cond;
		memcpy(cond, respbuf+1, (resp->data_len-1) * 4);
		frm->dev->status_valid = 1;
		kbd_check_poll(frm);
	}
}

static int kbd_poll_intern(maple_device_t *dev) {
	uint32 * send_buf;

	if (maple_frame_lock(&dev->frame) < 0)
		return 0;

	maple_frame_init(&dev->frame);
	send_buf = (uint32 *)dev->frame.recv_buf;
	send_buf[0] = MAPLE_FUNC_KEYBOARD;
	dev->frame.cmd = MAPLE_COMMAND_GETCOND;
	dev->frame.dst_port = dev->port;
	dev->frame.dst_unit = dev->unit;
	dev->frame.length = 1;
	dev->frame.callback = kbd_reply;
	dev->frame.send_buf = send_buf;
	maple_queue_frame(&dev->frame);

	return 0;
}

static void kbd_periodic(maple_driver_t *drv) {
	maple_driver_foreach(drv, kbd_poll_intern);
}

static int kbd_attach(maple_driver_t *drv, maple_device_t *dev) {
	memset(dev->status, 0, sizeof(dev->status));
	dev->status_valid = 0;
	return 0;
}

static void kbd_detach(maple_driver_t *drv, maple_device_t *dev) {
	memset(dev->status, 0, sizeof(dev->status));
	dev->status_valid = 0;
}

/* Device driver struct */
static maple_driver_t kbd_drv = {
	functions:	MAPLE_FUNC_KEYBOARD,
	name:		"Keyboard Driver",
	periodic:	kbd_periodic,
	attach:		kbd_attach,
	detach:		kbd_detach
};

/* Add the keyboard to the driver chain */
int dcp_kbd_init() {
	return maple_driver_reg(&kbd_drv);
}

void dcp_kbd_shutdown() {
	maple_driver_unreg(&kbd_drv);
}







/* KallistiOS 1.1.5

keyboard.c
(C)2000-2001 Jordan DeLong and Dan Potter
*/

#include <dc/maple.h>
#include <dc/maple/keyboard.h>
#include <string.h>

#include "dcplaya/config.h"
#include "controler.h"
#include "sysdebug.h"

/* in controler.c */
cont_cond_t * controler_get_cond(int idx);

//extern cont_cond_t controler_cond[4]; /* all joystick status */
//extern int controler_cond_connected[4];


/* Update the keyboard status; this will handle debounce handling as well as
   queueing keypresses for later usage. The key press queue uses 16-bit
   words so that we can store "special" keys as such. This needs to be called
   fairly periodically if you're expecting keyboard input. */
int kbd_poll_repeat(int elapsed_frame)
{
  kbd_cond_t cond;
  int i, j;
  int oldirq;

#if 0
#ifdef NOTYET
  /* Poll the keyboard for currently pressed keys */
  if (addr && !kbd_get_cond(addr, &cond)) {
    if ((kbd_present&255) != 1) {
      SDNOTICE("[kbd_poll_repeat] : keyboard detected\n");
      kbd_present = 0x101;
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

  } else 
#endif
  {
    if ((kbd_present&255) != 0) {
      SDNOTICE("[kbd_poll_repeat] : no keyboard\n");
      kbd_present = 0x100;
    }
    kbd_shift_keys = 0;
  }
#endif

  /* Process all joystick */
#define CONTROLER_JOY_DEAD          48
#define CONTROLER_TRIG_DEAD         15
  STOPIRQ;
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
	int k = j*16 + i;
	int p = kbd_matrix[128+k];
	kbd_matrix[128+k] = 2;	/* 2 == currently pressed */
	if (!p || kbd_frame_matrix2[k] <= 0) {
	  kbd_enqueue2((k+128)<<8);
		
	  kbd_frame_matrix2[k] = p? kbd_frame_matrix2[k]+3 : 20;
	}
      }
    }
  }
  STARTIRQ;

#if 1
  /* Now normalize the key matrix */
  for (i=0; i<sizeof(kbd_frame_matrix2)/sizeof(kbd_frame_matrix2[0]); i++) {
    if (kbd_matrix[128+i] == 1)
      kbd_matrix[128+i] = 0;
    else if (kbd_matrix[128+i] == 2) {
      if (kbd_frame_matrix2[i] > 0)
	kbd_frame_matrix2[i] -= elapsed_frame;
      kbd_matrix[128+i] = 1;
    }

    /*		if (kbd_frame_matrix[i])
		printf("%d %d\n", i, kbd_frame_matrix[i]);*/
  }
#endif
	
  return 0;
}
