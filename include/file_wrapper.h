/* $Id: file_wrapper.h,v 1.2 2002-09-06 23:16:09 ben Exp $ */

#ifndef _FILE_WRAPPER_H_
#define _FILE_WRAPPER_H_


#include "extern_def.h"

DCPLAYA_EXTERN_C_START


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

int fgetc(FILE *f);

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

DCPLAYA_EXTERN_C_END

#endif /*#ifndef _FILE_WRAPPER_H_ */

