/**
 * @ingroup dcplaya_border_devel
 * @file    border.h
 * @author  benjamin gerard
 * @date    2002/02/16
 * @brief   border texture
 */

#ifndef _BORDER_H_
#define _BORDER_H_

#include "draw/texture.h"
#include "draw/color.h"

/** @defgroup  dcplaya_border_devel  Border Texture
 *  @ingroup   dcplaya_devel
 *  @brief     border texture
 *
 *    This modules handles texture for rendering special FX used by dcplaya.
 *
 *  @author  benjamin gerard
 *  @{
 */

/** border definition.
 */
typedef draw_color_t border_def_t[3]; 

/** Standard border tile texture-id, mapped on "bordertile2".
 */
extern texid_t bordertex;

/** Init border texture.
 */
int border_init(void);

/** Customize a given border texture.
 */ 
int border_customize(texid_t texid, border_def_t def);

/** Get a predefined border definition.
 */
void border_get_def(border_def_t def, int n);

/**@}*/

#endif /* #define _BORDER_H_ */
