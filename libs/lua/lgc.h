/*
** $Id: lgc.h,v 1.2 2003-01-05 18:08:39 zigziggy Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/

#ifndef lgc_h
#define lgc_h

#define LUA_GCIDLE		0
#define LUA_GCMARK		1
#define LUA_GCSWEEPSTART	2
#define LUA_GCSWEEP		3
#define LUA_GCTMCALLBACK	4
#define LUA_GCFINAL		5

#define LUA_GCSTEP		100

#define mark_newvalue(L, v) { 			\
  v->marked = -1; 				\
  L->gcalloc++;					\
  if (!L->gcroot) {				\
    v->gcprev = v->gcnext = (GCValue*)v;	\
    L->gcroot = (GCValue*)v;			\
  }						\
  else {					\
    v->gcprev = L->gcroot->gcprev;		\
    v->gcnext = L->gcroot;			\
    L->gcroot->gcprev->gcnext = (GCValue*)v;	\
    L->gcroot->gcprev = (GCValue*)v;		\
  }						\
}

/*
   adjust v's mark when u now holds a new pointer to v. Basically
   it greymarks v if v is white and u is black
   
   this is to ensure coherence.
*/
#define adjust_mark(L, u, v) 	\
  if (iswhite(v) && isblack(u)) { greymark(L, v); }

/*
   greymark an object is to only mark itself as grey and do not
   traverse its references. It also moves the previously unmarked object
   to the end of marking list (to be marked again).
   
   mark an object is to
   	1) skip marking if the object is already black;
   	2) otherwise, blackmark it, and greymark all its referenced objects;
   	
   So eventually all grey objects shall be black when the marking completes.
*/

#define greymark(L, v) {					\
  if (!(v)->marked) {						\
    if ((v)->vtype == LUA_VTString || (v)->vtype == LUA_VUdata)	\
      (v)->marked = 1;						\
    else {							\
      (v)->marked = -1;						\
      if ((GCValue*)(v) != L->gcroot) {				\
        if ((v)->gcprev) (v)->gcprev->gcnext = (v)->gcnext; 	\
        if ((v)->gcnext) (v)->gcnext->gcprev = (v)->gcprev; 	\
        if ((GCValue*)(v) == L->gcptr) {			\
          L->gcptr = (v)->gcnext;				\
        }							\
        (v)->gcprev = L->gcroot->gcprev;			\
        (v)->gcnext = L->gcroot;				\
        L->gcroot->gcprev->gcnext = (GCValue*)(v);		\
        L->gcroot->gcprev = (GCValue*)(v);			\
      }								\
    }								\
  }								\
}
      
#define blackmark(v)	(v)->marked = (v)->marked ? abs((v)->marked) : 1

#define iswhite(v)	((v)->marked == 0)
#define isgrey(v)  	((v)->marked < 0)
#define isblack(v)  	((v)->marked > 0)

#include "lobject.h"

void markobject (TObject *o, lua_State *L, int hard);

int luaC_collect (lua_State *L, int all);
void luaC_checkGC (lua_State *L);

void luaC_collectgarbage (lua_State *L, int count);

#endif
