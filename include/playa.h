/**
 * @file     playa.h
 * @author   benjamin gerard <ben@sashipa.com>
 * @brief    music player threads
 *
 * $Id: playa.h,v 1.6 2002-10-10 06:07:39 benjihan Exp $
 */

#ifndef _PLAYA_H_
#define _PLAYA_H_

#include "extern_def.h"

DCPLAYA_EXTERN_C_START


/** @name  Main decoder thread status
 *  @{
 */
#define PLAYA_STATUS_INIT     0
#define PLAYA_STATUS_READY    1
#define PLAYA_STATUS_STARTING 2
#define PLAYA_STATUS_PLAYING  3
#define PLAYA_STATUS_STOPPING 4
#define PLAYA_STATUS_QUIT     5
#define PLAYA_STATUS_ZOMBIE   6
#define PLAYA_STATUS_REINIT   7
/*@}*/

#include "playa_info.h"

int playa_init();
int playa_shutdown();

int playa_isplaying();

int playa_start(const char *fn, int track, int immediat);
int playa_stop(int flush);
int playa_ispaused(void);
int playa_pause(int v);
int playa_fade(int ms);

int playa_volume(int volume);

int playa_status();
const char * playa_statusstr(int status);

int playa_info(playa_info_t * info, const char *fn);

unsigned int playa_playtime();
void playa_get_buffer(int **b, int *nbSamples, int *counter, int *frq);

DCPLAYA_EXTERN_C_END

#endif
