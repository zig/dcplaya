/** @ingroup dcplaya_fime_vis_plugin_devel
 *  @file    fime_analysis.h
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME Frequency and beat analysis.
 *
 *  $Id: fime_analysis.h,v 1.4 2003-03-19 05:16:16 ben Exp $
 */ 

#ifndef _FIME_ANALYSIS_H_
#define _FIME_ANALYSIS_H_

typedef struct {
  float v,w;
  float avg,avg2;
  float max, max2;
  float thre;
  float ts;
  int tacu;
} fime_analyser_t;

extern int fime_analysis_result;

int fime_analysis_init(void);
void fime_analysis_shutdown(void);
int fime_analysis_update(void);
const fime_analyser_t * fime_analysis_get(int i);

#endif /* #ifndef _FIME_ANALYSIS_H_ */
