/* $Id: file_wrapper.h,v 1.3 2002-09-14 00:47:13 zig Exp $ */

#ifndef _FILE_WRAPPER_H_
#define _FILE_WRAPPER_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


typedef int FILE;
typedef int fpos_t; 

int fread(void *ptr,int size, int nmemb, FILE *stream);
int fwrite(const void *ptr,int size, int nmemb, FILE *stream);
int fseek(FILE *stream, long offset, int whence);
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);
long ftell(FILE *stream);
int flen(FILE *stream);

int fgetpos(FILE *stream, fpos_t *pos);
int rewind(FILE *stream);
int fsetpos(FILE *stream, fpos_t *pos);

int feof(FILE *stream);

char *fgets(char *s, int size, FILE *stream);

int fgetc(FILE *f);

int fflush(FILE *stream);

void clearerr( FILE *stream);

int ungetc(int c, FILE *stream);

#ifndef SEEK_SET
# define SEEK_SET 0
#endif

#ifndef SEEK_CUR
# define SEEK_CUR 1
#endif

#ifndef SEEK_END
# define SEEK_END 2
#endif

#ifndef EOF
# define EOF -1
#endif

DCPLAYA_EXTERN_C_END

#endif /*#ifndef _FILE_WRAPPER_H_ */

