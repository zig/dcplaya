
#ifdef MALLOC_DEBUG

#include <malloc.h>
#include <stdio.h>

#undef memalign
#undef valloc
#undef pvalloc
#undef free
#undef malloc
#undef realloc
#undef calloc



#define ACHK
#define MAX_ALLOC_REMEMBER 1000

#define FALSE 0
#define TRUE 1
typedef struct tALLOC_REMEMBER tALLOC_REMEMBER;
struct tALLOC_REMEMBER {
   tALLOC_REMEMBER *Next;
   void *p;
   char Message[128];
//   int  n;
};

static tALLOC_REMEMBER AllocRemember[MAX_ALLOC_REMEMBER];
static tALLOC_REMEMBER *HTableAllocRemember[256], *FreeAllocRemember;
static int do_not_check_any_more = FALSE;

static void *LastChecked;
static char *LastMessage = "SYSTEM ALLOC";
static tALLOC_REMEMBER *LastAR;

static char Message[256];

static void FATAL()
{
   printf("\n**** FATAL ERROR - MUST EXIT IMMEDIATLY.\n");
   exit(1);
}

static void unremember( void *p )
{
   if(FreeAllocRemember == 0)
   {
     printf( "Free on unregistered pointer in '%s'\n", Message? Message:"?");
     if(!Message) return;
     //WriteErrorMessage(0, "Free on unregistered pointer in '%s'\n", Message);
     //return;
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
     //return;
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
      strcpy(ar->Message, Message);
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
       CheckUnfrees();
       do_not_check_any_more=1;
       FATAL();
     }
   }

   LastMessage = ar->Message;
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
    char *mes;
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
         if(ar->Message[0] != '!')
         {
	    int i;
            m++;
	    for (i=0; i<nl; i++)
	      if (!strcmp(l[i].mes, ar->Message))
		break;
	    if (i<nl) {
	      l[i].nb++;
	    } else {
	      l[i].nb = 1;
	      l[i].mes = ar->Message;
	      nl++;
	      //fprintf(stderr, "%s : %d \n", l[n].mes, l[n].nb);
	    }
         }
         ar = ar->Next;
      }
   }
   for (n=0; n<nl; n++) {
     fprintf(stderr, "%s : %d \n", l[n].mes, l[n].nb);
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


//#define VERBOSE

void *malloc_(unsigned int size, const char * file, int line)
{
    void *ptr;

#ifdef VERBOSE
    printf("malloc '%s' #%d : %d\n", file, line, size);
#endif

    ptr = malloc(size);

    if (ptr) {
      sprintf(Message, "File '%s' line #%d", file, line);
      _AllocCheck(ptr, line, file);
    }

    return ptr;
}

void *valloc_(unsigned int size, const char * file, int line)
{
    void *ptr;

#ifdef VERBOSE
    printf("valloc '%s' #%d : %d\n", file, line, size);
#endif

    ptr = valloc(size);

    if (ptr) {
      sprintf(Message, "File '%s' line #%d", file, line);
      _AllocCheck(ptr, line, file);
    }

    return ptr;
}

void *pvalloc_(unsigned int size, const char * file, int line)
{
    void *ptr;

#ifdef VERBOSE
    printf("pvalloc '%s' #%d : %d\n", file, line, size);
#endif

    ptr = pvalloc(size);

    if (ptr) {
      sprintf(Message, "File '%s' line #%d", file, line);
      _AllocCheck(ptr, line, file);
    }

    return ptr;
}

void *memalign_(unsigned int size1, unsigned int size2, const char * file, int line)
{
    void *ptr;

#ifdef VERBOSE
    printf("memalign '%s' #%d : %d, %d\n", file, line, size1, size2);
#endif

    ptr = memalign(size1, size2);

    if (ptr) {
      sprintf(Message, "File '%s' line #%d", file, line);
      _AllocCheck(ptr, line, file);
    }

    return ptr;
}

void *calloc_(unsigned int size1, unsigned int size2, const char * file, int line)
{
    void *ptr;

#ifdef VERBOSE
    printf("calloc '%s' #%d : %d, %d\n", file, line, size1, size2);
#endif

    ptr = calloc(size1, size2);

    if (ptr) {
      sprintf(Message, "File '%s' line #%d", file, line);
      _AllocCheck(ptr, line, file);
    }

    return ptr;
}

void *realloc_(void *ptr, unsigned int size, const char * file, int line)
{
  void * res;

  if (ptr == NULL) {
    //printf("realloc with NULL ptr at '%s' line #%d\n", file, line);
    return malloc_(size, file, line);
  }

#ifdef VERBOSE
  printf("realloc '%s' #%d : %x, %d\n", file, line, ptr, size);
#endif

  if(ptr && file && !do_not_check_any_more)
  {
    unremember(ptr);
  }

  res = realloc(ptr, size);

  if(!do_not_check_any_more) {
    if(file)
      sprintf(Message, "File '%s' line #%d", file, line);
    else
      Message[0] = 0;

    _AllocCheck(res, line, file);
  }

  return res;
}

void free_(void *ptr, const char * file, int line)
{
#ifdef VERBOSE
  printf("free '%s' #%d : %x\n", file, line, ptr);
#endif
   if(ptr && file && !do_not_check_any_more)
   {
     if(file) sprintf(Message, "File '%s' line #%d", file, line);
     else Message[0] = 0;
     unremember(ptr);
   }

  if (ptr) {
    free(ptr);
  }
}

#endif
