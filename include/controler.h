/* $Id: controler.h,v 1.4 2002-09-27 06:33:54 vincentp Exp $ */
/* 2002/02/13 */

#ifndef _CONTROLER_H_
#define _CONTROLER_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include <dc/controller.h>

typedef struct
{
  uint32 elapsed_frames;  /**< elapsed frame since last previous read */
  uint16 buttons_change;  /**< buttoms change state */
  uint16 buttons;         /**< buttons state */
  uint8 rtrig;            /**< right trigger */
  uint8 ltrig;            /**< left trigger */
  int8 joyx;              /**< joystick X */
  int8 joyy;              /**< joystick Y */
  int8 joy2x;             /**< second joystick X */
  int8 joy2y;             /**< second joystick Y */
} controler_state_t;

int controler_init(uint32 frame);
void controler_shutdown();

int controler_read(controler_state_t * state, uint32 frame);
int controler_pressed(const controler_state_t * state, uint32 mask);
int controler_released(const controler_state_t * state, uint32 mask);

int controler_getchar();
int controler_peekchar();

void controler_print(void);


#define KBD_CONT1_C		(0x80)
#define KBD_CONT1_B		(0x81)
#define KBD_CONT1_A		(0x82)
#define KBD_CONT1_START		(0x83)
#define KBD_CONT1_DPAD_UP	(0x84)
#define KBD_CONT1_DPAD_DOWN	(0x85)
#define KBD_CONT1_DPAD_LEFT	(0x86)
#define KBD_CONT1_DPAD_RIGHT	(0x87)
#define KBD_CONT1_Z		(0x88)
#define KBD_CONT1_Y		(0x89)
#define KBD_CONT1_X		(0x8a)
#define KBD_CONT1_D		(0x8b)
#define KBD_CONT1_DPAD2_UP	(0x8c)
#define KBD_CONT1_DPAD2_DOWN	(0x8d)
#define KBD_CONT1_DPAD2_LEFT	(0x8e)
#define KBD_CONT1_DPAD2_RIGHT	(0x8f)

#define KBD_CONT2_C		(0x90)
#define KBD_CONT2_B		(0x91)
#define KBD_CONT2_A		(0x92)
#define KBD_CONT2_START		(0x93)
#define KBD_CONT2_DPAD_UP	(0x94)
#define KBD_CONT2_DPAD_DOWN	(0x95)
#define KBD_CONT2_DPAD_LEFT	(0x96)
#define KBD_CONT2_DPAD_RIGHT	(0x97)
#define KBD_CONT2_Z		(0x98)
#define KBD_CONT2_Y		(0x99)
#define KBD_CONT2_X		(0x9a)
#define KBD_CONT2_D		(0x9b)
#define KBD_CONT2_DPAD2_UP	(0x9c)
#define KBD_CONT2_DPAD2_DOWN	(0x9d)
#define KBD_CONT2_DPAD2_LEFT	(0x9e)
#define KBD_CONT2_DPAD2_RIGHT	(0x9f)

#define KBD_CONT3_C		(0xa0)
#define KBD_CONT3_B		(0xa1)
#define KBD_CONT3_A		(0xa2)
#define KBD_CONT3_START		(0xa3)
#define KBD_CONT3_DPAD_UP	(0xa4)
#define KBD_CONT3_DPAD_DOWN	(0xa5)
#define KBD_CONT3_DPAD_LEFT	(0xa6)
#define KBD_CONT3_DPAD_RIGHT	(0xa7)
#define KBD_CONT3_Z		(0xa8)
#define KBD_CONT3_Y		(0xa9)
#define KBD_CONT3_X		(0xaa)
#define KBD_CONT3_D		(0xab)
#define KBD_CONT3_DPAD2_UP	(0xac)
#define KBD_CONT3_DPAD2_DOWN	(0xad)
#define KBD_CONT3_DPAD2_LEFT	(0xae)
#define KBD_CONT3_DPAD2_RIGHT	(0xaf)

#define KBD_CONT4_C		(0xb0)
#define KBD_CONT4_B		(0xb1)
#define KBD_CONT4_A		(0xb2)
#define KBD_CONT4_START		(0xb3)
#define KBD_CONT4_DPAD_UP	(0xb4)
#define KBD_CONT4_DPAD_DOWN	(0xb5)
#define KBD_CONT4_DPAD_LEFT	(0xb6)
#define KBD_CONT4_DPAD_RIGHT	(0xb7)
#define KBD_CONT4_Z		(0xb8)
#define KBD_CONT4_Y		(0xb9)
#define KBD_CONT4_X		(0xba)
#define KBD_CONT4_D		(0xbb)
#define KBD_CONT4_DPAD2_UP	(0xbc)
#define KBD_CONT4_DPAD2_DOWN	(0xbd)
#define KBD_CONT4_DPAD2_LEFT	(0xbe)
#define KBD_CONT4_DPAD2_RIGHT	(0xbf)



DCPLAYA_EXTERN_C_END

#endif /* #ifndef _CONTROLER_H_ */

