/* FIFO */

#include <kos.h>
#include "sysdebug.h"

static spinlock_t fifo_mutex;
static int *fifo_buffer; /* power of 2 */
static int fifo_r;
static int fifo_w;
static int fifo_b;
static int fifo_s;

int fifo_init(int size)
{
  SDDEBUG("[%s] : size=%d\n", __FUNCTION__, size); 
  /* Create mutex object */
  spinlock_init(&fifo_mutex);
  fifo_s = fifo_r = fifo_w = fifo_b = 0;
  fifo_buffer = (int *)malloc(size * sizeof(*fifo_buffer));
  if (fifo_buffer) {
	fifo_s = size;
  }
  spinlock_unlock(&fifo_mutex);
  return fifo_buffer ? fifo_s : -1;
}

int fifo_resize(int size)
{
  int * b;
  SDDEBUG("[%s] : size=%d\n", __FUNCTION__, size); 
  spinlock_lock(&fifo_mutex);
  b = (int *)realloc(fifo_buffer, size * sizeof(*fifo_buffer));
  if (b) {
	fifo_buffer = b;
	fifo_s = size;
	fifo_b &= (fifo_s-1);
	fifo_r &= (fifo_s-1);
	fifo_w &= (fifo_s-1);
  }
  spinlock_unlock(&fifo_mutex);
  SDDEBUG("[%s] := [%d]\n", __FUNCTION__, fifo_s); 
  return fifo_s;
}

int fifo_start(void)
{
  spinlock_lock(&fifo_mutex);
  fifo_r = fifo_w = fifo_b = 0;
  spinlock_unlock(&fifo_mutex);
  return 0;
}

void fifo_stop()
{
  fifo_start();
}

int fifo_free()
{
  return fifo_s - fifo_b;
}

int fifo_used()
{
  return fifo_b;
}

int fifo_size()
{
  return fifo_s;
}


void fifo_read_lock(int *i1, int *n1, int *i2, int *n2)
{
  int n;
  
  spinlock_lock(&fifo_mutex);

  *i1 = fifo_r;
  *i2 = 0;
  n = fifo_s - fifo_r;
  if (n > fifo_b) {
    n = fifo_b;
  }
  *n1 = n;
  *n2 = fifo_b - n;
}

void fifo_write_lock(int *i1, int *n1, int *i2, int *n2)
{
  int fifo_f, n;
  
  spinlock_lock(&fifo_mutex);
  
  *i1 = fifo_w;
  *i2 = 0;
  fifo_f = fifo_s - fifo_b;
  n = fifo_s - fifo_w;
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

/* Call it when PCM have been written into fifo (must be locked) */
static void fifo_written(int n)
{
  fifo_b += n;
  fifo_w = (fifo_w + n) & (fifo_s-1);
}

void fifo_state(int *r, int *w, int *b)
{
  spinlock_lock(&fifo_mutex);
  *r = fifo_r;
  *w = fifo_w;
  *b = fifo_b;
  spinlock_unlock(&fifo_mutex);
}

int fifo_read(int *buf, int n)
{
  int r,w,b;
  int m;

//$$$
//  return n;

  if (n <= 0) {
    return n;
  }
  
  /* Get pseudo-locked state */
  fifo_state(&r,&w,&b);
  
  /* Max PCM to read */
  if (n > b) {
    n = b;
  }
  
  m = fifo_s - r;
  if (m > n) {
    m = n;
  }
  memcpy(buf, fifo_buffer+r, m<<2);
  buf += m;
  m = n - m;
  memcpy(buf, fifo_buffer, m<<2);

  /* Advance read pointer */
  spinlock_lock(&fifo_mutex);
  fifo_b -= n;
  fifo_r = (fifo_r + n) & (fifo_s-1);
  spinlock_unlock(&fifo_mutex);
  
  return n;
}

int fifo_write(const int *buf, int n)
{
  int r,w,b,f;
  int m;

//$$$
//  return n;
  
  if (n <= 0) {
    return n;
  }
  
  /* Get pseudo-locked state */
  fifo_state(&r,&w,&b);
  f = fifo_s - b;
  
//  dbglog(DBG_DEBUG, "b=%d n=%d f=%d\n",b, n, f);
  
  /* Max PCM to write */
  if (n > f) {
    n = f;
  }
  
  if (n) {
    m = fifo_s - w;
    if (m > n) {
      m = n;
    }
  
    memcpy(fifo_buffer+w, buf, m<<2);
    buf += m;
    m = n - m;
    memcpy(fifo_buffer, buf, m<<2);

    /* Advance read pointer */
    spinlock_lock(&fifo_mutex);
    fifo_b += n;
    fifo_w = (fifo_w + n) & (fifo_s-1);
    spinlock_unlock(&fifo_mutex);
  }
  
  return n;
}


static void CopyMonoToStereo(int *d, const unsigned short *s, int n)
{
  while (n--) {
    int v = *s++;
    *d++ = v | (v<<16);
  }
}

int fifo_write_mono(const short *buf, int n)
{
  int r,w,b,f;
  int m;
  
  
  if (n <= 0) {
    return n;
  }
  
  /* Get pseudo-locked state */
  fifo_state(&r,&w,&b);
  f = fifo_s - b;
  
  /* Max PCM to write */
  if (n > f) {
    n = f;
  }
  
  if (n) {
    m = fifo_s - w;
    if (m > n) {
      m = n;
    }
    CopyMonoToStereo(fifo_buffer+w, buf, m);
    buf += m;
    m = n - m;
    CopyMonoToStereo(fifo_buffer, buf, m);

    /* Advance read pointer */
    spinlock_lock(&fifo_mutex);
    fifo_b += n;
    fifo_w = (fifo_w + n) & (fifo_s-1);
    spinlock_unlock(&fifo_mutex);
  }
  
  return n;
}

