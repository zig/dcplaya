/**
 * @ingroup  exe_plugin
 * @file     display_matrix.c
 * @author   Vincent Penne <ziggy@sashipa.com>
 * @author   Benjamin Gerard <ben@sashipa.com>
 * @date     2002/09/25
 * @brief    graphics lua extension plugin, matrix interface
 * 
 * $Id: display_matrix.c,v 1.5 2003-01-11 07:44:59 zigziggy Exp $
 */

#include <stdlib.h>
#include <dc/fmath.h>
#include "driver_list.h"
#include "display_matrix.h"
#include "allocator.h"
#include "sysdebug.h"

int matrix_tag;

static allocator_t * matrixref_allocator;
static allocator_t * matrixdef_allocator;

#define NEW_MATRIX(M,REF,lines,cols) \
  if ((M) = create_matrix(lines,cols), !(M)) { \
    printf("%s : malloc error\n", __FUNCTION__); \
    return 0; \
  } \
  (REF) = (M)->md; \
  driver_reference(&display_driver)
/*   printf("%s : NEW_MATRIX(%d) := %p\n", __FUNCTION__, lines, (M)); */

#define REF_MATRIX(M,REF,lines) \
  if ((M) = allocator_alloc(matrixref_allocator, sizeof(*(M))), !(M)) { \
    printf("%s : malloc error\n", __FUNCTION__); \
    return 0; \
  } \
  (M)->li = lines; \
  (M)->md = (REF); \
  (REF)->refcount++

static lua_matrix_t * create_matrix(int lig, int col)
{
  lua_matrix_def_t * md = 0;
  lua_matrix_t *m = 0;
  int size;

  if (lig <= 0 || col <= 0) {
	return 0;
  }

  size = sizeof(lua_matrix_def_t)
	+ sizeof(float)*(lig*col)
	- sizeof(md->v);

  md = allocator_alloc(matrixdef_allocator, size);
  if (md) {
	md->refcount = 1;
	md->l = lig;
	md->c = col;
  }
  m = allocator_alloc(matrixref_allocator, sizeof(*m));
  if (!m) {
	allocator_free(matrixdef_allocator, md);
  } else {
	m->md = md;
	m->li = 0;
  }
  return m;
}

DL_FUNCTION_START(set_trans)
{
  lua_matrix_t * mat;
  lua_matrix_def_t * md;
  CHECK_MATRIX(2);
  GET_MATRIX(mat,md,2,4,4);
  dl_set_trans(dl, *(const matrix_t *) md->v);
  return 0;
}
DL_FUNCTION_END()

