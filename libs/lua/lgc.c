/*
** $Id: lgc.c,v 1.3 2003-01-05 18:08:39 zigziggy Exp $
** Garbage Collector
** See Copyright Notice in lua.h
*/

#include "lua.h"

#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"

#include <stdio.h>
#if defined(_WINDOWS)||defined(_WIN32)||defined(WIN32)
#include <windows.h>
#else
//#include <sys/time.h>
#endif

/* dcplaya specific */
#include "sysdebug.h"


static int hasmark (const TObject *o) {
  /* valid only for locked objects */
  switch (o->ttype) {
    case LUA_TSTRING: case LUA_TUSERDATA:
      return !iswhite(tsvalue(o));
    case LUA_TTABLE:
      return !iswhite(hvalue(o));
    case LUA_TFUNCTION:
      return !iswhite(clvalue(o));
    default:  /* number */
      return 1;
  }
}

/* macro for internal debugging; check if a link of free refs is valid */
#define VALIDLINK(L, st,n)      (NONEXT <= (st) && (st) < (n))

static void invalidaterefs (lua_State *L) {
  int n = L->refSize;
  int i;
  for (i=0; i<n; i++) {
    struct Ref *r = &L->refArray[i];
    if (r->st == HOLD && !hasmark(&r->o))
      r->st = COLLECTED;
/*
**  this test must be disabled, since the refArray could have been
**  modified during the garbage collection.
   
    LUA_ASSERT((r->st == LOCK && hasmark(&r->o)) ||
               (r->st == HOLD && hasmark(&r->o)) ||
                r->st == COLLECTED ||
                r->st == NONEXT ||
               (r->st < n && VALIDLINK(L, L->refArray[r->st].st, n)),
               "inconsistent ref table");
*/
  }
  LUA_ASSERT(VALIDLINK(L, L->refFree, n), "inconsistent ref table");
}

static void checktab (lua_State *L, stringtable *tb) {
  if (tb->nuse < (lint32)(tb->size/4) && tb->size > 10)
    luaS_resize(L, tb, tb->size/2);  /* table is too big */
}

static int delstr (lua_State *L, stringtable *tb, TString *ts) {
  TString *next, **p;
  unsigned long h = ts->u.s.hash;
  int h1 = h & (L->strt.size-1);

  p = &tb->hash[h1];
  while ((next = *p) != NULL) {
    if (next == ts) {
      *p = next->nexthash;
      tb->nuse--;
      L->nblocks -= sizestring(next->len);
      luaM_free(L, next);
      return 1;
    }
    else p = &next->nexthash;
  }
  return 0;
}

static int deludata (lua_State *L, stringtable *tb, TString *ts) {
  TString *next, **p;
  int h1 = IntPoint(ts->u.d.value) & (tb->size-1);

  p = &tb->hash[h1];
  while ((next = *p) != NULL) {
    if (next == ts) {
      int tag = next->u.d.tag;
      *p = next->nexthash;
      next->nexthash = L->TMtable[tag].collected;  /* chain udata */
      L->TMtable[tag].collected = next;
      L->nblocks -= sizestring(next->len);
      L->udt.nuse--;
      return 1;
    }
    else p = &next->nexthash;
  }
  return 0;
}

static void collectstring (lua_State *L, TString *ts) {
  int r = delstr(L, &L->strt, ts);
  LUA_ASSERT(r, "fail to collect a string");
}


static int collectudata (lua_State *L, TString *ts) {
  int r = deludata(L, &L->udt, ts);
  LUA_ASSERT(r, "fail to collect a userdata");
  return r;
}

#define MINBUFFER	256
static void checkMbuffer (lua_State *L) {
  if (L->Mbuffsize > MINBUFFER*2) {  /* is buffer too big? */
    size_t newsize = L->Mbuffsize/2;  /* still larger than MINBUFFER */
    L->nblocks += (newsize - L->Mbuffsize)*sizeof(char);
    L->Mbuffsize = newsize;
    luaM_reallocvector(L, L->Mbuffer, newsize, char);
  }
}

static void callgcTM (lua_State *L, const TObject *o) {
  Closure *tm = luaT_gettmbyObj(L, o, TM_GC);
  if (tm != NULL) {
    int oldah = L->allowhooks;
    L->allowhooks = 0;  /* stop debug hooks during GC tag methods */
    luaD_checkstack(L, 2);
    clvalue(L->top) = tm;
    ttype(L->top) = LUA_TFUNCTION;
    *(L->top+1) = *o;
    L->top += 2;
    luaD_call(L, L->top-2, 0);
    L->allowhooks = oldah;  /* restore hooks */
  }
}

