/** @ingroup dcplaya_vis_driver
 *  @file    fime_bees.c
 *  @author  benjamin gerard 
 *  @date    2003/01/17
 *  @brief   FIME : bees 
 *  $Id: fime_bees.c,v 1.7 2003-01-25 11:37:44 ben Exp $
 */ 

#include <stdlib.h>
#include "config.h"
#include "math_float.h"

#include "fime_bees.h"
#include "fime_analysis.h"

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

static matrix_t target_mtx;
static vtx_t target_pos;
static vtx_t target_pos_scale;
static vtx_t target_angle;
static vtx_t target_inc;
static float target_inc_spd;
static float target_inc_spd_a;

static void update_target(matrix_t mtx)
{
  vtx_t pos;

  MtxIdentity(mtx);
  MtxRotateX(mtx, target_angle.x);
  MtxRotateY(mtx, target_angle.y);
  vtx_mul3(&pos, (vtx_t *)mtx[2], &target_pos_scale);
  MtxIdentity(mtx);
  vtx_add3((vtx_t *)mtx[3], &pos, &target_pos);
}

static void reset_target(matrix_t mtx,
			 const float x, const float y, const float z)
{
  target_inc_spd_a = 0;
  target_inc_spd = 1;
  vtx_set(&target_pos, x, y, z);
  vtx_set(&target_pos_scale, 0.7, 0.7*0.75, 1);
  vtx_identity(&target_angle);
  vtx_set(&target_inc, -0.021f, 0.033f, 0);

  update_target(mtx);
}


static void move_target(matrix_t mtx)
{
  vtx_t inc;

  target_inc_spd_a = __vtx_inc_angle(target_inc_spd_a, 0.0023);
  target_inc_spd = Cos(target_inc_spd_a) + 0.5;

  vtx_scale3(&inc, &target_inc, target_inc_spd);
  vtx_inc_angle(&target_angle, &inc);

  update_target(mtx);
}

//static matrix_t beat_matrix;
static float beatf;

static void bee_matrix(fime_bee_t * bee, matrix_t mtx, const vtx_t * axe)
{
  MtxLookAt(mtx, axe->x, axe->y, axe->z);
  //MtxInvMult(mtx,beat_matrix);
  MtxTranspose3x3(mtx);
  MtxScale(mtx,bee->scale*(1.0+beatf*2));
  MtxTranslate(mtx, bee->pos.x, bee->pos.y, bee->pos.z);
}

static void bee_update(matrix_t leader_mtx, matrix_t mtx, fime_bee_t * bee)
{
  vtx_t leader_pos, tgt, mov, axe;
  float d;

  bee->prev_pos = bee->pos;

  /* Get leader position */
  leader_pos = *(vtx_t *)leader_mtx[3];

  
  MtxVectMult(&tgt.x, &bee->rel_pos.x, leader_mtx);

  /* Compute move axis. */
  vtx_sub3(&axe, &tgt, &bee->pos);

  /* Get distance to target */
  d = vtx_norm(&axe);

  if (d < MF_EPSYLON) {
    /* Target is very near, move to target */
    bee->pos = tgt;
    bee->spd = 0;
    d = 0;
    /* Do not update matrix except for translation. */
    //    *(vtx_t *)mtx[3] = tgt;
  } else {
    const float id = Inv(d);
    float a;
    const float f = 0.2f;
    vtx_t new_axe;
    const float spd_max = 0.014;

    vtx_scale3(&mov, &axe, id);
    a = vtx_dot_product(&mov,&bee->axe);
    /* Should determine acceleration here */
    bee->spd = d * f + bee->spd * (1.0-f);

    if (bee->spd > spd_max) bee->spd = spd_max;

    vtx_blend3(&new_axe, &axe, &bee->axe, 0.5);

    d = vtx_inorm(&new_axe);
    if (d < 0) {
      vtx_scale3(&bee->axe, &axe, id);
    } else {
      vtx_scale3(&bee->axe, &new_axe, d);
    }
    vtx_scale3(&mov, &bee->axe, bee->spd);
    vtx_add(&bee->pos, &mov);
    bee_matrix(bee, mtx, &bee->axe);
  }
}

static void r_bee_update(matrix_t leader_mtx, fime_bee_t * bee)
{
  for ( ; bee; bee=bee->buddy) {
    bee_update(leader_mtx, bee->mtx, bee);
    r_bee_update(bee->mtx, bee->soldier);
  }
}

static void dump_bee(const fime_bee_t * bee, int level) {
  if (bee) {
    int i;
    for (i=0;i<level;++i) printf(" ");
    printf("[bee:%p lea:%p bud:%p sol:%p (%.02f %.02f %.02f)]\n",
	   bee, bee->leader, bee->buddy, bee->soldier,
	   bee->rel_pos.x,bee->rel_pos.y,bee->rel_pos.z);
  } 
}

