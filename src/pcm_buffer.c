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

  if (!pcm_size) {
    pcm_size = _PCM_BUFFER_SIZE;
  }
  if (pcm_size > pcm_buffer_size) {
    new = realloc(pcm_buffer, pcm_size * sizeof(*pcm_buffer));
    if (new) {
      pcm_buffer = new;
      pcm_buffer_size = pcm_size;
    }
  }

  if (!bs_size) {
    bs_size = _BS_BUFFER_SIZE;
  }
  if (bs_size > bs_buffer_size) {
    new = realloc(bs_buffer, bs_size * sizeof(*bs_buffer));
    if (new) {
      bs_buffer = new;
      bs_buffer_size = bs_size;
    }
  }

  /* Got en error if cuurent size is less than requested */
  return -(pcm_buffer_size<pcm_size || bs_buffer_size<bs_size);
}
