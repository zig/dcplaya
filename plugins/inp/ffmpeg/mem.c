
/**
 * @file mem.c
 * default memory allocator for libavcodec.
 */

#undef MALLOC_DEBUG 
#include <malloc.h>
#include <stdio.h>
#include <assert.h>

int av_nballocs;

//#define MEMALIGN_HACK 1
#define HAVE_MEMALIGN

#define ACHK
#define MAX_ALLOC_REMEMBER 1000

#define FALSE 0
#define TRUE 1
typedef struct tALLOC_REMEMBER tALLOC_REMEMBER;
struct tALLOC_REMEMBER {
   tALLOC_REMEMBER *Next;
   void *p;
   const char * file;
   int line;
  //char Message[256];
//   int  n;
};

static tALLOC_REMEMBER AllocRemember[MAX_ALLOC_REMEMBER];
static tALLOC_REMEMBER *HTableAllocRemember[256], *FreeAllocRemember;
static int do_not_check_any_more = FALSE;

static void *LastChecked;
//static char *LastMessage = "SYSTEM ALLOC";
static tALLOC_REMEMBER *LastAR;

static char Message[256];

static void FATAL()
{
   printf("\nFFMPEG - mem.c\n**** FATAL ERROR - MUST EXIT IMMEDIATLY.\n");
   exit(1);
}

static void unremember( void *p )
{
   if(FreeAllocRemember == 0)
   {
     printf( "Free on unregistered pointer in '%s'\n", Message? Message:"?");
     if(!Message) return;
     //WriteErrorMessage(0, "Free on unregistered pointer in '%s'\n", Message);
     return;
     FATAL();
   }
   unsigned char h = (((int)p)&255) ^ ((((int)p)>>8)&255)
                                  ^ ((((int)p)>>16)&255)
                                  ^ ((((int)p)>>24)&255);
   tALLOC_REMEMBER *ar = HTableAllocRemember[h], *ar2=0;
   while(ar)
   {
      if(ar->p == p)
         break;
      ar2 = ar;
      ar = ar->Next;
   }
   if(ar == 0)
   {
     printf("Free on unregistered pointer in '%s'\n", Message? Message:"?");
     if(!Message) return;
     //WriteErrorMessage(0, "Free on unregistered pointer in '%s'\n", Message);
     return;
     FATAL();
   }
   if(ar == LastAR) LastAR = 0;
   if(ar2) ar2->Next = ar->Next;
     else  HTableAllocRemember[h] = ar->Next;
   ar->Next = FreeAllocRemember;
   FreeAllocRemember = ar;
/*   if(ar->n)
      fprintf(stderr, "Coucou %d in '%s'\n", ar->n, ar->Message);
   ar->n++;*/
}


/*******************************************************************************
*  Fonction publique: AllocCheck
*  Description:
*        Verifie qu'un pointeur a ete correctement alloue
*        (i.e. est different de zero)
*
*  Parametres (l/e):
*        - Pointeur a verifie
*        - Message : description du lieu d'appel
*
*  Retour: VOID
*
*  Remarques:
*
*  Auteur: Vincent PENNE
*  Date: Wed 28th August 1997
*******************************************************************************/

void * _AllocCheck ( void *ptr, int line, const char *file )
{
#ifdef ACHK
   if(ptr == 0)
   {
      sprintf(Message, "File '%s' line #%d", file, line);
      fprintf(stderr, "No more memory in function '%s'\n", Message);
      FATAL();
   }
   if(do_not_check_any_more) return ptr;
   sprintf(Message, "File '%s' line #%d", file, line);
   if(LastAR && ptr == LastAR->p)
   {
      fprintf(stderr, "Warning ! AllocCheck twice on same pointer in '%s'\n",
        Message);
      //AllocComment(ptr);
      return ptr;
   }
   if(FreeAllocRemember == 0)
   {
      int n;
      FreeAllocRemember = AllocRemember;
      for(n=0; n<MAX_ALLOC_REMEMBER-1; n++)
         AllocRemember[n].Next = AllocRemember+n+1;
      AllocRemember[n].Next = 0;
   }

   tALLOC_REMEMBER *ar = FreeAllocRemember;
   if(ar->Next)
   {
      unsigned char h = (((int)ptr)&255) ^ ((((int)ptr)>>8)&255)
                                       ^ ((((int)ptr)>>16)&255)
                                       ^ ((((int)ptr)>>24)&255);
      FreeAllocRemember = ar->Next;
      //strcpy(ar->Message, Message);
      ar->line = line;
      ar->file = file;
      ar->p = ptr;
      ar->Next = HTableAllocRemember[h];
//      ar->n=0;
      HTableAllocRemember[h] = ar;
   }
   else
   {
     if(!do_not_check_any_more)
     {
       fprintf(stderr, "Warning : Cannot remember any more allocs at '%s'\n", Message);
       do_not_check_any_more=1;
     }
      //      FATAL();
   }

   //LastMessage = ar->Message;
   LastAR = ar;
#endif
   return ptr;
}



/*******************************************************************************
*  Fonction publique: CheckUnfrees
*  Description:
*        Verifie que tous les pointeurs ont ete liberes
*
*  Auteur: Vincent PENNE
*  Date: Sat 01st November 1997
*******************************************************************************/


