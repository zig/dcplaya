/** @ingroup dcplaya_vis_driver
 *  @file    fime_star.c
 *  @author  benjamin gerard 
 *  @date    2003/02/04
 *  @brief   FIME star field 
 *  $Id: fime_star.c,v 1.2 2003-03-10 22:55:34 ben Exp $
 */ 

#include <stdlib.h>

#include "dcplaya/config.h"
#include "fime_star.h"
#include "fime_analysis.h"
#include "obj3d.h"
#include "driver_list.h"
#include "obj_driver.h"
#include "draw/texture.h"
#include "draw/vertex.h"
#include "draw_object.h"
#include "sysdebug.h"

extern int rand();

static texid_t texid;

static float rand_float(const float m, const float M)
{
  const float s = 1.0f / 65535.0f;
  return (float)(rand()&0xFFFF) * s * (M-m) + m;
}

static void random_star(fime_star_t * star,
			float xs, float xt,
			float ys, float yt,
			float zs, float zt)
{
  star->pos.x = (float)(rand()&0xFFFF) * xs + xt;
  star->pos.y = (float)(rand()&0xFFFF) * ys + yt;
  star->pos.z = (float)(rand()&0xFFFF) * zs + zt;
}

static void init_generator(fime_star_field_t *sf, fime_star_t *s)
{
  fime_star_box_t * box = &sf->box;

  random_star(s,
	      (box->xmax - box->xmin) / 65535.0f, box->xmin,
	      (box->ymax - box->ymin) / 65535.0f, box->ymin,
	      (box->zmax - box->zmin) / 65535.0f, box->zmin);
  s->trans = &sf->trans[rand() % (unsigned int)sf->nb_trans];
  s->obj   = &sf->forms[rand() % (unsigned int)sf->nb_forms]->obj;
}

static void std_generator(fime_star_field_t *sf, fime_star_t *s)
{
  fime_star_box_t * box = &sf->box;

  random_star(s,
	      (box->xmax - box->xmin) / 65535.0f, box->xmin,
	      (box->ymax - box->ymin) / 65535.0f, box->ymin,
	      0, box->zmax + (s->pos.z-box->zmin));
  s->trans = &sf->trans[rand() % (unsigned int)sf->nb_trans];
  s->obj   = &sf->forms[rand() % (unsigned int)sf->nb_forms]->obj;
}


static fime_star_field_t * create_star_field(int nb_stars,
					     const char ** forms,
					     int nb_trans)
{
  const char ** n;
  fime_star_field_t * sf = 0;
  int size;
  int nb_forms, i;

  if (nb_stars <= 0 || nb_trans <=0) {
    goto error;
  }

  /* Count forms. */
  for (n=forms, nb_forms = 0; *n; ++n, ++nb_forms)
    ;
  if (!nb_forms) {
    goto error;
  }

  /* Compute needed bytes */
  size = 0
    + sizeof(*sf)
    + sizeof(*sf->stars) * nb_stars
    + sizeof(*sf->trans) * nb_trans
    + sizeof(*sf->forms) * nb_forms;

  /* Allocation / clean */
  sf = malloc(size);
  if (!sf) {
    return 0;
  }
  memset(sf,0,size);

  /* Set struct. */
  sf->stars = (fime_star_t *)(sf+1);
  sf->forms = (obj_driver_t **)(sf->stars+nb_stars);
  sf->trans = (fime_star_trans_t *)(sf->forms+nb_forms);
  sf->nb_stars = nb_stars;
  sf->nb_forms = nb_forms;
  sf->nb_trans = nb_trans;

  /* Create forms */
  for (n=forms, nb_forms = 0; *n; ++n, ++nb_forms) {
    sf->forms[nb_forms] = (obj_driver_t *)driver_list_search(&obj_drivers, *n);
    if (!sf->forms[nb_forms]) {
      sf->nb_forms = nb_forms;
      SDERROR("[fime] : create star field, missing object [%s]\n", *n);
      for (--nb_forms; nb_forms>=0; --nb_forms) {
	driver_dereference(&sf->forms[nb_forms]->common);
      }
      free(sf);
      return 0;
    }
  }
  
  /* Setup transform */
  for (i=0; i<nb_trans; ++i) {
    const float scale=0.3;
    MtxIdentity(sf->trans[i].mtx);
    vtx_set(&sf->trans[i].scale,scale,scale,scale);
    sf->trans[i].cur_scale = sf->trans[i].scale;
    sf->trans[i].goal_scale = sf->trans[i].scale;
    vtx_set(&sf->trans[i].inc_angle,
	    rand_float(1,5),
	    rand_float(1,5),
	    rand_float(1,5));
    sf->trans[i].z_mov = 15;
  }
  
  return sf;
  
 error:
  fime_star_shutdown(sf);
  return 0;
}

