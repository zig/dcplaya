#include "SHAtranslator/SHAtranslatorBlitter.h"
#include "SHAsys/SHAsysTypes.h"
#include "SHAsys/SHAsysPeek.h"

#include <string.h>

void GREY8toARGB32(void * dst, const void * src, int n)
{
  while (n--) {
    int v = *(SHAuint8 *)src;
    v += v<<8;
    v += v<<16;
    *(SHAint32 *)dst = v;
    src = (void *)((char *)src+1);
    dst = (void *)((char *)dst+4);
  }
}

void RGB565toARGB32(void * dst, const void * src, int n)
{
  while (n--) {
    int v = *(SHAuint16 *)src;
    int x;
    int r;

    // Alpha
    r = 0;
    // Red
    x = (v>>11) & 0x1F;
    x <<= 3;
    x |= x >> 5;
    r += x << 16;
    // Green
    x = (v>>5) & 0x3F;
    x <<= 2;
    x |= x >> 6;
    r += x << 8;
    // Blue
    v &= 0x0001F;
    v <<= 3;
    v |= v>>5;
    r += v;

    *(SHAint32 *)dst = r;

    src = (void *)((char *)src+2);
    dst = (void *)((char *)dst+4);
  }
}


void ARGB1555toARGB32(void * dst, const void * src, int n)
{
  while (n--) {
    int v = *(SHAsint16 *)src;
    int x;
    int r;

    // Alpha
    r = (v>>7) & 0xFF000000;
    // Red
    x = (v>>10) & 0x1F;
    x <<= 3;
    x |= x >> 5;
    r += x << 16;
    // Green
    x = (v>>5) & 0x1F;
    x <<= 3;
    x |= x>>5;
    r += x<<8;
    // Blue
    v &= 0x0001F;
    v <<= 3;
    v |= v>>5;
    r += v;

    *(SHAint32 *)dst = r;

    src = (void *)((char *)src+2);
    dst = (void *)((char *)dst+4);
  }
}

void ARGB32toARGB1555(void * dst, const void * src, int n)
{
  while (n--) {
    int v = *(SHAsint32 *)src;
    int r;

    // Alpha
    r = (v>>(24+7-15)) & 0x8000;
    // Red
    r |= (v>>(16+3-10)) & (0x1F<<10);
    // Green
    r |= (v>>(8+3-5)) & (0x1F<<5);
    // Blue
    r |= (v>>3) & 0x1f;
    *(SHAuint16 *)dst = r;
    src = (void *)((char *)src+4);
    dst = (void *)((char *)dst+2);
  }
}

void ARGB4444toARGB32(void * dst, const void * src, int n)
{
  while (n--) {
    int v = *(SHAuint16 *)src;
    int x;
    int r;

    // Alpha
    r = (v & 0xF000) << 12;
    r += r << 4;
    // Red
    x = v & 0x0F00;
    x <<= 8;
    r += (x + (x<<4));
    // Green
    x = v & 0x00F0;
    x <<= 4;
    r += (x + (x<<4));
    // Blue
    v &= 0x000F;
    r += (v + (v<<4));

    *(SHAint32 *)dst = r;

    src = (void *)((char *)src+2);
    dst = (void *)((char *)dst+4);
  }
}

void RGB24toARGB32(void * dst, const void * src, int n)
{
  while (n--) {
    *(SHAuint32 *)dst = SHApeekU24(src);
    src = (void *)((char *)src+3);
    dst = (void *)((char *)dst+4);
  }
}

void ARGB32toARGB32(void * dst, const void * src, int n)
{
  memcpy(dst, src, 4*n);
}
