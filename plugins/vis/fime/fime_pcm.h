/** @ingroup dcplaya_fime_vis_plugin_devel
 *  @file    fime_pcm.h
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME pcm
 *
 *  $Id: fime_pcm.h,v 1.2 2003-03-19 05:16:16 ben Exp $
 */ 

#ifndef _FIME_PCM_H_
#define _FIME_PCM_H_

#define FIME_PCM_SIZE 64

int fime_pcm_init(void);
void fime_pcm_shutdown(void);
float * fime_pcm_update(void);
float * fime_pcm_get(void);

#endif /* #ifndef _FIME_PCM_H_ */
