#ifndef _PCM_BUFFER_H_
#define _PCM_BUFFER_H_

#define PCM_BUFFER_SIZE pcm_buffer_size
#define BS_SIZE         bs_buffer_size

extern short * pcm_buffer;
extern int pcm_buffer_size;

extern char  * bs_buffer;
extern int bs_buffer_size;

int pcm_buffer_init(int pcm_size, int bs_size);

#endif