void CheckUnfrees ( )
{
#ifdef ACHK
  struct {
    const char * file;
    int line;
    int nb;
  } *l;
  int nl = 0;
  l = malloc(sizeof(*l)*1024);

   if(do_not_check_any_more) return;
   //   if(fespos) { printf("Alloc file position stack non-empty !\n"); }
   fflush(stdout);
   //do_not_check_any_more = TRUE;
   int n, m=0, sz=0;
   for(n=0; n<256; n++)
   {
      tALLOC_REMEMBER *ar = HTableAllocRemember[n];
      while(ar)
      {
         if(ar->file[0] != '!')
         {
	    int i;
            m++;
	    for (i=0; i<nl; i++)
	      if (!strcmp(l[i].file, ar->file) && l[i].line == ar->line)
		break;
	    if (i<nl) {
	      l[i].nb++;
	    } else {
	      l[i].nb = 1;
	      l[i].file = ar->file;
	      l[i].line = ar->line;
	      nl++;
	      //fprintf(stderr, "%s : %d \n", l[n].mes, l[n].nb);
	    }
         }
         ar = ar->Next;
      }
   }
   for (n=0; n<nl; n++) {
     fprintf(stderr, "%s (%d) : %d \n", l[n].file, l[n].line, l[n].nb);
   }
   if(m)
   {
      fprintf(stderr, "\n%d unfree pointers !!\n", m);
#ifdef _WINDOWS
      getchar();
#endif
   }
#endif

   free(l);
}

void FreeAll()
{
   int n;
   do_not_check_any_more = 1;
   for(n=0; n<256; n++)
   {
      tALLOC_REMEMBER *ar = HTableAllocRemember[n];
      while(ar)
      {
	//printf("Freeing pointer at '%s'\n", ar->Message);
	 free(ar->p);
         ar = ar->Next;
      }
   }
}







#define PKT_BUFSZ 1*1024*1024
static char pkt_buf[PKT_BUFSZ];
static int pkt_buf_pos;

void * av_pkt_malloc(size)
{
  char * res;
  if (size > PKT_BUFSZ/2)
    return 0;

  if (PKT_BUFSZ - pkt_buf_pos < size) {
    res = pkt_buf;
    pkt_buf_pos = size;
  } else {
    res = pkt_buf + pkt_buf_pos;
    pkt_buf_pos += size;
  }

  return res;
}

void av_pkt_free(void * ptr)
{
  if (ptr < pkt_buf || ptr >= pkt_buf+PKT_BUFSZ) {
    for (;;)
      printf("PKT_FREE on bad pointer !\n");
    av_free_(ptr, __FILE__, __LINE__);
  }
}







/* you can redefine av_malloc and av_free in your project to use your
   memory allocator. You do not need to suppress this file because the
   linker will do it automatically */

/** 
 * Memory allocation of size byte with alignment suitable for all
 * memory accesses (including vectors if available on the
 * CPU). av_malloc(0) must return a non NULL pointer.
 */
void *av_malloc_(unsigned int size, const char * file, int line)
{
    void *ptr;

    //printf("alloc '%s' #%d : %d\n", file, line, size);
/*     if (!size) { */
/*       printf("malloc null size '%s' #%d : %d\n", file, line, size); */
/*       size++; */
/*     } */

#ifdef MEMALIGN_HACK
    int diff;
    ptr = malloc(size+16+1);
    diff= ((-(int)ptr - 1)&15) + 1;
    ptr += diff;
    ((char*)ptr)[-1]= diff;
#elif defined (HAVE_MEMALIGN) 
    ptr = memalign(32,size);
#else
    ptr = malloc(size);
#endif

    if (ptr) {
      av_nballocs++;

      sprintf(Message, "File '%s' line #%d", file, line);
      _AllocCheck(ptr, line, file);
    }

    return ptr;
}

/**
 * av_realloc semantics (same as glibc): if ptr is NULL and size > 0,
 * identical to malloc(size). If size is zero, it is identical to
 * free(ptr) and NULL is returned.  
 */
void *av_realloc_(void *ptr, unsigned int size, const char * file, int line)
{
  void * res;

/*   if (!size) { */
/*     printf("realloc null size '%s' #%d : %d\n", file, line, size); */
/*     size++; */
/*   } */

  if (ptr >= pkt_buf && ptr < pkt_buf+PKT_BUFSZ) {
    for (;;)
      printf("av_realloc on PKT pointer ('%s' #%d) !\n", file, line);
  }

  if (ptr == NULL) {
    //printf("realloc with NULL ptr at '%s' line #%d\n", file, line);
    return av_malloc_(size, file, line);
  }

  //printf("realloc '%s' #%d : %d\n", file, line, size);

  if(ptr && file && !do_not_check_any_more)
  {
    unremember(ptr);
  }

#ifdef MEMALIGN_HACK
  {
    //FIXME this isnt aligned correctly though it probably isnt needed
    int diff= ptr ? ((char*)ptr)[-1] : 0;
    res = realloc(ptr - diff, size + diff) + diff;
  }
#else
  res = realloc(ptr, size);
#endif

  if(!do_not_check_any_more) {
    if(file)
      sprintf(Message, "File '%s' line #%d", file, line);
    else
      Message[0] = 0;

    _AllocCheck(res, line, file);
  }

  if (res && ptr == NULL)
      av_nballocs++;

  return res;
}

/* NOTE: ptr = NULL is explicetly allowed */
void av_free_(void *ptr, const char * file, int line)
{
  if (ptr >= pkt_buf && ptr < pkt_buf+PKT_BUFSZ) {
    for (;;)
      printf("av_free on PKT pointer ('%s' #%d) !\n", file, line);
  }


   if(ptr && file && !do_not_check_any_more)
   {
     if(file) sprintf(Message, "File '%s' line #%d", file, line);
     else Message[0] = 0;
     unremember(ptr);
   }

    /* XXX: this test should not be needed on most libcs */
  if (ptr) {
#ifdef MEMALIGN_HACK
        free(ptr - ((char*)ptr)[-1]);
#else
        free(ptr);
#endif

    av_nballocs--;
  }
}

