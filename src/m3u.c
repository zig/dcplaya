/* .m3u library
 *
 * COPYRIGHT (c) 2002 ben(jamin) gerard <ben@sashipa.com>
 */

#include "m3u.h"

/* The current driver */
static M3Udriver_t driver;

/** DRIVER INTERFACE */
static void * M3Umalloc(int bytes)
{
  return !driver.malloc ? 0 : driver.malloc(driver.cookie, bytes);
}

static void M3Ufree(void * data)
{
  if(driver.free) {
    driver.free(driver.cookie, data);
  }
}

static int M3Uread(void * data, int bytes)
{
  return !driver.read ? 0 : driver.read(driver.cookie, data, bytes);
}

static int M3UreadChar(void)
{
  int l;
  char c;
  l = M3Uread(&c,1);
  if (l<0) {
    return -1;  /* error */
  }
  if (l==0) {
    return -2;  /* eof */
  }
  return (int)c & 255;
}



/** ENTRY INTERFACE */
static void M3UentryClean(M3Uentry_t * e)
{
  e->name = 0;
  e->path = 0;
  e->time = 0;
}

static void M3UentryFree(M3Uentry_t * e)
{
  if (e->name) {
    M3Ufree(e->name);
    e->name = 0;
  }
  if (e->path) {
    M3Ufree(e->path);
    e->path = 0;
  }
}

/** LIST INTERFACE */
static int M3UlistAlloc(M3Ulist_t * l)
{
  if (l->n < l->sz) {
    return l->n++;
  }
  return -1;
}

static M3Ulist_t * M3UlistCreate(int sz)
{
  M3Ulist_t *l;
  int len;

  if (sz <= 0) {
    return 0;
  }
  len = sizeof(M3Ulist_t) + sizeof(M3Uentry_t) * (sz-1);
  l = M3Umalloc(len);
  if (l) {
    l->n = 0;
    l->sz = sz;
  }
  return l;
}

static void M3UlistKill(M3Ulist_t *l)
{
  int i;
  if (!l) return;
  for (i=0; i<l->n; ++i) {
    M3UentryFree(l->entry+i);
  }
  M3Ufree(l);
}

static int readline(char *b, int max)
{
  char *start = b;
  int c;


  while ((c = M3UreadChar())>0 && c != '\n' && max--) {
    *b++ = c;
  }
  *b = 0;
  if (c == -1 || !max) {
    return -1;
  }
  return b-start;
}

static int find_eol(char *line, int len)
{
  int i;
  for (i=0; i<len && line[i] && line[i] != '\n'; ++i)
    ;
  return i;
}

static char * findstr(char *where, char * what)
{
  int b;

  for (b = *what++; b && *where++==b; b = *what++)
    ;
  return (b == 0) ? where : 0;
}

static char * findtime(char *l, int *time)
{
  char * l2 = l;
  int r = 0, c;
  while (c = *l, (c<='9' && c>='0')) {
    r = r * 10 + c - '0';
    ++l;
  }

  if (r==0 || (c && c!=',')) {
    *time = 0;
    l = l2;
  } else {
    /* Found a valid time */
    *time = r;
    l += (c != 0);  /* Skip final ',' if any */
  }
  return l;
}

static int getextend(char *l, int eol, M3Uentry_t * e)
{
  int ret = 0;
  char * line = l;

  l = findstr(l,"#EXTINF:");
  if (l) {
    char * l2;
    l = findtime(l2=l, &e->time);
    ret = (l2 != l);
    if (*l) {
      int len;

      len = eol - (l-line);
      e->name = M3Umalloc(len+1);
      if (e->name) {
        int i;

        for (i=0; i<len; ++i, ++l) {
          e->name[i] = *l;
        }
        e->name[i] = 0;
        ret |= 2;
      }
    }
  }
  return ret;
}

static M3Ulist_t * process_r(char *line, int max, int n)
{
  M3Uentry_t e;

  M3UentryClean(&e);

  /* Read new data */
  while (1) {
    int l;

    l = readline(line, max);
    if (l < 0) {
      return 0;
    }
    if (l == 0) {
      /* No more line... We are finished here */
      M3UentryFree(&e);
      return M3UlistCreate(n);
    }

    if (line[0] == '#') {
      getextend(line, l, &e);
    } else {
      M3Ulist_t * list;

      e.path = M3Umalloc(l+1);
      if (!e.path) {
        break;
      } else {
        int i;

        for (i = 0; i < l; ++i) {
	  int c = line[i];
	  if (c == '\\') {
	    c = '/';
	  }
          e.path[i] = c;
        }
        e.path[i] = 0;
      }
      list = process_r(line, max, n+1);
      if (list) {
        list->n++;    /* $$$ For verify */
        list->entry[n] = e;
      }
      return list;
    }
  }

  M3UentryFree(&e);
  return 0;
}


M3Ulist_t * M3Uprocess(void)
{
  M3Ulist_t * list;
  char line[1024];

  list = process_r(line, sizeof(line), 0);
  if (list) {
    list->n = list->sz;
  }
  return list;
}

int M3Udriver(M3Udriver_t * p_driver)
{
  /* Fill user driver and exit */
  if (p_driver && !p_driver->free && !p_driver->malloc && ! p_driver->read) {
    *p_driver = driver;
    return 0;
  }

  /* Install new driver */
  if (p_driver) {
    driver = *p_driver;
  }
  if (!driver.free || !driver.malloc || !driver.read) {
    return -1;
  }

  return 0;
}

void M3Ukill(M3Ulist_t * l)
{
  M3UlistKill(l);
}