static int callgcTMudata (lua_State *L, int *count) {
  int all = 0, times = 0, tag;
  TString *udata;
  TObject o;
  ttype(&o) = LUA_TUSERDATA;
  if (!count) {
     L->gcTM = L->last_tag;
     all = 1;
  }
  do {
    tag = L->gcTM;
    
    while ((udata = L->TMtable[tag].collected) != NULL && (!count || *count > 0)) {
      L->TMtable[tag].collected = udata->nexthash;  /* remove it from list */
      if (L->TMtable[tag].method[TM_GC]) {
        tsvalue(&o) = udata;
        callgcTM(L, &o);
        times++;
      }
      if (count) *count = *count - 1;
      luaM_free(L, udata);
    }
    if (!udata) {
       if (tag == 0) 
         return 1;
       else
         L->gcTM--;
    }
  } while (all);
#ifdef LUA_DEBUG
if (times) {
fprintf(stderr, "collect %d times %d\n", tag, times);
}
#endif
  return 0;
}

static void markproto (Proto *f, lua_State *L) {
  if (!isblack(f)) {
    int i;
    blackmark(f);
    greymark(L, f->source);
    for (i=0; i<f->nkstr; i++)
      greymark(L, f->kstr[i]);
    for (i=0; i<f->nkproto; i++)
      greymark(L, (GCValue*)f->kproto[i]);
    for (i=0; i<f->nlocvars; i++)  /* mark local-variable names */
      greymark(L, f->locvars[i].varname);
  }
}

static void markstack (lua_State *L) {
  StkId o;
  for (o=L->stack; o<L->top; o++)
    markobject(o, L, 0);
}

static void marklock (lua_State *L) {
  int i;
  for (i=0; i<L->refSize; i++) {
    if (L->refArray[i].st == LOCK)
      markobject(&L->refArray[i].o, L, 0);
  }
}

static void markclosure (Closure *cl, lua_State *L) {
  int i;
  
  if (!isblack(cl)) {
    blackmark(cl);
    if (!cl->isC)
      greymark(L, cl->f.l);
    
    for (i=0; i<cl->nupvalues; i++)  /* mark its upvalues */
      markobject(&cl->upvalue[i], L, 0);
  }
}

static void marktagmethods (lua_State *L) {
  int e;
  for (e=0; e<TM_N; e++) {
    int t;
    for (t=0; t<=L->last_tag; t++) {
      Closure *cl = luaT_gettm(L, t, e);
      if (cl) greymark(L, cl);
    }
  }
}

static void markhash (Hash *h, lua_State *L) {
  if (!isblack(h)) {
    int i;
    
    blackmark(h);
    
    for (i=0; i<h->size; i++) {
      Node *n = node(h, i);
      if (ttype(key(n)) != LUA_TNIL) {
        if (ttype(val(n)) == LUA_TNIL)
          luaH_remove(h, key(n));  /* dead element; try to remove it */
        markobject(&n->key, L, 0);
        markobject(&n->val, L, 0);
      }
    }
  }
}

static void markvalue (GCValue *v, lua_State *L, int hard) {
  switch(v->vtype) {
    case LUA_VUdata:  case LUA_VTString:
      blackmark(v);
      break;
    case LUA_VClosure:
      if (hard) markclosure((Closure*)v, L);
      else greymark(L, v);
      break;
    case LUA_VProto:
      if (hard) markproto((Proto*)v, L);
      else greymark(L, v);
      break;
    case LUA_VHash:
      if (hard) markhash((Hash*)v, L);
      else greymark(L, v);
      break;
    default: break;  /* numbers, etc */
  }
}

void markobject (TObject *o, lua_State *L, int hard) {
  switch (ttype(o)) {
    case LUA_TUSERDATA:
      LUA_ASSERT(tsvalue(o)->vtype == LUA_VUdata, "type disagree!");
      markvalue((GCValue*)tsvalue(o), L, hard);
      break;
    case LUA_TSTRING:
      LUA_ASSERT(tsvalue(o)->vtype == LUA_VTString, "type disagree!");
      markvalue((GCValue*)tsvalue(o), L, hard);
      break;
    case LUA_TMARK:
      LUA_ASSERT(infovalue(o)->func->vtype == LUA_VClosure, "type disagree!");
      markvalue((GCValue*)infovalue(o)->func, L, hard);
      break;
    case LUA_TFUNCTION:
      LUA_ASSERT(clvalue(o)->vtype == LUA_VClosure, "type disagree!");
      markvalue((GCValue*)clvalue(o), L, hard);
      break;
    case LUA_TTABLE: 
      LUA_ASSERT(hvalue(o)->vtype == LUA_VHash, "type disagree!");
      markvalue((GCValue*)hvalue(o), L, hard);
      break;
    default: break;  /* numbers, etc */
  }
}

static void collectvalue (lua_State *L, GCValue *v) {
  int r;
  switch(v->vtype) {
    case LUA_VTString:
      collectstring(L, (TString*)v);
      break;
    case LUA_VUdata:
      r = collectudata(L, (TString*)v);
      if (r) L->gcTM = L->last_tag;
      break;
    case LUA_VClosure:
      luaF_freeclosure(L, (Closure*)v);
      break;
    case LUA_VProto:
      luaF_freeproto(L, (Proto*)v);
      break;
    case LUA_VHash: 
      luaH_free(L, (Hash*)v);
      break;
    default:
      break;
  }
  L->gcalloc--;
}

