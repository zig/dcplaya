/* FIFO */

#include <arch/spinlock.h>
#include <stdlib.h>

#include "dcplaya/config.h"
#include "sysdebug.h"

static spinlock_t fifo_mutex;
static int *fifo_buffer; /* FIFO buffer */
static int fifo_r; /* FIFO read index   */
static int fifo_w; /* FIFO write index  */
static int fifo_s; /* FIFO size (power of 2) - 1 */
static int fifo_k; /* FIFO bak-buffer index */
static int fifo_m; /* FIFO bak-buffer threshold */

/* used by playa.c to evaluate how many samples have been written */
int fifo_written = 0;

#define FIFO_BAK_MAX (1<<14)

#define FIFO_USED2(R,W,S) (((W)-(R))&(S))
#define FIFO_FREE2(R,W,S) (((R)-(W)+(S))&(S))

#define FIFO_USED FIFO_USED2(fifo_r,fifo_w,fifo_s)
#define FIFO_FREE FIFO_FREE2(fifo_k,fifo_w,fifo_s)

int fifo_init(int size)
{
  SDDEBUG("[%s] : size=%d\n", __FUNCTION__, size);
  /* Create mutex object */
  spinlock_init(&fifo_mutex);
  fifo_s = fifo_r = fifo_w = fifo_k = 0;
  fifo_buffer = (int *)malloc(size * sizeof(*fifo_buffer));
  if (fifo_buffer) {
    fifo_s = size - 1;
  }
  fifo_m = size >> 1;
  if (fifo_m > FIFO_BAK_MAX) {
    fifo_m = FIFO_BAK_MAX;
  }

  spinlock_unlock(&fifo_mutex);
  return -!fifo_buffer;
}

int fifo_resize(int size)
{
  int * b;
  SDDEBUG("[%s] : size=%d\n", __FUNCTION__, size); 
  spinlock_lock(&fifo_mutex);
  b = (int *)realloc(fifo_buffer, size * sizeof(*fifo_buffer));
  if (b) {
    fifo_buffer = b;
    fifo_s = size - 1;
    fifo_r &= fifo_s;
    fifo_w &= fifo_s;
    fifo_k &= fifo_s;
    fifo_m = size >> 1;
    if (fifo_m > FIFO_BAK_MAX) {
      fifo_m = FIFO_BAK_MAX;
    }
  }
  spinlock_unlock(&fifo_mutex);
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, fifo_s); 
  return fifo_s;
}

int fifo_start(void)
{
  spinlock_lock(&fifo_mutex);
  fifo_r = fifo_w = fifo_k = 0;
  spinlock_unlock(&fifo_mutex);
  return 0;
}

void fifo_stop(void)
{
  fifo_start();
}

int fifo_free(void)
{
  int v;
  spinlock_lock(&fifo_mutex);
  v = FIFO_FREE;
  spinlock_unlock(&fifo_mutex);
  return v;
}

int fifo_used(void)
{
  int v;
/*  spinlock_lock(&fifo_mutex); */
  v = FIFO_USED;
/*  spinlock_unlock(&fifo_mutex); */
  return v;
}

int fifo_bak(void)
{
  int v;
  spinlock_lock(&fifo_mutex);
  v = FIFO_USED2(fifo_k, fifo_r, fifo_s);
  spinlock_unlock(&fifo_mutex);
  return v;
}


int fifo_size(void)
{
  return fifo_s;
}


void fifo_read_lock(int *i1, int *n1, int *i2, int *n2)
{
  int n, fifo_u;
  
  spinlock_lock(&fifo_mutex);

  fifo_u = FIFO_USED;
  *i1 = fifo_r;
  *i2 = 0;
  n = fifo_s + 1 - fifo_r;
  if (n > fifo_u) {
    n = fifo_u;
  }
  *n1 = n;
  *n2 = fifo_u - n;
}

void fifo_write_lock(int *i1, int *n1, int *i2, int *n2)
{
  int fifo_f, n;
  
  spinlock_lock(&fifo_mutex);
  
  *i1 = fifo_w;
  *i2 = 0;
  fifo_f = FIFO_FREE;
  n = fifo_s + 1 - fifo_w;
  if (n > fifo_f) {
    n = fifo_f;
  }
  *n1 = n;
  *n2 = fifo_f - n;
}

