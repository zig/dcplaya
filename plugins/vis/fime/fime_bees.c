#include <stdlib.h>
#include "config.h"
#include "math_float.h"

#include "fime_bees.h"

#include "driver_list.h"
#include "obj_driver.h"
#include "sysdebug.h"

#include "draw_object.h"
#include "draw/texture.h"
#include "draw/vertex.h"

static texid_t texid;

static vtx_t bo_vtx[4];
static tri_t bo_tri[5];
static tlk_t bo_tlk[4];
static vtx_t bo_nrm[4];

static obj_t bo_obj = {
  "bees",
  0,
  4,
  4,
  4,
  4,
  bo_vtx,
  bo_tri,
  bo_tlk,
  bo_nrm
};

static obj_t * cur_obj;
static obj_driver_t * bee_obj;
static fime_bee_t * bees;

static int init_bo_obj(void)
{
  const float s = 1, sx = s*0.35, sy = s*0.2;
  int i;

  for (i=0;i<4;++i) {
    if (!i) {
      bo_vtx[i].x = 0;
      bo_vtx[i].y = 0;
      bo_vtx[i].z = s * 0.5;
    } else {
      const float a = (float)(3-i)*2*MF_PI/3.0f;
      bo_vtx[i].x = -Sin(a) * sx;
      bo_vtx[i].y = Cos(a) * sy;
      bo_vtx[i].z = -s * 0.5;
    }
    bo_vtx[i].w = 1;
  }

  for (i=0; i<3; ++i) {
    bo_tri[i].a = 0;
    bo_tri[i].b = (i+0)%3u + 1;
    bo_tri[i].c = (i+1)%3u + 1;
  }

  bo_tri[3].a = 2;
  bo_tri[3].b = 1;
  bo_tri[3].c = 3;

  bo_tri[4].flags = 1;

  bo_tlk[0].a = 2;
  bo_tlk[0].b = 3;
  bo_tlk[0].c = 1;

  bo_tlk[1].a = 0;
  bo_tlk[1].b = 3;
  bo_tlk[1].c = 1;

  bo_tlk[2].a = 1;
  bo_tlk[2].b = 3;
  bo_tlk[2].c = 0;

  bo_tlk[3].a = 0;
  bo_tlk[3].b = 2;
  bo_tlk[3].c = 1;

  //$$$
  for (i=0; i<4; ++i) {
    bo_tlk[i].a = bo_tlk[i].b = bo_tlk[i].c = 4;
  }

  return obj3d_build_normals(&bo_obj);

}

