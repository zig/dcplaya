/*  #include <kos.h> //$$$ for debug */
#include "vupeek.h"
#include "math_int.h"
#include "sysdebug.h"

static int last_id;

vupeek_t
  peek1 = {
    256, 0xe000, 0x8000, 0xf000 
    
  },

  peek2 = {
    2048, 0xc000, 0x8000, 0xe000
  },

  peek3 = {
    1024, 0xffff, 0xf000, 0xff00
  };



static unsigned int smooth(unsigned int a, unsigned int b, unsigned int f)
{
  return ((a * f) + (b * (0x10000-f))) >> 16;
}

static void do_group(vupeek_t *p)
{
  unsigned int val;
  
  if (p->cnt < p->grp_sca) {
    return;
  }
/*    if (p->cnt != p->grp) { */
/*      *(int*)0x1 = 0x12345678; */
/*    } */
  
  val = p->acu / p->cnt;
  
  if (val > 65535) {
    SDDEBUG("val = %d\n",val);
    BREAKPOINT(0xFAFADADA);
  }
  val = int_sqrt(val<<16);
  
  
  if (val > p->max) p->max = val;
  p->avg += val;
  p->avg_cnt++;
  
  p->smo = smooth(p->smo, val, p->smo_fac);
  p->dyn = smooth(p->dyn, val, (val > p->dyn) ?  p->dup_fac :  p->ddw_fac);
  p->val = val;
  
//  p->dyn_max;
  
  p->acu = 0;
  p->cnt = 0;
  
}

void vupeek_adddata(int *spl, int n, int id, int frq)
{
  /* Do not process the same input buffer ... */
  if (id == last_id) {
    return;
  }
  last_id = id;

  {
    int f;

    f = (frq>0) ? ((frq<<14) / 44100) : (1<<14);
/*      dbglog(DBG_DEBUG,"%f(%d) ", f, frq); */
    peek1.grp_sca = (f * peek1.grp) >> 14;
    peek2.grp_sca = (f * peek2.grp) >> 14;
    peek3.grp_sca = (f * peek3.grp) >> 14;
  }

  while (n) {
    int r,r2;
    unsigned int p;
    
    r  = n;
    r2 = peek1.grp_sca - peek1.cnt;
    if (r2 < r) r = r2;
    r2 = peek2.grp_sca - peek2.cnt;
    if (r2 < r) r = r2;
    r2 = peek3.grp_sca - peek3.cnt;
    if (r2 < r) r = r2;
    n -= r;
    
    if (r > 0) {
      peek1.cnt += r;
      peek2.cnt += r;
      peek3.cnt += r;
      
      p = 0;
      do {
	int h,l,s;
	
	h = *spl++;
	l = (signed short)h;
      s = l >> 31;        /* 0 -1 */
      p += (l ^ s) - s;  
      l = h >> 16;
      s = l >> 31;        /* 0 -1 */
      p += (l ^ s) - s;   /* 0-65535 */
      } while (--r);

      peek1.acu += p;
      peek2.acu += p;
      peek3.acu += p;
    }
    
    do_group(&peek1);
    do_group(&peek2);
    do_group(&peek3);
        
  }
}

