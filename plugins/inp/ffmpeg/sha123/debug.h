#ifndef _SHA123_DEBUG_H_
# define _SHA123_DEBUG_H_
# ifdef SHA123_DEBUG
#  if SHA123_DEBUG > 1
#   define SHA123_PARANO 1
#  endif
# define SHA123_BREAKPOINT(v) *(int*)1 = (v)
void sha123_debug(const char * fmt, ...);
# else /* #ifdef SHA123_DEBUG */
#  define SHA123_BREAKPOINT(v)
#  define sha123_debug(FMT, ...)
# endif /* #ifdef SHA123_DEBUG */
#endif /* #ifdef _SHA123_DEBUG_H_ */
