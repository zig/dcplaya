/**
 * @ingroup SHAtk
 * @file    SHAstreamFile.cxx
 * @brief   sashipa toolkit FILE stream class implementation
 * @author  BeN(jamin) Gerard <ben@sashipa.com>
 * @date    2001/05/14
 */
/* --------------------------------------------------------------------------
 * $Id: SHAstreamFile.cxx,v 1.2 2002-10-05 09:43:58 benjihan Exp $
 * --------------------------------------------------------------------------
 */

#include "SHAtk/SHAstreamFile.h"
#include <stdio.h>

SHAstreamFile::SHAstreamFile()
{
  f = 0;
}

SHAstreamFile::~SHAstreamFile()
{
  Close();
}

int SHAstreamFile::Open(const char *name, const char *mode)
{
  if (f) {
    return -1;
  }
  f = fopen(name, mode);
  if (f) {
    right = 0;
    while(1) {
      switch(*mode++) {
      case 0:
        return 0;
      case 'r':
        right |= 1;
        break;
      case 'w': case 'a':
        right |= 2;
        break;
      case '+':
        right |= 3;
        break;
      }
    }
  }
  return -1;
}

int SHAstreamFile::Open(void *file, int access)
{
  if (f) {
    return -1;
  }
  f = file;
  right = access;
  return 0;
}

int SHAstreamFile::Close(void)
{
  int err = 0;

  if (f) {
    err = fclose((FILE *)f);
    f = 0;
  }
  right = 0;
  return err;
}

int SHAstreamFile::Read(void *data, SHAstreamPos n)
{
  int err = -1;
  if (f) {
    err = fread(data, 1 , n, (FILE *)f);
  }

  if (err < 0) {
    return ~n;
  } else {
    return n - err;
  }
}

int SHAstreamFile::Write(const void *data, SHAstreamPos n)
{
  int err = -1;
  if (f) {
    err = fwrite(data, 1 , n, (FILE *)f);
  }

  if (err < 0) {
    return ~n;
  } else {
    return n - err;
  }
}

int SHAstreamFile::Seek(SHAstreamPos offset, SHAstreamSeek_e origin)
{
  int org;
  switch(origin) {
  case CUR:
    org = SEEK_CUR;
    break;
  case SET:
    org = SEEK_SET;
    break;
  case END:
    org = SEEK_END;
    break;
  default:
    org = origin;
  }
  return f ? fseek((FILE *)f, offset, org) : -1;
}

SHAstreamPos SHAstreamFile::Tell(void)
{
  return f ? ftell((FILE *)f) : -1;
}

int SHAstreamFile::Flush(void)
{
  return f ? fflush((FILE *)f) : -1;
}

int SHAstreamFile::IsEOF(void)
{
  return f ? feof((FILE *)f) : -1;
}
