/* 2002/02/19 */

#ifndef _INFO_H_
#define _INFO_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


void info_render(uint32 elapsed_frames, int is_playing);
int info_setup(void);
int info_is_help(void);

DCPLAYA_EXTERN_C_END

#endif /* #define _INFO_H_ */
