#include <stdlib.h>
#include <dc/fmath.h>

#include "fime_bees.h"

#include "driver_list.h"
#include "obj_driver.h"
#include "sysdebug.h"

static void bee_update(matrix_t leader_mtx, matrix_t mtx, fime_bee_t * bee)
{
  bee->prev_pos = bee->pos;

  MtxIdentity(mtx);
  mtx[3][0] = bee->rel_pos.x;
  mtx[3][1] = bee->rel_pos.y;
  mtx[3][2] = bee->rel_pos.z;

  MtxMult(mtx, leader_mtx);

  /* Move */
  vtx_blend(&bee->pos, (const vtx_t *)mtx[3], 0.80);

}

static void r_bee_update(matrix_t leader_mtx, fime_bee_t * bee)
{
  for ( ; bee; bee=bee->buddy) {
    bee_update(leader_mtx, bee->mtx, bee);
    r_bee_update(bee->mtx, bee->soldier);
  }
}

static void bee_update_tree(matrix_t mtx, fime_bee_t * bee)
{
  r_bee_update(mtx, bee);
}    

static fime_bee_t * find_nearest(fime_bee_t *bee, fime_bee_t *bees, int n)
{
  int i,best;
  float dbest;

  if (!bee || !bees || !n) {
    return 0;
  }

  dbest = vtx_sqdist(&bee->pos,&bees->pos);
  best = 0;

  for (i=1; i<n; ++i) {
    float d = vtx_sqdist(&bee->pos,&bees[i].pos);
    if (d < dbest) {
      dbest = d;
      best = i;
    }
  }
  return bees + best;
}

static fime_bee_t * create_bee(int n)
{
  const float rmin = 0.1, rmax = 0.5, sep=0.3;
  int i;
  int m = (n * (n+1)) >> 1;
  fime_bee_t *bees, *bee, *leader;
  int n_leader = 0;
  float z;

  bees = (fime_bee_t *) malloc(m * sizeof(*bees));
  if (!bees) {
    return 0;
  }
  memset(bees, 0, m * sizeof(*bees));
 

  leader = 0;
  n_leader = 0;
  for (i=0, z=0, bee=bees; i<n; ++i, z+=sep) {
    int j;
    float a,s=(2*3.14159) / (i+1);
    float f = (float)i/(float)n;
    float sx = rmin * (1.0-f) + rmax * f;
    float sy = sx;
    fime_bee_t *first_buddy = bee;
   
    for (j=0, a=0; j<=i; ++j, ++bee, a+=s) {
      bee->pos.x = fcos(a) * sx;
      bee->pos.y = fsin(a) * sy;
      bee->pos.z = z;
      bee->pos.w = 1;

      bee->leader = find_nearest(bee, leader , n_leader);
      if (bee->leader) {
	fime_bee_t ** b = &bee->leader->soldier;
	for ( ; *b; b = &(*b)->buddy)
	  ;
	*b = bee;
      }

    }
    leader = first_buddy;
    n_leader = bee-leader;
  }

  /* Compute relative position. */
  bees->rel_pos = bees->pos;
  for (i=1; i<m; ++i) {
    bee=bees+i;
    vtx_sub3(&bee->rel_pos, &bee->pos, &bee->leader->pos);
  }

  return bees;
}

static fime_bee_t * bees;
static obj_driver_t * bee_obj;

int fime_bees_init(void)
{
  int err = 0;

  bee_obj = (obj_driver_t *)driver_list_search(&obj_drivers,"spaceship1");
  bees = create_bee(5);
  err = !bees || !bee_obj;

  SDDEBUG("[fime] bees init := [%s].\n", !err ? "OK" : "FAILED");
  
  return -(!!err);
  
}

void fime_bees_shutdown(void)
{
  driver_dereference(&bee_obj->common);
  bee_obj = 0;

  if (bees) {
    free(bees);
    bees = 0;
  }
  SDDEBUG("[fime] bees shutdowned.\n");
}

int fime_bees_update(void)
{
  int err = 0;
  static float ax;
  static float ay;
  matrix_t tmp;
  float z = 5;

  if (!bees) {
    return -1;
  }

  MtxIdentity(tmp);
  MtxRotateX(tmp, ax += -0.021f);
  MtxRotateY(tmp, ay += 0.033f);

  MtxIdentity(bees->mtx);

  bees->mtx[3][0] = tmp[0][0] * 1.4;
  bees->mtx[3][1] = tmp[1][0] * 1.0;
  bees->mtx[3][2] = z;// + 0.1;

  return -(!!err);
}

int fime_bees_render(viewport_t *vp,
		     matrix_t camera, matrix_t proj)
{
  int err = 0;
  if (!bees) {
    return -1;
  }

  return -(!!err);

}

