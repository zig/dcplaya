/* 2002/02/20 */

#ifndef _OPTION_H_
#define _OPTION_H_

typedef enum {
  OPTION_LCD_VISUAL_NONE,
  OPTION_LCD_VISUAL_SCOPE,
  OPTION_LCD_VISUAL_FFT_FULL,
  OPTION_LCD_VISUAL_FFT_HALF
} option_lcd_visual_e;

int option_volume();
int option_filter();
int option_visual();
int option_lcd_visual();
int option_shuffle();

int option_setup(void);

void option_render(unsigned int elapsed_frame);

#endif /* #ifndef _OPTION_H_ */
