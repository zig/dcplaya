#ifndef _M3U_H_
#define _M3U_H_

/** Driver */
typedef void *  (*M3Umalloc_f)(void *cookie, int bytes);
typedef void    (*M3Ufree_f)(void *cookie, void *data);
typedef int     (*M3Uread_f)(void *cookie, char *dst, int bytes);
typedef struct
{
  void * cookie;
  M3Umalloc_f malloc;
  M3Ufree_f   free;
  M3Uread_f   read;
} M3Udriver_t;

/** Entry */
typedef struct {
  char *path;
  char *name;
  int  time;
} M3Uentry_t;

/** List */
typedef struct
{
  int sz;
  int n;
  M3Uentry_t entry[1];
} M3Ulist_t;

int M3Udriver(M3Udriver_t * p_driver);
M3Ulist_t * M3Uprocess(void);
void M3Ukill(M3Ulist_t * l);

#endif /* #define _M3U_H_ */
