/* 2002/02/13 */

#ifndef _CONTROLER_H_
#define _CONTROLER_H_

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
int controler_read(controler_state_t * state, uint32 frame);
int controler_pressed(const controler_state_t * state, uint32 mask);
int controler_released(const controler_state_t * state, uint32 mask);

void controler_print(void);

#endif /* #ifndef _CONTROLER_H_ */

