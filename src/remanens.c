/**
 * @file    remanens.c
 * @author  bejamin gerard <ben@sashipa.com>
 * @date    2002/02/21
 * @brief   Remanence FX.
 *
 * @version $Id: remanens.c,v 1.2 2002-09-13 14:48:25 ben Exp $
 */

#include "config.h"
#include "remanens.h"

#define REMANENS_MAX 128

static remanens_t remanens[REMANENS_MAX];
static int idx, nb;

void remanens_clean(void)
{
  int i;
  
  idx = REMANENS_MAX - 1;
  nb = 0;
  for (i=0; i<REMANENS_MAX; ++i) {
    MtxIdentity(remanens[i].mtx);
    remanens[i].frame = 0;
    remanens[i].o = 0;
  }
}

int remanens_setup(void)
{
  remanens_clean();
  return 0;
}

/* static remanens_t * remanens_alloc(void) */
/* { */
/*   if (nb >= REMANENS_MAX) { */
/*     return 0; */
/*   } else { */
/*     return  remanens + ((idx + nb++) & (REMANENS_MAX-1)); */
/*   } */
/* } */

static remanens_t * remanens_alloc_always(void)
{
  idx = (idx+1) & (REMANENS_MAX-1);
  nb += (nb < REMANENS_MAX);

  return  remanens + idx;
}

static void remanens_free(int n)
{
  nb -= n;
  nb &= ~(nb >> (sizeof(int) * 8 - 1));
}

void remanens_push(obj_t *o, matrix_t mtx, unsigned int frame)
{
  remanens_t *r;

  r = remanens_alloc_always();
  r->o = o;
  MtxCopy(r->mtx, mtx);
  r->frame = frame;
}

void remanens_remove_old(unsigned int threshold_frame)
{
  int i;
  for (i=0; i<nb &&
	 remanens[(idx+REMANENS_MAX-i)&(REMANENS_MAX-1)].frame
	 >= threshold_frame;
       ++i)
    ;
  nb -= i;
}

int remanens_nb()
{
  return nb;
}

remanens_t *remanens_get(int id)
{
  if ((unsigned int)id >= (unsigned int)nb) {
    return 0;
  }
  return & remanens[ (idx+REMANENS_MAX-id) & (REMANENS_MAX-1) ];
}
