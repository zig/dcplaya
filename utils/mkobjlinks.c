/* 2002/02/16 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tmpobj.inc"

static void PrintLinks(tlk_t *l, int n);

#define ABS(a) ((a)<0 ? -(a) : (a))

static volatile int indent = 0;
void Error(const char *fmt, ...)
{
  int i;
  va_list list;
  va_start(list, fmt);
  for (i=0; i<indent; ++i) {
    fprintf(stderr, " ");
  }
  vfprintf(stderr,fmt,list);
  va_end(list);
}

static volatile int missed = 0;       /* Missed when create */
static volatile int verif_missed = 0; /* Missed when verify */

static void CrossProduct(float *d, const float *v, const float * w)
{
  d[0] = v[1] * w[2] - v[2] * w[1];
  d[1] = v[2] * w[0] - v[0] * w[2];
  d[2] = v[0] * w[1] - v[1] * w[0];
}

static void FaceNormal(float *d, const vtx_t * v, const tri_t *t)
{
  const vtx_t *a=v+t->a,*b=v+t->b,*c=v+t->c;
  float n;
  vtx_t A,B;

  A.x = a->x - b->x;
  A.y = a->y - b->y;
  A.z = a->z - b->z;

  B.x = a->x - c->x;
  B.y = a->y - c->y;
  B.z = a->z - c->z;

  CrossProduct(d,&A.x,&B.x);
  n = 1.0f / (float)sqrt((double)(d[0]*d[0] + d[1]*d[1] + d[2]*d[2]));
  d[0] *= n;
  d[1] *= n;
  d[2] *= n;
}

static void BuildNormals(obj_t *o)
{
  int i;

  if (!o->nvx) {
    o ->nvx = (vtx_t *)malloc(sizeof(vtx_t) * o->nbf);
  }
  if (!o->nvx) {
    return;
  }
  for (i=0; i<o->nbf; ++i) {
    FaceNormal((float *)(o->nvx+i),o->vtx,o->tri+i);
  }
}

static float FaceCosAngle(obj_t *o, int ia, int ib)
{
  vtx_t *va = o->nvx + ia;
  vtx_t *vb = o->nvx + ib;
  
  return va->x * vb->x + va->y * vb->y + va->z * vb->z;
}

static int optimize_vtx(vtx_t *v, int n)
{
  int i,j;
  int cnt;
  for (i=0; i<n; ++i) {
    for (j=i+1; j<n; ++j) {
      float d = ABS(v[i].x-v[j].x) + ABS(v[i].y-v[j].y) + ABS(v[i].z-v[j].z);
      if (d <= 0) {
        ++cnt;
      }
    }
  }
  return cnt;
}

static int verify_a_link(tlk_t *l, tri_t *t, int j, int i, int nbf, int nbv)
{
  int link = (&l[i].a)[j];     /* link = num_tri*4 + num_seg */
  int a1 = (&t[i].a)[j];       /* vertex 1 */
  int b1 = (&t[i].a)[(j+1)%3]; /* vertex 2 */
  int a2,b2;
  int err = 0;

  if (link < 0) {
    //    Error( "Missed links #%d of face %d [%d]\n", j, i, link);
    ++verif_missed;
    return 1;
  } else {
    int code;
    int k = (link & 3);
    int k2 = (k+1) % 3;
    link >>= 2;

    /* Verify index range */
    if (link >= nbf) {
      Error(
	      "bad links #%d of face %d : "
	      "out of range (%d>=%d)\n",
	      j, i, link, nbf);
      return -1;
    }

    b2 = (&t[link].a)[k];
    a2 = (&t[link].a)[(k+1)%3];

    if ((unsigned int)a1>=nbv ||
	(unsigned int)b1>=nbv ||
	(unsigned int)a2>=nbv ||
	(unsigned int)b2>=nbv) {
      Error(
	      "bad links #%d of face %d : "
	      "out of range [%d,%d,%d,%d] >= %d\n",
	      j, i, a1, b1, a2, b2, nbv);
      return -1;
    }
    

    /* Verify the link */
    code = (a1==a2) | ((b1==b2)<<1);
    if (code != 3) {
      Error(
	      "bad links #%d of face %d : "
	      "[code=%d a1=%d b1=%d a2=%d b2=%d]\n",
	      j, i, code, a1,b1,a2,b2);
      return -1;
    }

    /* Verify symetric link */

/*   
    Error(
	    "Ok links #%d of face %d : "
	    "[code=%d a1=%d b1=%d a2=%d b2=%d]\n",
	    j, i, code, a1,b1,a2,b2);
*/
    return 0;
  }
}

static int verify_links(tlk_t *l, tri_t *t, int n, int nbv)
{
  int i, err=0, cnt=0;
  int verif_missed = 0;
  
  for (i=0; i<n; ++i) {
    int j;
    for (j=0; j<3; ++j) {
      int c;
      c = verify_a_link(l,t,j,i, n, nbv);
      if (c<0) {
	err ++;
      } else if (c==1) {
	cnt++;
      }
    }
  }
  return err ? -err : cnt;
}

