#ifndef _VUPEEK_H_
#define _VUPEEK_H_

typedef struct
{
  unsigned int grp;       /* size of group for 44100 hz */
  unsigned int smo_fac;   /* smooth factor [0..0x10000] */
  unsigned int dup_fac;   /* dynamic increase factor [0..0x10000] */
  unsigned int ddw_fac;   /* dynamic reduce factor [0..0x10000] */
  
  unsigned int grp_sca;   /* size of group scaled to match replay freq */
  
  unsigned int acu;   /* 1st half acu */
  unsigned int cnt;   /* number of value in acu */
  
  unsigned int val;   /* last value calculated */
  unsigned int smo;   /* smoothed value */
  unsigned int dyn;   /* dynamic value */
  
  
  unsigned int max;
  unsigned int avg_cnt;
  unsigned int avg;
  
} vupeek_t;

extern vupeek_t peek1, peek2, peek3;
void vupeek_adddata(int *spl, int n, int id, int frq);

#endif
