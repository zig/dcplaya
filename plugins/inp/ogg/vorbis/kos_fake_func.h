#if 0
#ifndef __KOS_FAKE_FUNC__
#define __KOS_FAKE_FUNC__

#define stderr 2

typedef unsigned int FILE;

FILE * fopen(const char *path, const char *mode);
/* int fprintf(FILE *stream, const char *fmt, ...); */
#define fprintf(stream, fmt, ARGS...) printf(fmt, ## ARGS)
int fclose(FILE *stream);
unsigned int fread(void *ptr, unsigned int size, unsigned int nmemb, FILE *stream);
unsigned int fwrite(void *ptr, unsigned int size, unsigned int nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
int sprintf(char *buf, const char *fmt, ...) ;
void exit(int status);

#endif

#endif