static int findlinks(tri_t *tri, int nbf, int a, int b)
{
  int i;
  
  for (i=0; i<nbf; ++i, ++tri) {
    if (tri->a == a && tri->b == b) {
      return (i<<2);
    } else if (tri->b == a && tri->c == b) {
      return (i<<2)+1;
    } else if (tri->c == a && tri->a == b) {
      return (i<<2)+2;
    }
  }
  
  return -1;
}


static tlk_t *MakeLinks(obj_t *o)
{
  int i,j;
  int skip = 0;
  tlk_t *lnk;
  
  missed = 0;
  
  lnk = malloc (sizeof(tlk_t) * o->nbf);
  if (!lnk) {
    perror("link buffer");
    return 0;
  }
  o->tlk = lnk;
  
  /* Setup */
  for (i=0; i<o->nbf; ++i) {
    lnk[i].a = lnk[i].b = lnk[i].c = -1;
    lnk[i].flags = 0;
  }
  

  /* Build for each face */
  for (i=0; i<o->nbf; ++i) {
    int *t = &o->tri[i].a;
    int *l = &o->tlk[i].a;
    
    /* For each segment. Segment 0 starts with vertex 0 of face ... */
    for (j=0; j<3; ++j) {
      int k;
      int link = l[j];
      
      if (link >= 0) {
        ++skip;
        continue;
      }
      
      k = findlinks(o->tri, o->nbf, t[(j+1)%3], t[j]);

/*
      if (1 || (i == 0)) {
	Error(
		"F:%d,L:%d [%d %d %d] -> "
		"F:%d,L:%d [%d %d %d]\n",
		i,    j,   o->tri[i].a, o->tri[i].b,    o->tri[i].c,
		k>>2, k&3, o->tri[k>>2].a, o->tri[k>>2].b, o->tri[k>>2].c);
      }
*/

      if (k >= 0) {
        float cos_v, cos_v2;
        int forte;
        
        // face #i seg #j linked to face #k>>2 seg #k&3
        // face #k>>2 seg #k&3 linked to face #i seg#j
        
        l[j] = k;
        //(&lnk[(k>>2)].a)[k&3] = i;
/*
        cos_v  = FaceCosAngle(o,i,k>>2);
        cos_v2 = FaceCosAngle(o,k>>2,i);
//        Error( "ANG= %.04f   %.04f\n", cos_v, cos_v2);
        
        forte = cos_v > 0;
        o->tlk[   i].flags |= forte << j;
        //o->tlk[k>>2].flags |= forte << (k&3);
*/        
      } else {
        missed++;
      }
    }
  }
  Error( "- Missed links : %d / %d\n", missed, o->nbf * 3);
  Error( "- Skipped links : %d / %d\n", skip, o->nbf * 3);
  
  return lnk;
}

static void PrintLinks(tlk_t *l, int n)
{
  printf("{\n");
  while(n--) {
    printf("  { %3d, %3d, %3d, 0 },\n", l->a>>2, l->b>>2, l->c>>2);
    l++;
  } 
  printf("  { -1, -1, -1, 0 },\n");
  printf("};\n");
}

static void scramble(tri_t *t, int n)
{
  while (n--) {
    int v[3], d = n + rand()%83;
    v[(0+d)%3] = t->a;
    v[(1+d)%3] = t->b;
    v[(2+d)%3] = t->c;
    t->b = v[0];
    t->a = v[1];
    t->c = v[2];
    ++t;
  }
}

/* Fake functions for linkage purpose */
int obj3d_init(any_driver_t * d)
{
  return 0;
}

int obj3d_shutdown(any_driver_t * d)
{
  return 0;
}

driver_option_t * obj3d_options(any_driver_t * d,
                                int idx,
                                driver_option_t * o)
{
  return o;
}

int main(int na, char **a)
{
  tlk_t *l;
  obj_t * o = & OBJECTNAME.obj;
  int cnt_missed;

/* DON'T DO THAT !!!! */
/*   Error("******** Scramble Faces ********\n"); */
/*   scramble(o->tri, o->nbf); */

/* #define TOTO(O) (O##_tlk) */
/*   if (o->nbf != sizeof(TOTO(OBJECTNAME)) / sizeof(*TOTO(OBJECTNAME))) { */
/*     Error("Bad number of link faces\n"); */
/*     return 23; */
/*   } */

  indent = 2;
  Error("- Build Nornals...\n");
  indent = 4;
  BuildNormals(o);

  indent = 2;
  Error("- Make links...\n");
  indent = 4;
  l = MakeLinks (o);
  if (!l) {
    return 6;
  }

  indent = 2;
  Error("- Verify links...\n");  
  indent = 4;
  if (cnt_missed = verify_links(l, o->tri, o->nbf, o->nbv), cnt_missed < 0)
    return 3;
  if (cnt_missed != missed || cnt_missed != verif_missed) {
    Error( "Verify error : %d!=%d!=%d\n",
	    cnt_missed, missed,verif_missed);
    return 4;
  }

  
/*
  indent = 2;
  printf("opt vtx:%d\n", 
    optimize_vtx(o->vtx, o->nbv)); 
*/

  indent = 2;
  Error("- Print links...\n");
  indent = 4;
  PrintLinks(l, o->nbf);
  
  return l ? 0 : 2;
}
