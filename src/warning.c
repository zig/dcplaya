/* 2002/02/23 */
#include "config.h"
#include "gp.h"

extern float fade68;
extern unsigned int fade_argb(unsigned int argb);

typedef struct {
  float size;
  unsigned int argb;
  const char *s;
} warning_str_t;


static const warning_str_t warning_str[] = {

#define NORMAL_COLOR 0xFFFFFFFF
#define AUTHOR_COLOR 0xFF80FFFF
#define YEL_COLOR 0xFFFFFF00
#define RED_COLOR 0xFFFF0000

{  30.0f, RED_COLOR,     "!!! WARNING !!!"},
{  20.0f, NORMAL_COLOR,  ""},
{  22.0f, NORMAL_COLOR,  "%cdream mp3 - mp3 player for Dreamcast"},
{  20.0f, NORMAL_COLOR,  ""},
{  22.0f, YEL_COLOR,     "%cThis program is **NO** official SEGA production."},
{  20.0f, NORMAL_COLOR,  ""},
{  20.0f, NORMAL_COLOR,  "%cThis program is distributed in the hope that it will be"},
{  20.0f, NORMAL_COLOR,  "%cuseful, but WITHOUT ANY WARRANTY; without even the"},
{  20.0f, NORMAL_COLOR,  "%cimplied warranty of MERCHANTABILITY or FITNESS FOR A"},
{  20.0f, NORMAL_COLOR,  "%cPARTICULAR PURPOSE."},
{  20.0f, NORMAL_COLOR,  ""},
{  20.0f, NORMAL_COLOR,  "%cDeveloped with free software utilities"},
{  18.0f, NORMAL_COLOR,  "%cThanks to KallistiOS developers"},
{  18.0f, AUTHOR_COLOR,  "%c>Dan Potter & Jordan DeLong of Cryptic Allusion<"},
{  18.0f, NORMAL_COLOR,  "%cThanks %c>andrewk<%c for dcload"},
{  18.0f, NORMAL_COLOR,  "%cThanks to %c>Mr Bee<%c for the serial link"},
{  18.0f, NORMAL_COLOR,  0}
};

void warning_render(void)
{
  const float ys = 50;
  const float z = 80.0f;
  float y1 = ys;
  const warning_str_t * s = warning_str;
  const float spaces = 8.0f;
  float save;
  unsigned int author_color = fade_argb(AUTHOR_COLOR);
  
  save = text_set_font_size(20.0f);
  
  s=warning_str;
  
  if (s->s) {
    const float speed = 0.01f;
    static float color = 1.0f, step = -speed;

    if (color >= 1.0f) {
      color = 1.0f;
      step = -speed;
    } else if (color < 0.2f) {
      color = 0.2f;
      step = speed;
    }
  
    text_set_font_size(s->size);
    y1 += spaces + draw_poly_center_text(y1, z, fade68, 1.0f, color, color, s->s, fade_argb(s->argb));
    
    color += step;
    ++s;    
  }
  
  for (; s->s; ++s) {
    if (s->s[0]) {
      unsigned int color1 = fade_argb(s->argb);
      
      text_set_font_size(s->size);
      y1 += spaces + draw_poly_center_text(y1, z, fade68, 1.0f, 1.0f, 1.0f, s->s, color1, author_color, color1);
    } else {
      y1 += 1.5f * spaces;
    }
  }
  
  text_set_font_size(save);
}
