/* 2002/02/21 */

#ifndef _VIEWPORT_H_
#define _VIEWPORT_H_

typedef struct
{
  float tx; ///< Translation XM
  float ty; ///< Translation YM
  float mx; ///< Multiplier X ( scale * w * 0.5 )M
  float my; ///< Multiplier Y ( scale * h * 0.5 )M
} viewport_t;

void viewport_set(viewport_t *v, int posX, int posY, int width, int height, const float scale);
void viewport_apply(viewport_t *v, float *d, int dbytes, const float *s, int sbytes, int nb);

#endif /* #ifndef _VIEWPORT_H_ */

