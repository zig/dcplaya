#if 0

#include <kos.h>

typedef unsigned int FILE;

FILE * fopen(const char *path, const char *mode)
{
    if (mode[0] == 'r')
	return (FILE *)fs_open(path, O_RDONLY);
    if (mode[0] == 'w')
	return (FILE *)fs_open(path, O_WRONLY);
    return 0;
}

/* int fprintf(FILE *stream, const char *fmt, ...)
{
    va_list args;
    int i;
    char buf[1024];

    if (stream <= 2) {
	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);
	
	printf("%s", buf);
	
	return i;
    } else
	return -1;
} */

int fclose(FILE *stream)
{
    fs_close((unsigned int)stream);
    return 0;
}

unsigned int fread(void *ptr, unsigned int size, unsigned int nmemb, FILE *stream)
{
    return fs_read((unsigned int)stream, ptr, size*nmemb)/size;
}

unsigned int fwrite(void *ptr, unsigned int size, unsigned int nmemb, FILE *stream)
{
    return fs_write((unsigned int)stream, ptr, size*nmemb)/size;
}

int fseek(FILE *stream, long offset, int whence)
{
    return fs_seek((unsigned int)stream, offset, whence);
}

void exit(int status)
{
    printf("fake exit() called - you must reset now\n");    
    while(1);
}

#endif
