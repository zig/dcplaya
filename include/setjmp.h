

typedef char jmp_buf[1024];


int longjmp(jmp_buf buf, int what_is_that);
int setjmp(jmp_buf buf);

