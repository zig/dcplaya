/* $Id: extern_def.h,v 1.1 2002-09-06 23:16:09 ben Exp $ */

#ifndef _EXTERN_DEF_H_
#define _EXTERN_DEF_H_

#if defined(__cplusplus) || defined(c_plusplus)
# define DCPLAYA_EXTERN_C extern "C"
# define DCPLAYA_EXTERN_C_START extern "C" {
# define DCPLAYA_EXTERN_C_END   }
#else
# define DCPLAYA_EXTERN_C extern
# define DCPLAYA_EXTERN_C_START
# define DCPLAYA_EXTERN_C_END
#endif

#endif /* #define _EXTERN_DEF_H */
