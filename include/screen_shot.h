/**
 * @file    screen_shot.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/14
 * @brief   Takes TGA screen shot.
 * 
 * $Id: screen_shot.h,v 1.2 2003-02-12 12:31:56 ben Exp $
 */

#ifndef _SCREEN_SHOT_H_
#define _SCREEN_SHOT_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START

/** Takes a TGA screen shot.
 *
 *     The screen_shot() function saves current frame buffer into a TGA
 *     file in the DCPLAYA_HOME directory. Filenames will be auto
 *     incremented.
 *
 *  @param  basename  Screen-shot file name without extension.
 *
 *  @return  error-code
 *  @retval  0 success
 *  @retval <0 failure
 */
int screen_shot(const char *basename);

DCPLAYA_EXTERN_C_END

#endif /* #define _SCREEN_SHOT_H_ */