DL_FUNCTION_START(get_trans)
{
  lua_matrix_t * m;
  lua_matrix_def_t * md;

  NEW_MATRIX(m, md, 4, 4);
  memcpy(md->v, dl_get_trans(dl), sizeof(matrix_t));
  lua_settop(L, 0);
  lua_pushusertagsz(L, m, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}
DL_FUNCTION_END()

DL_FUNCTION_DECLARE(mat_gc)
{
  lua_matrix_t * mat;
  lua_matrix_def_t * md;
#ifdef DEBUG
  int refcount;
  int c;
#endif


  CHECK_MATRIX(1);
  GET_MATRIX_OR_VECTOR(mat,md,1,0);

#ifdef DEBUG
  refcount = md->refcount;
  c        = md->c;
#endif


  if (--md->refcount<=0) {
	if (md->refcount < 0) {
	  SDCRITICAL("%s : [%p] refcount=%d !!!\n",
				 __FUNCTION__, md, md->refcount);
	}
/* 	printf("%s : destroy matrix def %p\n", __FUNCTION__, md); */
	allocator_free(matrixdef_allocator, md);
	driver_dereference(&display_driver);
  }
#if defined DEBUG && 0
  if (mat->li) {
	printf("%s : [m:%p c:%d li:%d]\n", __FUNCTION__,
		   mat, refcount, ((mat->li - md->v) * c)+1);
  }
#endif

  allocator_free(matrixref_allocator, mat);
  return 0;
}

static int char2col(const lua_matrix_def_t *md, const char *s)
{
  /* x y z w a r g b u v */
  /* 0 1 2 3 4 5 6 7 8 9 */
  /* x y z w u v */
  /* 0 1 2 3 4 5 */

  int k;
  if (!s) return -1;
  k = *s;
  if ( k >= 'w' && k <= 'z' ) {
	return ((k-'w') - 1) & 3;
  }
  switch (k) {
  case 'a':
	return 4;
  case 'r':
	return 5;
  case 'g':
	return 6;
  case 'b':
	return 7;
  case 'u': case 'v':
	return k-'u' + 4 + ((md->c > 8)<<2);
  }
  return -1;
}

DL_FUNCTION_DECLARE(mat_gettable)
{
  lua_matrix_t * m;
  lua_matrix_def_t * md;
  unsigned int c = 0, l = 0;
  float * li;

  GET_MATRIX_OR_VECTOR(m,md,1,0);
  if (li=m->li, li) {
	/* Matrix is a line vector */  
	int type = lua_type(L,2);
	if (type == LUA_TNUMBER) {
	  c = lua_tonumber(L,2) - 1;
	} else if (type == LUA_TSTRING) {
	  c = char2col(md,lua_tostring(L,2));
	}
/*  	printf("%s(%d,%d) : m:%p d:%p c:%d\n", __FUNCTION__, */
/*  		   ((li - md->v)*md->c)+1, c+1, m, md, md->refcount); */
	if (c >= md->c) {
	  printf("%s : column index %d out of range\n", __FUNCTION__, c+1);
	  return 0;
	}
	lua_settop(L, 0);
	lua_pushnumber(L, li[c]);
	return 1;
  } else {
	lua_matrix_t * r;

	l = lua_tonumber(L,2) - 1;
/*printf("%s(%d) : m:%p d:%p c:%d\n", __FUNCTION__, l+1, m,md,md->refcount); */
	if (l >= md->l) {
	  printf("%s : line index %d out of range\n", __FUNCTION__, l+1);
	  return 0;
	}
	REF_MATRIX(r, md, &md->v[l * md->c]);
	lua_settop(L, 0);
	lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
/* 	m->li = &md->v[l<<md->log2]; */
/* 	lua_settop(L, 1); */
 	return 1;
  }
  return 0;
}

DL_FUNCTION_DECLARE(mat_settable)
{
  lua_matrix_t * m;
  lua_matrix_def_t * md;
  unsigned int c = 0, l = 0;
  float * li;

  GET_MATRIX_OR_VECTOR(m,md,1,0);

  if (li=m->li, li) {
	int type;
	/* Matrix reference vector */
	if (lua_type(L,3) != LUA_TNUMBER) {
	  printf("%s : bad type argument #3.\n", __FUNCTION__);
	  return 0;
	}
	type = lua_type(L,2);
	if (type == LUA_TNUMBER) {
	  c = lua_tonumber(L,2) - 1;
	} else if (type == LUA_TSTRING) {
	  c = char2col(md,lua_tostring(L,2));
	}
/* 	printf("%s(%d,%d) : m:%p d:%p c:%d := %.3f\n", __FUNCTION__, */
/* 		   ((li - md->v)>>md->log2)+1, c+1, */
/* 		   m, md, md->refcount, lua_tonumber(L,3)); */
	if (c >= md->c) {
	  printf("%s : column index %d out of range\n", __FUNCTION__, c+1);
	  return 0;
	}
	li[c] = lua_tonumber(L,3);
  } else {
	lua_matrix_t * r;
	lua_matrix_def_t * rd;
	GET_VECTOR(r,rd,3,md->c);
	l = lua_tonumber(L,2) - 1;

/* 	printf("%s(%d) : m:%p d:%p c:%d := m:%p d:%p c:%d\n", __FUNCTION__, */
/* 		   l+1, m, md, md->refcount, r, rd, rd->refcount); */

	if (l >= md->l) {
	  printf("%s : line index %d out of range\n", __FUNCTION__, l+1);
	  return 0;
	}
	memcpy(&md->v[l * md->c], r->li, md->c<<2);
  }
  return 0;
}

#if 0

DL_FUNCTION_DECLARE(mat_getglobal)
{
  lua_matrix_t * mat, * r;
  lua_matrix_def_t * md;
/*   const char * global = lua_tostring(L,1); */

  CHECK_MATRIX(2);
  GET_MATRIX_OR_VECTOR(mat,md,2,0);
  REF_MATRIX(r, md, mat->li);
/*   printf("%s : [%s] m:%p d:%p c:%d -> m:%p d:%p c:%d\n", __FUNCTION__, global, */
/* 		 mat, md, md->refcount-1, r, r->md, r->md->refcount); */
  lua_settop(L,0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}


DL_FUNCTION_DECLARE(mat_setglobal)
{
  lua_matrix_t * mat;
  lua_matrix_def_t * md;
  const char * global = lua_tostring(L,1);

  /* NAME MTX VALUE */

  CHECK_MATRIX(2);
  GET_MATRIX_OR_VECTOR(mat,md,2,0);
/*   printf("%s : [%s] m:%p r:%p c:%d -> ", */
/* 		 __FUNCTION__, global, mat, md, md->refcount); */
  md->refcount--;

  if (lua_tag(L, 3) != matrix_tag) {
	lua_remove(L,2);
	/* NAME VALUE */
/* 	printf("[%s]=<%s>,%s\n", lua_tostring(L,1), lua_tostring(L,2), */
/* 		   lua_typename(L, lua_type(L,2)));  */
  } else {
	lua_matrix_t * r;
	lua_matrix_def_t * rd;

	GET_MATRIX_OR_VECTOR(r,rd,3,0);
	rd->refcount++;
	*mat = *r;
/* 	printf("m:%p r:%p c:%d\n", mat, rd, rd->refcount); */
	lua_settop(L,1);
	/* NAME */
	lua_pushusertagsz(L, mat, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
	/* NAME VALUE */
  }
  /* NAME VALUE */
  lua_getglobals(L);
  /* NAME VALUE GLOBALS */
  lua_insert(L,1);
  /* GLOBALS NAME VALUE GLOBALS */
  lua_settop(L,3);
  /* GLOBALS NAME VALUE */
  lua_settable(L, 1);
  lua_settop(L,0);
  return 0;
}
#endif 

static void mat_mult(lua_matrix_def_t * rd,
					lua_matrix_def_t * leftd,
					lua_matrix_def_t * rightd)
{
  int bytes = rd->c << 2;

  MtxVectorsMult(rd->v, leftd->v, *(matrix_t*)rightd->v, rd->l, bytes, bytes);
  /* Copy left matrix remaining data into destination. */
  if (rd != leftd && rd->c > 4) {
	MtxCopyFlexible(rd->v+4, leftd->v+4, rd->l, rd->c-4, bytes, bytes);
  }
}

DL_FUNCTION_DECLARE(mat_mult)
{
  lua_matrix_t * r, * left, * right;
  lua_matrix_def_t * rd, * leftd, * rightd;

  CHECK_MATRIX(1);
  CHECK_MATRIX(2);

  GET_MATRIX(right,rightd,2,4,4);
  GET_MATRIX(left,leftd,1,0,0);
  NEW_MATRIX(r,rd,leftd->l, leftd->c);

  mat_mult(rd,leftd,rightd);
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(mat_mult_self)
{
  lua_matrix_t * r, * left, * right;
  lua_matrix_def_t * rd, * leftd, * rightd;
  int n = lua_gettop(L);

  if (n < 2 || n > 3) {
	printf("%s : bad arguments\n", __FUNCTION__);
  }

  CHECK_MATRIX(1);
  CHECK_MATRIX(2);
  if (n == 3) {
	CHECK_MATRIX(3);
  }
	
  GET_MATRIX(right,rightd,n,4,4);
  GET_MATRIX(left,leftd,n-1,0,0);
  if (n == 3) {
	GET_MATRIX(r,rd,1,leftd->l,leftd->c);
  } else {
	r = left;
	rd = leftd;
  }
  mat_mult(rd,leftd,rightd);
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(init_matrix_type)
{
  lua_getglobal(L,"matrix_tag");
  matrix_tag = lua_tonumber(L,1);
  if (! matrix_tag) {
	matrix_tag = lua_newtag(L);
	lua_pushnumber(L,matrix_tag);
	lua_setglobal(L,"matrix_tag");
  }

  lua_pushcfunction(L, lua_mat_gc);
  lua_settagmethod(L, matrix_tag, "gc");
  
  lua_pushcfunction(L, lua_mat_mult);
  lua_settagmethod(L, matrix_tag, "mul");

  lua_pushcfunction(L, lua_mat_gettable);
  lua_settagmethod(L, matrix_tag, "gettable");

  lua_pushcfunction(L, lua_mat_settable);
  lua_settagmethod(L, matrix_tag, "settable");

/*   lua_pushcfunction(L, lua_mat_getglobal); */
/*   lua_settagmethod(L, matrix_tag, "getglobal"); */

/*   lua_pushcfunction(L, lua_mat_setglobal); */
/*   lua_settagmethod(L, matrix_tag, "setglobal"); */

  return 0;
}

DL_FUNCTION_DECLARE(shutdown_matrix_type)
{
  return 0;
}

DL_FUNCTION_DECLARE(mat_new)
{
  lua_matrix_t * r;
  lua_matrix_def_t * rd;

  if (lua_tag(L,1) == matrix_tag) {
	lua_matrix_t * m;
	lua_matrix_def_t * md;
	GET_MATRIX(m,md,1,0,0);
	NEW_MATRIX(r, rd, md->l, md->c);
	memcpy(rd->v, md->v, rd->l * rd->c << 2);
  } else {
	int l = 0, c = 0;
	int n = lua_gettop(L);

	if (n >= 1) {
	  l = (int) lua_tonumber(L,1);
	}
	if (n >= 2) {
	  c = (int) lua_tonumber(L,2);
	}

	if (!l) {
	  l = 4;
	}
	if (c<=0) {
	  c = 4;
	}
	NEW_MATRIX(r, rd, l, c);

	if (l == 4 && c == 4) {
	  MtxIdentity(* (matrix_t *) rd->v);
	} else {
	  memset(rd->v, 0, l * c << 2);
	}
  }
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

#define Cos fcos
#define Sin fsin
#define Inv(a) (1.0f/(a))

static void myMtxRotateX(matrix_t m2, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  MtxIdentity(m2);
  m2[1][1] = m2[2][2] = ca;
  m2[1][2] = sa;
  m2[2][1] = -sa;
}

static void myMtxRotateY(matrix_t m2, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  MtxIdentity(m2);
  m2[0][0] = m2[2][2] = ca;
  m2[2][0] = sa;
  m2[0][2] = -sa;
}

static void myMtxRotateZ(matrix_t m2, const float a)
{
  const float ca = Cos(a);
  const float sa = Sin(a);
  MtxIdentity(m2);
  m2[0][0] = m2[1][1] = ca;
  m2[0][1] = sa;
  m2[1][0] = -sa;
}

DL_FUNCTION_DECLARE(mat_rotx)
{
  lua_matrix_t * r;
  lua_matrix_def_t * rd;
  NEW_MATRIX(r,rd,4,4);
  myMtxRotateX(* (matrix_t *) rd->v, lua_tonumber(L, 1));
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(mat_roty)
{
  lua_matrix_t * r;
  lua_matrix_def_t * rd;

  NEW_MATRIX(r,rd,4,4);
  myMtxRotateY(* (matrix_t *) rd->v, lua_tonumber(L, 1));
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(mat_rotz)
{
  lua_matrix_t * r;
  lua_matrix_def_t * rd;

  NEW_MATRIX(r,rd,4,4);
  myMtxRotateZ(* (matrix_t *) rd->v, lua_tonumber(L, 1));
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(mat_scale)
{
  lua_matrix_t * r;
  lua_matrix_def_t * rd;

  NEW_MATRIX(r,rd,4,4);
  memset(rd->v, 0, sizeof(matrix_t));
  rd->v[ 0] = lua_tonumber(L, 1);
  rd->v[ 5] = lua_tonumber(L, 2);
  rd->v[10] = lua_tonumber(L, 3);
  rd->v[15] = 1.0f;
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(mat_trans)
{
  lua_matrix_t * r;
  lua_matrix_def_t * rd;

  NEW_MATRIX(r,rd,4,4);
  MtxIdentity(* (matrix_t *) rd->v);
  rd->v[12] = lua_tonumber(L, 1);
  rd->v[13] = lua_tonumber(L, 2);
  rd->v[14] = lua_tonumber(L, 3);
  rd->v[15] = 1.0f/* + lua_tonumber(L, 4)*/;
  lua_settop(L, 0);
  lua_pushusertagsz(L, r, matrix_tag, sizeof(lua_matrix_t) + sizeof(lua_matrix_def_t));
  return 1;
}

DL_FUNCTION_DECLARE(mat_li)
{
  int n = lua_gettop(L);
  unsigned int l;
  lua_matrix_t * m;
  lua_matrix_def_t * md;
  
  CHECK_MATRIX(1);
  GET_MATRIX(m,md,1,0,0);

  if (n != 2) {
	printf("%s : bad arguments\n", __FUNCTION__);
	return 0;
  }

  l = lua_tonumber(L,2) - 1;
  if (l >= md->l) {
	printf("%s : line index %d out of range\n", __FUNCTION__, l+1);
	return 0;
  }

  lua_settop(L,0);
  lua_newtable(L);
  {
	float *v = &md->v[l * md->c];
	for (n=0; n<md->c; ++n) {
	  lua_pushnumber(L, n+1);
	  lua_pushnumber(L, *v++);
	  lua_rawset(L, 1);
	}
  }
  return 1;
}

DL_FUNCTION_DECLARE(mat_co)
{
  int n = lua_gettop(L);
  unsigned int c;
  lua_matrix_t *m;
  lua_matrix_def_t * md;
  
  CHECK_MATRIX(1);
  GET_MATRIX(m,md,1,0,0);

  if (n != 2) {
	printf("%s : bad arguments\n", __FUNCTION__);
	return 0;
  }

  c = lua_tonumber(L,2) - 1;
  if (c >= md->c) {
	printf("%s : column index %d out of range\n", __FUNCTION__, c+1);
	return 0;
  }

  lua_settop(L,0);
  lua_newtable(L);
  {
	float * v = md->v+c;
	for (n=0; n<md->l; ++n, v += md->c) {
	  lua_pushnumber(L, n+1);
	  lua_pushnumber(L, *v);
	  lua_rawset(L, 1);
	}
  }
  return 1;
}

DL_FUNCTION_DECLARE(mat_el)
{
  int n = lua_gettop(L);
  unsigned int l, c;
  lua_matrix_t * m;
  lua_matrix_def_t * md;
  
  CHECK_MATRIX(1);
  GET_MATRIX(m,md,1,0,0);

  if (n != 3) {
	printf("%s : bad arguments\n", __FUNCTION__);
	return 0;
  }

  l = lua_tonumber(L,2) - 1;
  c = lua_tonumber(L,3) - 1;
  if (c >= md->c || l >= md->l) {
	printf("%s : indexes (%d,%d) out of range\n", __FUNCTION__, l+1, c+1);
	return 0;
  }

  lua_settop(L,0);
  lua_pushnumber(L, md->v[l * md->c + c]);
  return 1;
}

DL_FUNCTION_DECLARE(mat_dim)
{
  lua_matrix_t * m;
  lua_matrix_def_t * md;
  
  CHECK_MATRIX(1);
  GET_MATRIX_OR_VECTOR(m,md,1,0);

  lua_settop(L,0);
  if (!m->li) {
	lua_pushnumber(L, md->l);
  }
  lua_pushnumber(L, md->c);
  return lua_gettop(L);
}

static void dump_matrix_def(const lua_matrix_def_t *def, int idx, int level,
							const char *indent)
{
  if (!indent) {
	indent = "";
  }
  if (level > 0) {
	printf("%s#%04d  %p[%dx%d] refcount:%d\n", indent, idx,
		   def, def->l, def->c, def->refcount);
  }
  if (level > 1) {
	int j;
	printf("%s [\n",indent);
	for (j=0; j<def->l; ++j) {
	  int k;
	  printf("%s  [ ", indent);
	  for (k=0; k<def->c; ++k) {
		printf("%-5.2f ", def->v[k + j * def->c]);
	  }
	  printf("]\n");
	}
	printf("%s  ] \n", indent);
  }
}

static void dump_matrix_ref(const lua_matrix_t *m, int idx, int level,
							const char *indent)
{
  if (level > 0) {
	 printf("%s#%4d  %p",indent, idx, m->md);
	 if (m->li) {
	   printf("[%d]", ((m->li - m->md->v) / m->md->c) + 1);
	 }
	 printf("\n");
  }
}

DL_FUNCTION_DECLARE(mat_dump)
{
  lua_matrix_t * m;
  lua_matrix_def_t * md;
  int level;
  CHECK_MATRIX(1);
  GET_MATRIX_OR_VECTOR(m,md,1,0);
  level = lua_tonumber(L,2) + 1;
  dump_matrix_def(md, 0, level, "");
  dump_matrix_ref(m, 0, level, "");
  lua_settop(L,0);
  return 0;
}

DL_FUNCTION_DECLARE(mat_stat)
{
  int i, level = lua_tonumber(L, 1);
  allocator_elt_t * e;

  printf("\n"
		 "Matrix statistics :\n");

  printf("\n"
		 " Matrix definitions : ");
  if (!matrixdef_allocator) {
	printf("[NONE]!\n");
  } else {
	int f,u;
	allocator_t * a = matrixdef_allocator;
	allocator_lock(a);
	for (f=0, e = a->free; e; e = e->next, ++f)
	  ;
	for (u=0, e = a->used; e; e = e->next, ++u)
	  ;

	printf("[e-size:%d  size:%d  free:%d  used:%d]\n",
		   a->elt_size, f+u, f, u);
	if (level > 0) {
	  for (i=0, e = a->used; e; e = e->next, ++i) {
		lua_matrix_def_t *def = (lua_matrix_def_t *)(e+1);
		dump_matrix_def(def, i, level, "  ");
	  }
	}
	allocator_unlock(a);
  }

  printf("\n"
		 " Matrix reference : ");
  if (!matrixref_allocator) {
	printf("[NONE]!\n");
  } else {
	int f,u;
	allocator_t * a = matrixref_allocator;
	allocator_lock(a);
	for (f=0, e = a->free; e; e = e->next, ++f)
	  ;
	for (u=0, e = a->used; e; e = e->next, ++u)
	  ;
	printf("[e-size:%d  size:%d  free:%d  used:%d]\n",
		   a->elt_size, f+u, f, u);
	if (level > 0) {
	  for (i=0, e = a->used; e; e = e->next, ++i) {
		lua_matrix_t * m = (lua_matrix_t *)(e+1);
		dump_matrix_ref(m, i, level, "  ");
	  }
	}
	allocator_unlock(a);
  }

  return 0;
}

int display_matrix_shutdown(void)
{
  allocator_destroy(matrixdef_allocator);
  allocator_destroy(matrixref_allocator);
  matrixdef_allocator = 0;
  matrixref_allocator = 0;
  return 0;
}

int display_matrix_init(void)
{
  if (!matrixdef_allocator) {
	matrixdef_allocator = allocator_create(256, sizeof(lua_matrix_def_t));
  }
  if (!matrixref_allocator) {
	matrixref_allocator = allocator_create(256, sizeof(lua_matrix_t));
  }

  if (!matrixdef_allocator || !matrixref_allocator) {
	display_matrix_shutdown();
	return -1;
  }
  return 0;
}
