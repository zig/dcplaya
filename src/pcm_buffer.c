#include <stdlib.h>
#include "pcm_buffer.h"

/* Default sizes */
#define _PCM_BUFFER_SIZE (1<<15)
#define _BS_BUFFER_SIZE  (4*1024)

/* Global pcm buffer */
short * pcm_buffer = 0;
int pcm_buffer_size = 0;

/* Global bitstream buffer */
char * bs_buffer = 0;
int bs_buffer_size = 0;

int pcm_buffer_init(int pcm_size, int bs_size)
{
  void *new;

  /* Doing PCM stuff */
  if (pcm_size == -1) {
    pcm_size = _PCM_BUFFER_SIZE;
  } else if (pcm_size == -2) {
    pcm_size = pcm_buffer_size;
  }

  if (!pcm_size) {
    if (pcm_buffer) {
      free(pcm_buffer);
      pcm_buffer = 0;
    }
    pcm_buffer_size = 0;
  } else {
    if (pcm_size > pcm_buffer_size) {
      new = realloc(pcm_buffer, pcm_size * sizeof(*pcm_buffer));
      if (new) {
	pcm_buffer = new;
	pcm_buffer_size = pcm_size;
      }
    }
  }

  /* Doing BS stuff */
  if (bs_size == -1) {
    bs_size = _BS_BUFFER_SIZE;
  } else if (bs_size == -2) {
    bs_size = bs_buffer_size;
  }

  if (!bs_size) {
    if (bs_buffer) {
      free(bs_buffer);
      bs_buffer = 0;
    }
    bs_buffer_size = 0;
  } else {
    if (bs_size > bs_buffer_size) {
      new = realloc(bs_buffer, bs_size * sizeof(*bs_buffer));
      if (new) {
	bs_buffer = new;
	bs_buffer_size = bs_size;
      }
    }
  }

  /* Got en error if cuurent size is less than requested */
  return -(pcm_buffer_size<pcm_size || bs_buffer_size<bs_size);
}

void pcm_buffer_shutdown(void)
{
  pcm_buffer_init(0,0);
}
