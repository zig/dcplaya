/**
 * @file    reader.c
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/09/20
 * @brief   Memory file reader for dcplaya mikmod input plugin.
 *
 * $Id: reader.c,v 1.1 2002-09-21 09:53:40 benjihan Exp $
 */

#include "reader.h"
#include "sysdebug.h"

static BOOL Seek(struct MREADER * r, long offset, int whence)
{
  DCPLAYA_MREADER * mr = (DCPLAYA_MREADER *)r;
  int pos;

  if (!mr) {
    return -1;
  }

  //  SDDEBUG("S");

  pos = mr->pos;

  switch(whence) {
  case SEEK_SET:
    pos = offset;
    break;
  case SEEK_END:
    pos = mr->max + offset;
    break;
  case SEEK_CUR:
    pos += offset;
    break;
  default:
    return -1;
  }
  if (pos < 0) {
    pos = 0;
  } else if (pos > mr->max) {
    pos = mr->max;
  }

  //  SDDEBUG("[%d] ", pos);

  mr->pos = pos;
  return 0;
}

static long Tell(struct MREADER * r)
{
  DCPLAYA_MREADER * mr = (DCPLAYA_MREADER *)r;

  if (!r) {
    return -1;
  }

  //  SDDEBUG("?[%d] ", mr->pos);

  return mr->pos;
}

static BOOL Read(struct MREADER * r,void * data, size_t size)
{
  int save = size;
  DCPLAYA_MREADER * mr = (DCPLAYA_MREADER *)r;

  if (!r) {
    return -1;
  }

  if (size > (mr->max - mr->pos)) {
    size = mr->max - mr->pos;
  }
  if (!size) {
    return 0;
  }
  if (!data || !mr->data) {
    return -1;
  }
  memcpy(data, mr->data + mr->pos, size);
  mr->pos += size;

  //  SDDEBUG("R[%d/%d] ", size, save);

  return size;
}

static int Get(struct MREADER * r)
{
#if DEBUG
  static int acu = 0; 
#endif
  DCPLAYA_MREADER * mr = (DCPLAYA_MREADER *)r;

  if (!r) {
    return -1;
  }
  if (mr->max == mr->pos) {
    return EOF;
  }

#if DEBUG
  if (!(acu++)&1023) {
    //    SDDEBUG(".");
  }
#endif

  return mr->data[mr->pos++];
}

static BOOL Eof(struct MREADER * r)
{
  DCPLAYA_MREADER * mr = (DCPLAYA_MREADER *)r;
  if (!r) {
    return -1;
  }
  //  SDDEBUG("!");

  return mr->pos == mr->max;
}

DCPLAYA_MREADER dcplaya_mreader = {
  {
    Seek,
    Tell,
    Read,
    Get,
    Eof
  },
  0,0,0
};
  
