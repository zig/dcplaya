/** @ingroup dcplaya_controler_devel
 *  @file    controler.h
 *  @author  benjamin gerard
 *  @author  vincent penne
 *  @date    2002/02/13
 *  @brief   Controllers access
 *
 *  $Id: controler.h,v 1.8 2004-07-04 14:16:44 vincentp Exp $
 */


#ifndef _CONTROLER_H_
#define _CONTROLER_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** @defgroup dcplaya_controler_devel Controllers
 *  @ingroup  dcplaya_devel
 *  @brief    read controller status
 *  @remark   All functions and variables of this modules used a wrong 
 *  orthography for the word controller with a missing 'L'.
 *
 *  @author   benjamin gerard
 *  @author   vincent penne
 *  @{
 */

#include <dc/maple/controller.h>

/** controller state. */
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

/** Inititialize the controller system. */
int controler_init(void);
/** Shutdown the controller system. */
void controler_shutdown(void);

/** Get state of a controller.
 *  @param state  controller state to fill
 *  @param idx    controller identifier (0 based)
 */
int controler_read(controler_state_t * state, unsigned int idx);

/** Test if some buttons are currently pressed.
 *  @param   state  filled controller state
 *  @param   mask   buttons to test
 *  @return  pressed button mask
 */
int controler_pressed(const controler_state_t * state, uint32 mask);

/** Test if some buttons have been released.
 *  @param   state  filled controller state
 *  @param   mask   buttons to test
 *  @return  released button mask
 */
int controler_released(const controler_state_t * state, uint32 mask);

/** Set controller keyboard binding status.
 *
 *    Each controller may be bind to keyboard or not. When binded a
 *    controller generates key-code. The binding status is a bit field
 *    value where bit 0 is the binding of the first controller...
 *    The controler_binding() function performs the following operation :
 * @code
 * int controler_binding(int clear, int modify)
 * {
 *   int old = cond_connected_mask;  // Save current status;
 *   cond_connected_mask &= ~clear;  // Clear bits
 *   cond_connected_mask ^= ~modify; // Toggle bits
 *   return old;
 * }
 * @endcode
 *
 *    So if you want to set a bit, you just have to set it in both
 *    clear and modify. If you want to clear a bit, you set it in clear only
 *    and finally if you want to toggle a bit youy set it in modify only.
 *
 *  @param clear   bits to clear
 *  @param modify  bits to toggle
 *  @return old binding.
 */
int controler_binding(int clear, int modify);

/** @name Key code function.
 *
 *    Controllers buttons and pad directions are mapped to some special
 *    keycode used by
 *    @link dcplaya_events event system @endlink.
 *
 * @warning The keycode are defined up-to 4 controllers whereas the controller
 * has a define that allow up-to 32 controllers.
 *
 * @{
 */
 
/** Get a keycode. */
int controler_getchar(void);
/** Peek a keycode. */
int controler_peekchar(void);

/**@}*/

/* void controler_print(void); */

/** @name Controllers key codes.
 *
 *  Defines keycodes binded to controler buttons and moves.
 *  @{
 */

/** Keycodes for 1st controller. */
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

/** Keycodes for 2nd controller. */
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

/** Keycodes for 3td controller. */
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

/** Keycodes for 4th controller. */
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

/** Keycodes for Nth controller. */
#define KBD_CON_C(N)		(0x80+((N)<<4))
#define KBD_CON_B(N)		(0x81+((N)<<4))
#define KBD_CON_A(N)		(0x82+((N)<<4))
#define KBD_CON_START(N)	(0x83+((N)<<4))
#define KBD_CON_DPAD_UP(N)      (0x84+((N)<<4))
#define KBD_CON_DPAD_DOWN(N)	(0x85+((N)<<4))
#define KBD_CON_DPAD_LEFT(N)	(0x86+((N)<<4))
#define KBD_CON_DPAD_RIGHT(N)	(0x87+((N)<<4))
#define KBD_CON_Z(N)		(0x88+((N)<<4))
#define KBD_CON_Y(N)		(0x89+((N)<<4))
#define KBD_CON_X(N)		(0x8a+((N)<<4))
#define KBD_CON_D(N)		(0x8b+((N)<<4))
#define KBD_CON_DPAD2_UP(N)	(0x8c+((N)<<4))
#define KBD_CON_DPAD2_DOWN(N)	(0x8d+((N)<<4))
#define KBD_CON_DPAD2_LEFT(N)	(0x8e+((N)<<4))
#define KBD_CON_DPAD2_RIGHT(N)	(0x8f+((N)<<4))

/**@}*/

/**@}*/

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _CONTROLER_H_ */

