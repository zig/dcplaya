#ifndef _FILE_WRAPPER_H_
#define _FILE_WRAPPER_H_

typedef int FILE;
typedef int fpos_t; 

int fread(void *ptr,int size, int nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
long ftell(FILE *stream);
int flen(FILE *stream);

int fgetpos(FILE *stream, fpos_t *pos);
int rewind(FILE *stream);
int fsetpos(FILE *stream, fpos_t *pos);

void clearerr( FILE *stream);

#ifndef SEEK_SET
# define SEEK_SET 0
#endif

#ifndef SEEK_CUR
# define SEEK_CUR 1
#endif

#ifndef SEEK_END
# define SEEK_END 2
#endif

#endif

