#ifndef _SHATRANSLATORBLITTER_H_
#define _SHATRANSLATORBLITTER_H_


#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/// Convert copy single 8-bit channel to four 8-bit channels.
void GREY8toARGB32(void * dst, const void * src, int n);
void RGB565toARGB32(void * dst, const void * src, int n);
void ARGB1555toARGB32(void * dst, const void * src, int n);
void ARGB4444toARGB32(void * dst, const void * src, int n);
void RGB24toARGB32(void * dst, const void * src, int n);
void ARGB32toARGB32(void * dst, const void * src, int n);

void ARGB32toARGB1555(void * dst, const void * src, int n);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif //#define _SHATRANSLATORBLITTER_H_
