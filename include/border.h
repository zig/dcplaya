/**
 * @ingroup dcplaya_devel
 * @file    border.h
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/02/16
 * @brief   border rendering
 */

#ifndef _BORDER_H_
#define _BORDER_H_

#include "draw/texture.h"
#include "draw/color.h"

/* typedef struct { */
/*   struct { */
/*     float u,v; */
/*   } uv[4]; */
/* } borderuv_t; */

/** border definition. */
typedef draw_color_t border_def_t[3]; 

/** Standard border tile texture-id, mapped on "bordertile2". */
extern texid_t bordertex;

/** Init border texture. */
int border_init(void);

/** Customize a given border texture. */ 
int border_customize(texid_t texid, border_def_t def);

/** Get a predefined border definition. */
void border_get_def(border_def_t def, int n);

#endif /* #define _BORDER_H_ */
