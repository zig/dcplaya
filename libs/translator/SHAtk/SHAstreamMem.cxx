/**
 * @ingroup SHAtk
 * @file    SHAstreamMem.cxx
 * @brief   sashipa toolkit memory stream class implementation
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/05/14
 */
/* --------------------------------------------------------------------------
 * $Id: SHAstreamMem.cxx,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 * --------------------------------------------------------------------------
 */

#include "SHAtk/SHAstreamMem.h"

#include <string.h>

SHAstreamMem::SHAstreamMem()
{
}

int SHAstreamMem::Open(char *buffer, unsigned int size, int access)
{
  buf = buffer;
  pos = buf;
  end = buf + size;
  right = access & 3;
  eof = 0;
  return 0;
}

SHAstreamMem::~SHAstreamMem()
{
  Close();
}

int SHAstreamMem::Close(void)
{
  right = 0;
  buf = 0;
  pos = 0;
  end = 0;
  eof = 0;
  return 0;
}

int SHAstreamMem::Read(void *data, SHAstreamPos n)
{
  if (! (right & 1)) {
    return ~n;
  }

  int l = end - pos;
  int w = n;
  if (l < n) {
    w = l;
    eof = 1;
  }

  memcpy(data, pos, w);
  pos += w;
  //  return ~(n - w);
  return n - w;
}

int SHAstreamMem::Write(const void *data, SHAstreamPos n)
{
  if (! (right & 2)) {
    return ~n;
  }

  int l = end - pos;
  int w = n;
  if (l < n) {
    w = l;
    eof = 1;
  }

  memcpy(pos, data, w);
  pos += w;
  //  return ~(n - w);
  return n - w;
}

int SHAstreamMem::Seek(SHAstreamPos offset, SHAstreamSeek_e origin)
{
  char * p;

  switch(origin) {
  case CUR:
    p = pos;
    break;
  case SET:
    p = buf;
    break;
  case END:
    p = end;
    break;
  default:
    return -1;
  }

  p += offset;
  if (p > end) {
    pos = end;
    return -1;
  } else if (p < buf) {
    pos = buf;
    return -1;
  }
  pos = p;
  return 0;
}

SHAstreamPos SHAstreamMem::Tell(void)
{
  return pos - buf;
}

int SHAstreamMem::Flush(void)
{
  return 0;
}

int SHAstreamMem::IsEOF(void)
{
  return eof;
}