static void bee_update(matrix_t leader_mtx, matrix_t mtx, fime_bee_t * bee)
{
  vtx_t leader_pos; 
  vtx_t tgt;
  vtx_t mov;
  float d;
  char * dump=0;

  bee->prev_pos = bee->pos;

  leader_pos = *(vtx_t *)leader_mtx[3];
  MtxVectMult(&tgt.x, &bee->rel_pos.x, leader_mtx);

#if 1
  vtx_sub3(&mov, &tgt, &bee->pos);
  d = vtx_norm(&mov);
  if (d < MF_EPSYLON) {
    printf("Heho : %f [%f %f %f]\n", d, mov.x, mov.y, mov.z);
    dump = "NO MOVE";
    bee->spd = 0;
    d = 0;
  } else {
    if (d <= bee->spd) {
      bee->spd = d;
      //      dump = "TOO FAST";
    } else {
      const float f = 0.2f;
      const float maxspd = 0.013f;
      bee->spd = d * f + bee->spd * (1.0-f);
      if (bee->spd > maxspd) {
	bee->spd = maxspd;
      }
      vtx_scale(&mov, bee->spd/d);
    }
    vtx_add(&bee->pos, &mov);
  }
#else
  vtx_blend(&bee->pos,&tgt, 0.9);
  
#endif
/*   if (!dump && bee == bees) dump = "Leader"; */

  if (dump) {
    printf("dump [%p,'%s']\n"
	   " lea:[ %f %f %f ]\n"
	   " tgt:[ %f %f %f ]\n"
	   " mov:[ %f %f %f ] := %f\n"
	   " dir:[ %f %f %f ]\n"
	   " pos:[ %f %f %f ] := %f\n"
	   " rel:[ %f %f %f ]\n",
	   bee,dump,
	   leader_pos.x, leader_pos.y, leader_pos.z,
	   tgt.x, tgt.y, tgt.z,
	   mov.x, mov.y, mov.z, d,
	   (leader_pos.x-bee->pos.x),(leader_pos.y-bee->pos.y),
	   (leader_pos.z-bee->pos.z),
	   bee->pos.x, bee->pos.y, bee->pos.z, bee->spd,
	   bee->rel_pos.x, bee->rel_pos.y, bee->rel_pos.z
	   );
	   
  }

  MtxLookAt(mtx,
	    (leader_pos.x - bee->pos.x),
	    (leader_pos.y - bee->pos.y),
	    (leader_pos.z - bee->pos.z));
  MtxTranspose3x3(mtx);
  MtxScale(mtx,0.2);
/*   MtxIdentity(mtx); */
  MtxTranslate(mtx, bee->pos.x, bee->pos.y, bee->pos.z);
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

static int r_bee_count(const fime_bee_t * bee)
{
  int cnt = 0;
  for ( ; bee; bee=bee->buddy) {
    cnt += r_bee_count(bee->soldier) + 1;
  }
  return cnt;
}

static fime_bee_t * create_bee(int n)
{
  const float rmin = 0.1, rmax = 0.5, sep=0.6;
  int i;
  int m = (n * (n+1)) >> 1;
  fime_bee_t *bees, *bee, *leader;
  int n_leader = 0;
  float z;

  SDDEBUG("[fime] : create_bee (%d) -> %d bees\n",n,m);

  bees = (fime_bee_t *) malloc(m * sizeof(*bees));
  if (!bees) {
    return 0;
  }
  memset(bees, 0, m * sizeof(*bees));

  leader = 0;
  n_leader = 0;
  for (i=0, z=0, bee=bees; i<n; ++i, z-=sep) {
    int j;
    float a,s=(2*MF_PI) / (float)(i+1);
    float f = (float)i/(float)n;
    float sx = rmin * (1.0-f) + rmax * f;
    float sy = sx;
    fime_bee_t *first_buddy = bee;
   
    for (j=0, a=0; j<=i; ++j, ++bee, a+=s) {
      if (bee - bees >= m) {
	SDERROR("[fime] : create_bee too many bees (INTERNAL)\n");
	free(bees);
	BREAKPOINT(0xdead0bee);
	return 0;
      }

      bee->pos.x = Cos(a) * sx;
      bee->pos.y = Sin(a) * sy;
      bee->pos.z = z;
      bee->pos.w = 1;

      bee->leader = find_nearest(bee, leader , n_leader);
      if (bee->leader) {
	fime_bee_t ** b = &bee->leader->soldier;
	for ( ; *b; b = &(*b)->buddy)
	  ;
	*b = bee;
      }
      MtxIdentity(bee->mtx);

    }
    leader = first_buddy;
    n_leader = bee-leader;
  }

  if (bee - bees != m) {
    SDERROR("[fime] : bad number of bees [%d != %d]\n",bee-bees,m);
    free(bees);
    BREAKPOINT(0xdead0bee);
    return 0;
  }

  /* Compute relative position. */
  bees->pos.x = bees->pos.y = bees->pos.z = 0;
  bees->rel_pos = bees->pos;
  for (i=1; i<m; ++i) {
    vtx_t tmp;
    bee=bees+i;
    tmp = bee->leader->pos;
/*     tmp.x = tmp.y = 0; */
    vtx_sub3(&bee->rel_pos, &bee->pos, &tmp);
  }

  /* Count to verify */
  {
    int cnt = r_bee_count(bees);
    if (cnt != m) {
      SDDEBUG("[fime] bee create count failed [%d != %d]\n",cnt,m);
      if (bees) {
	free(bees);
	bees = 0;
      }
    }
  }

  return bees;
}

int fime_bees_init(void)
{
  int err = 0;
  SDDEBUG("[fime] bees init...\n");

  bee_obj = 0;
  if (0) {
    bee_obj = (obj_driver_t *)driver_list_search(&obj_drivers,"spaceship1");
    err = !bee_obj;
    if (!err) {
      cur_obj = &bee_obj->obj;
    } 
  }
  if (!cur_obj) {
    if (!init_bo_obj()) {
      cur_obj = &bo_obj;
    }
  }

  bees = create_bee(6);
  texid = texture_get("fime_bordertile");
  err |= !bees || !cur_obj || (texid<0);

  SDDEBUG("[fime] bees init := [%s].\n", !err ? "OK" : "FAILED");
  
  return -(!!err);
  
}

void fime_bees_shutdown(void)
{
  driver_dereference(&bee_obj->common);
  bee_obj = 0;
  cur_obj = 0;

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
  vtx_t pos, pos_scale = {1.4, 1.0, 2.0, 1};

  if (!bees) {
    return -1;
  }

  MtxIdentity(tmp);
  MtxRotateX(tmp, ax += -0.021f);
  MtxRotateY(tmp, ay += 0.033f);
  vtx_mul3(&pos, (vtx_t *)tmp[2], &pos_scale);
  pos.z += 4;

  MtxIdentity(tmp);
  MtxScale(tmp,0.3f);
  *(vtx_t *)tmp[3] = pos;

  bee_update_tree(tmp, bees);

  return -(!!err);
}

static int bee_render(fime_bee_t * bee,
		      viewport_t *vp, matrix_t camera, matrix_t proj)
{
  int err;
  vtx_t ambient = { 0.1, 0.2, 0.3, 1 };
  vtx_t color = { 0.3, 1, 1, 0 };
  const int opaque = 1;

  if (!cur_obj) {
    return -1;
  }

  cur_obj->flags = 0
    | DRAW_NO_FILTER
    | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (texid << DRAW_TEXTURE_BIT);

  MtxMult(bee->mtx, camera);

/*   err =  DrawObjectSingleColor(vp, bee->mtx, proj, */
/* 			       cur_obj, &color); */
  err =  DrawObjectFrontLighted(vp, bee->mtx, proj,
			       cur_obj, &ambient, &color);

  return -!!err;
}

static int r_bee_render(fime_bee_t * bee,
			viewport_t *vp, matrix_t camera, matrix_t proj)
{
  int err = 0;

  for ( ; bee; bee=bee->buddy) {
    err |= bee_render(bee, vp, camera, proj);
    err |= r_bee_render(bee->soldier, vp, camera, proj);
  }
  return err;
}

int fime_bees_render(viewport_t *vp,
		     matrix_t camera, matrix_t proj)
{
  int err;

  err = r_bee_render(bees, vp, camera, proj);
  return -(!!err);
}

