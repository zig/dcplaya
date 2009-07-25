#include "istream/istream_def.h"

const char * istream_filename(istream_t * istream)
{
  return (!istream || !istream->name)
    ? 0
    : istream->name(istream);
}

int istream_open(istream_t *istream)
{
  return (!istream || !istream->open)
    ? -1
    : istream->open(istream);
}

int istream_close(istream_t *istream)
{
  return (!istream || !istream->close)
    ? -1
    : istream->close(istream);
}

int istream_read(istream_t *istream, void * data, int len)
{
  return (!istream || !istream->read)
    ? -1
    : istream->read(istream, data, len);
}

int istream_length(istream_t *istream)
{
  return (!istream || !istream->length)
    ? -1
    : istream->length(istream);
}

int istream_tell(istream_t *istream)
{
  return (!istream || !istream->tell)
    ? -1
    : istream->tell(istream);
}

int istream_seek(istream_t *istream, int offset)
{
  int pos;

  pos = istream_tell(istream);
  if (pos != -1) {
    if (offset) {
      istream_seek_t seek = (offset > 0) ? istream->seekf : istream->seekb;
      if (seek && seek(istream, offset) != -1) {
	pos += offset;
      } else {
	pos = -1;
      }
    }
  }
  return pos;
}
