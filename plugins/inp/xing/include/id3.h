#ifndef _ID3_H_
#define _ID3_H_

#include "file_wrapper.h"
#include "id3tag.h"
#include "playa_info.h"

/*  $$$ Get this from the id3lib !!! Sorry it is ugly joe :_(
 */
/* 
struct filetag
{
  struct id3_tag *tag;
  unsigned long location;
  id3_length_t length;
};

struct id3_file
{
  FILE *iofile;
  enum id3_file_mode mode;
  int flags;
  int options;
  struct id3_tag *primary;
  unsigned int ntags;
  struct filetag *tags;
};
*/
int id3_info(playa_info_t * info, const char *fn);

#endif

