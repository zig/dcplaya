/* 2002/02/10 */

#ifndef _SONGMENU_H_
#define _SONGMENU_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


#define WIN68_DIR_X       42.0f
#define WIN68_PLAYLIST_X  345.0f
#define WIN68_Y           122.0f

int songmenu_init(void);
int songmenu_start(void);
int songmenu_kill(void);
void songmenu_render(int elapsed);
int songmenu_current_window(void);

DCPLAYA_EXTERN_C_END

#endif /* #ifndef _SONGMENU_H_ */