static void bee_dump(fime_bee_t * bee, int level)
{
  fime_bee_t * b = bee;
  if (!b) return;

  for ( ; bee; bee=bee->buddy) {
    dump_bee(bee, level);
  }

  for (bee=b ; bee; bee=bee->buddy) {
    bee_dump(bee->soldier, level+1);
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
  const float rmin = 0.1, rmax = 0.5, sep=1.1;
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

      bee->scale = 0.1;
      if (bee == bees) {
	/* Leader */
	bee->pos.x = 0;
	bee->pos.y = 0;
      } else {
	bee->pos.x = Cos(a) * sx;
	bee->pos.y = Sin(a) * sy;
      }
      bee->pos.z = z;
      bee->pos.w = 1;

      bee->leader = find_nearest(bee, leader, n_leader);
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
  for (i=0; i<m; ++i) {
    fime_bee_t *leader;
    vtx_t tmp;
    bee=bees+i;
    leader = bee->leader;
    if (!leader) {
      vtx_identity(&tmp);
    } else {
      tmp = leader->pos;
    }
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

static void bee_reset_position(fime_bee_t *bee, matrix_t leader_mtx)
{
  vtx_t leader_pos; 

  if (!bee) {
    return;
  }

  leader_pos = *(vtx_t *)leader_mtx[3];
  MtxVectMult(&bee->pos.x, &bee->rel_pos.x, leader_mtx);
  bee_matrix(bee, bee->mtx, (vtx_t *)leader_mtx[2]);

  /* Compute fake previous position. */
  vtx_sub3(&bee->prev_pos, &bee->pos, (vtx_t *)bee->mtx[2]);

  /* Clean */
  bee->spd = 0;
}

static void fime_bee_reset_position(fime_bee_t *bee, matrix_t leader_mtx)
{
  for ( ; bee; bee=bee->buddy) {
    bee_reset_position(bee, leader_mtx);
    fime_bee_reset_position(bee->soldier, bee->mtx);
  }
}


int fime_bees_update(void)
{
  int err = 0;
  if (!bees) {
    return -1;
  }

  /* Compute target position. */
  {
    const fime_analyser_t * a = fime_analysis_get(1);
    float b = (a->v * 0.00 + a->w * 1.0);
    float f = b > beatf ? 0.3 : 0.90;
    beatf =  beatf * f + b * (1.0-f);
  }

  move_target(target_mtx);
  bee_update_tree(target_mtx, bees);

  return -(!!err);
}

const char * cflagsstr(int f)
{
  int i;
  static char t[7], t2[] = "ZzYyXx";
  for (i=0; i<6; ++i) {
    t[i] = (f&(1<<i)) ? t2[i] : '.';
  }
  return t;
}

static int render_it(viewport_t *vp, matrix_t mtx, matrix_t proj,
		     const vtx_t * ambient, const vtx_t * diffuse)
{
  const int opaque = 1;
  int flags;
  cur_obj->flags = 0
    | DRAW_NO_FILTER
    | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (texid << DRAW_TEXTURE_BIT);

  flags = DrawObject(vp, mtx, proj, cur_obj, ambient, diffuse, 0);
/*   if (flags>0) { */
/*     printf("[%s ", cflagsstr(flags>>6)); */
/*     printf("%s]", cflagsstr(flags)); */
/*   } */
  return -(flags<0);
/*     return DrawObjectFrontLighted(vp, mtx, proj, */
/* 				  cur_obj, */
/* 				  ambient, diffuse); */
}

static int bee_render(fime_bee_t * bee,
		      viewport_t *vp, matrix_t camera, matrix_t proj)
{
  int err;
  vtx_t ambient = { 0.1, 0.2, 0.3, 1 };
  vtx_t color = { 0.3, 1, 1, 0 };
  matrix_t mtx;

  MtxMult3(mtx, bee->mtx, camera);

  vtx_scale(&color, beatf * 0.5 + 0.5);

  err = render_it(vp, mtx, proj, &ambient, &color);

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

  if (!cur_obj) {
    return -1;
  }

  if (1) {
    /* render the target */
    static const vtx_t ambient = { 0.2,0.3,0,1 };
    static const vtx_t color = { 1,1,0,1 };
    matrix_t mtx;

    MtxIdentity(mtx);
    MtxRotateX(mtx, target_angle.x);
    MtxRotateY(mtx, target_angle.y);
    MtxMult(mtx, target_mtx);
    MtxScale3x3(mtx, 0.2);
    MtxMult(mtx, camera);

    render_it(vp, mtx, proj,
	      &ambient, &color);
  }
  
  err = r_bee_render(bees, vp, camera, proj);
  return -(!!err);
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

  bees = create_bee(4);
  bee_dump(bees,0);
  {
    reset_target(target_mtx, 0, 0, 3);
    fime_bee_reset_position(bees, target_mtx);
  }


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
