/* 2002/02/20 */

#ifndef _OPTION_H_
#define _OPTION_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#include "vis_driver.h"

typedef enum {
  OPTION_LCD_VISUAL_NONE,
  OPTION_LCD_VISUAL_SCOPE,
  OPTION_LCD_VISUAL_FFT,
  OPTION_LCD_VISUAL_BAND,
} option_lcd_visual_e;

int option_volume();
int option_filter();
vis_driver_t * option_visual();
void option_no_visual();
int option_lcd_visual();
int option_shuffle();

int option_setup(void);

void option_render(unsigned int elapsed_frame);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _OPTION_H_ */