static int luaC_mark (lua_State *L, int *count)
{
  while (*count > 0) {
    GCValue *p = L->gcptr;
    if (isgrey(L->gcptr))
      markvalue(L->gcptr, L, 1);
    LUA_ASSERT(p == L->gcptr, "gcptr changed value!");
    L->gcptr = L->gcptr->gcnext;
    if (L->gcptr == L->gcroot) {
      return 1;
    }
    *count = *count - 1;
  }
  return 0;
}

static int _luaC_collect (lua_State *L, int *count, int flag) {
  if (flag) {	/* collect all objects, start from root */
    L->gcptr = L->gcroot;
  }  

  while ((!count || *count > 0) && L->gcptr) {
    if ((!flag && iswhite(L->gcptr)) || (flag == 1) || 
    	(flag == -1 && L->gcptr->vtype == LUA_VUdata)) {
      GCValue *p = L->gcptr;

      p->gcprev->gcnext = p->gcnext;
      p->gcnext->gcprev = p->gcprev;

      if (L->gcroot == L->gcptr) { 
        if (L->gcroot == L->gcroot->gcprev) { /* last to free? */
          L->gcroot = L->gcptr = NULL;
        }
        else {
          L->gcroot = L->gcptr = p->gcnext;
        }
      }
      else {
        L->gcptr = p->gcnext;
	if (L->gcptr == L->gcroot)
          return 1;
      }

      collectvalue(L, p);
    }
    else {
      if (L->gcptr->marked <= 1) L->gcptr->marked = 0;
      L->gcptr = L->gcptr->gcnext;
      if (L->gcptr == L->gcroot)
        return 1;
    }
    if (count) *count = *count - 1;
  }

  return 0;
}

void luaC_collectgarbage (lua_State *L, int count) {
  if (L->gcstage == LUA_GCIDLE) {	/* let's start marking important stuffs */
    markvalue((GCValue*)L->gt, L, 0);  /* mark table of globals */
    marktagmethods(L);  /* mark tag methods */
    marklock(L); /* mark locked objects */
    L->gcstage = LUA_GCMARK;
    L->gcptr = L->gcroot;
  }
  if (L->gcstage == LUA_GCMARK) {	/* continue to mark */
    int r;
    markstack(L); /* mark stack objects */
    r = luaC_mark(L, &count);
    if (r == 1) {	/* reach the end of it, should change to sweep */    
      L->gcstage = LUA_GCSWEEPSTART;
      L->gcptr = NULL;
    }
  }
  if (L->gcstage == LUA_GCSWEEPSTART) {
    invalidaterefs(L);  /* check unlocked references */
    L->gcTM = -1;
    L->gcstage = LUA_GCSWEEP;
    L->gcptr = L->gcroot;
  }
  if (L->gcstage == LUA_GCSWEEP) {	/* continue to sweep */
    int r;
    r = _luaC_collect(L, &count, 0);
    if (r == 1) {	/* finish collecting */
      L->gcstage = LUA_GCIDLE;
      L->gcptr = NULL;
      checktab(L, &L->strt);
      checktab(L, &L->udt);
      checkMbuffer(L);
      L->gcstage = LUA_GCTMCALLBACK;
    }
  }
  if (L->gcstage == LUA_GCTMCALLBACK) {
    int r = 1;
    if (L->gcTM >= 0) 
	r = callgcTMudata(L, &count);
    if (r) {
      L->gcstage = LUA_GCIDLE;
      L->gcTM = 0;        
      callgcTM(L, &luaO_nilobject);
      SDDEBUG("collect done %ld %ld\n", L->GCthreshold, L->nblocks);
    }
  }

}

int luaC_collect (lua_State *L, int step) {

  L->last_gcalloc = -L->gcalloc;

  if (step < 0) { /* collect everything! */
    L->gcstage = LUA_GCTMCALLBACK;
    L->gcTM = 0;
    _luaC_collect (L, 0, -1);
    if (L->gcTM) {
      callgcTMudata(L, 0);
    }
    _luaC_collect (L, 0, 1);
  }
  else {
    luaC_collectgarbage(L, step ? step : LUA_GCSTEP);
  }

  L->last_gcalloc = L->gcalloc;

  return 0;
}

void luaC_checkGC (lua_State *L) {

  if (L->last_gcalloc < 0) return;	/* to avoid recursively checking GC */

  if (L->GCthreshold < L->nblocks) {
      do {
      	luaC_collect(L, 1000);
      } while (L->gcstage != LUA_GCIDLE);
      L->GCthreshold = L->nblocks * 2;
  }
}
