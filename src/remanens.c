/* 2002/02/21 */

#include "config.h"
#include "remanens.h"

#define REMANENS_MAX 128

static remanens_t remanens[REMANENS_MAX];
static int idx, nb;

void remanens_clean(void)
{
  int i;
  
  idx = nb = 0;
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


static remanens_t * remanens_alloc(void)
{
  if (nb >= REMANENS_MAX) {
    return 0;
  } else {
    return  remanens + ((idx + nb++) & (REMANENS_MAX-1));
  }
}

static remanens_t * remanens_alloc_always(void)
{
  if (nb >= REMANENS_MAX) {
    int i = idx;
    idx = (idx+1) & (REMANENS_MAX-1);
    return remanens + i;
  } else {
    return  remanens + ((idx + nb++) & (REMANENS_MAX-1));
  }
}

static void remanens_free(int n)
{
  if ((unsigned int)n > (unsigned int)nb) n = nb;
  nb -= n;
  idx = (idx+n) & (REMANENS_MAX-1);
}

void remanens_push(obj_t *o, matrix_t mtx, unsigned int frame)
{
  remanens_t *r;
  r = remanens_alloc_always();
  r->o      = o;
  MtxCopy(r->mtx, mtx);
  r->frame  = frame;
}

void remanens_remove_old(unsigned int threshold_frame)
{
  int i;
  for (i=0; i<nb && remanens[(idx+i)&(REMANENS_MAX-1)].frame <= threshold_frame; ++i);
  nb -= i;
  idx = (idx+i) & (REMANENS_MAX-1);
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
  return & remanens[ (idx+id) & (REMANENS_MAX-1) ];
}