void fifo_unlock()
{
  spinlock_unlock(&fifo_mutex);
}

void fifo_state(int *r, int *w, int *k)
{
  spinlock_lock(&fifo_mutex);
  *r = fifo_r;
  *w = fifo_w;
  *k = fifo_k;
  spinlock_unlock(&fifo_mutex);
}


static int fifo_read_any(int * buf, int r, int w, int n)
{
  int b, m;
  b = FIFO_USED2(r,w,fifo_s);
  if (!b) {
    return 0;
  }
  
  /* Max PCM to read */
  if (n > b) {
    n = b;
  }
  
  m = fifo_s + 1 - r;
  if (m > n) {
    m = n;
  }
  if (buf) {
    memcpy(buf, fifo_buffer+r, m<<2);
    buf += m;
    m = n - m;
    memcpy(buf, fifo_buffer, m<<2);
  }
  return n;
}

int fifo_readbak(int *buf, int n)
{
  int r,w,k;

  if (n <= 0) {
    return n;
  }
  
  /* Get pseudo-locked state */
  fifo_state(&r,&w,&k);
  n = fifo_read_any(buf,k,r,n);
  if (n) {
    /* Advance bak pointer */
    spinlock_lock(&fifo_mutex);
    fifo_k = (fifo_k + n) & fifo_s;
    spinlock_unlock(&fifo_mutex);
  }
  
  return n;
}

int fifo_read(int *buf, int n)
{
  int r,w,k;

  if (n <= 0) {
    return n;
  }
  
  /* Get pseudo-locked state */
  //fifo_state(&r,&w,&k);
  r = fifo_r;
  w = fifo_w;
  k = fifo_k;
  n = fifo_read_any(buf,r,w,n);
  if (n) {
    int bak_size;

    /* Advance read pointer */
    //spinlock_lock(&fifo_mutex);
    fifo_r = (fifo_r + n) & fifo_s;

    bak_size = FIFO_USED2(fifo_k, fifo_r, fifo_s);
    if (bak_size > fifo_m) {
/*       SDDEBUG("fifo bak-size : %d > %d\n", bak_size, fifo_m); */
      bak_size -= fifo_m;
      fifo_k = (fifo_k + bak_size) & fifo_s;
    }

    //spinlock_unlock(&fifo_mutex);
  }
  
  return n;
}

typedef const void * (*fifo_copy_f)(void *d, const void *v, int n);

int fifo_write_any(const void * buf, int n, fifo_copy_f copy)
{
  int r,w,f,k;
  int m;

  if (n <= 0) {
    return n;
  }
  
  /* Get pseudo-locked state */
  fifo_state(&r,&w,&k);
  f = FIFO_FREE2(k,w,fifo_s);
  if (!f) {
    return 0;
  }
  
  /* Max PCM to write */
  if (n > f) {
    n = f;
  }
  
  m = fifo_s + 1 - w;
  if (m > n) {
    m = n;
  }
  
  buf = copy(fifo_buffer+w, buf, m);
  m = n - m;
  copy(fifo_buffer, buf, m);

  /* Advance write pointer */
  spinlock_lock(&fifo_mutex);
  fifo_w = (fifo_w + n) & fifo_s;
  spinlock_unlock(&fifo_mutex);
  
  return n;
}


static const void * copy_mono(int *d, const unsigned short *s, int n)
{
  fifo_written += n;

  while (n--) {
    int v = *s++;
    *d++ = v | (v<<16);
  }
  return s;
}

static const void * copy_stereo(int *d, const int *s, int n)
{
  fifo_written += n;

  while (n--) {
    *d++ = *s++;
  }
  return s;
}

int fifo_write(const int *buf, int n)
{
  return fifo_write_any(buf,n,(fifo_copy_f)copy_stereo);
}

int fifo_write_mono(const short *buf, int n)
{
  return fifo_write_any(buf,n,(fifo_copy_f)copy_mono);
}