fime_star_field_t * fime_star_init(int nb_stars,
				   const char ** forms,
				   const fime_star_box_t * box,
				   void * cookie)
{
  fime_star_field_t * sf;
  
  SDDEBUG("[fime] : star init\n");

  texid = texture_get("fime_bordertile");
  if (texid < 0) {
    return 0;
  }

  sf = create_star_field(nb_stars, forms, 4);
  if (sf) {
    int i;

    sf->generator = std_generator;
    sf->box = *box;
    sf->cookie = cookie;

    for (i=0; i<sf->nb_stars; ++i) {
      init_generator(sf,sf->stars+i);
    }

    SDDEBUG("[fime] : star := [%p, %d stars, %d forms, %d trans]\n",
	    sf, sf->nb_stars, sf->nb_forms, sf->nb_trans);
  } else {
    SDERROR("[fime] : star := [FAILED]\n");
  } 

  return sf;
}

void fime_star_shutdown(fime_star_field_t * sf)
{
  SDDEBUG("[fime] : starfield [%p] shutdown\n", sf);
  if (sf) {
    int i;
    for (i=0; i<sf->nb_forms; ++i) {
      driver_dereference(&sf->forms[i]->common);
    }
    free(sf);
  }
}

int fime_star_update(fime_star_field_t * sf, const float seconds)
{
  int i;
  fime_star_t *s,*se;
  const int analysis = fime_analysis_result;

  if (!sf) {
    return -1;
  }

  /* Update transform */
  for (i=0; i<sf->nb_trans; ++i) {
    vtx_t inc_angle, *s;

    sf->trans[i].cur_z_mov = sf->trans[i].z_mov * seconds;
    vtx_scale3(&inc_angle, &sf->trans[i].inc_angle, seconds);
    vtx_inc_angle(&sf->trans[i].angle,&inc_angle);
    MtxIdentity(sf->trans[i].mtx);
    if (analysis & 1<<(i&1)) {
      vtx_scale3(&sf->trans[i].goal_scale, &sf->trans[i].scale, 2);
    }
    s = &sf->trans[i].cur_scale;
    if (vtx_sqdist(s,&sf->trans[i].goal_scale) < MF_EPSYLON) {
      *s = sf->trans[i].goal_scale;
      sf->trans[i].goal_scale = sf->trans[i].scale;
    } else {
      vtx_blend(s,&sf->trans[i].goal_scale,0.6);
    }

    MtxScale3(sf->trans[i].mtx, s->x, s->y, s->z);
    MtxRotateX(sf->trans[i].mtx, sf->trans[i].angle.x);
    MtxRotateY(sf->trans[i].mtx, sf->trans[i].angle.y);
    MtxRotateZ(sf->trans[i].mtx, sf->trans[i].angle.z);
  }

  /* Move stars */
  {
    const float zmin = sf->box.zmin;
    for (s=sf->stars, se=s+sf->nb_stars; s<se; ++s) {
      if (!s->trans) {
	continue;
      }
      if ((s->pos.z -= s->trans->cur_z_mov) < zmin) {
	sf->generator(sf, s);
      }
    }
  }

  return 0;
}

int fime_star_render(fime_star_field_t * sf,
		     viewport_t *vp,
		     matrix_t camera, matrix_t proj,
		     vtx_t * ambient,
		     vtx_t * diffuse,
		     vtx_t * light,
		     int opaque)
{
  int flags,i;
  fime_star_t *s,*se;

  if (!sf) {
    return -1;
  }

  flags = 0
    | DRAW_NO_FILTER
    | (opaque ? DRAW_OPAQUE : DRAW_TRANSLUCENT)
    | (texid << DRAW_TEXTURE_BIT);

  for (i=0; i<sf->nb_forms; ++i) {
    sf->forms[i]->obj.flags = flags;
  }

  for (s=sf->stars, se=s+sf->nb_stars; s<se; ++s) {
    matrix_t mtx;
    MtxCopy(mtx,s->trans->mtx);
    MtxTranslate(mtx,s->pos.x,s->pos.y,s->pos.z);
    MtxMult(mtx,camera);
    DrawObject(vp, mtx, proj, s->obj, ambient, diffuse, light);
  }

  return 0;
}
