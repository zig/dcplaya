#include <assert.h>
#include <string.h>
#include <malloc.h>

#include "sha123/api.h"
#include "sha123/sha123.h"
#include "istream/istream_def.h"

#include "ffmpeg.h"

#include "playa.h"
#include "id3tag/id3tag.h"

#include "dcplaya/config.h"
#include "sysdebug.h"

/*  $$$ Get this from the id3lib !!! Sorry it is ugly joe :_(
 */ 
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

static void show_id3(playa_info_t * info, struct id3_tag const *tag);

int id3_info(playa_info_t * info, const char *fn)
{
  struct id3_file * f;
  int i;

  SDDEBUG(">> %s(%s)\n", __FUNCTION__, fn);

  memset(info, 0, sizeof(*info));
  
  f = id3_file_open(fn, ID3_FILE_MODE_READONLY);
  if (!f) {
    SDERROR("id3 : open error\n");
    return -1;
  }
   
  for (i=0; i<f->ntags; ++i) {
    show_id3(info, f->tags[i].tag);
  }
  
  id3_file_close(f);
  SDDEBUG("<< %s(%s) := [0]\n", __FUNCTION__, fn);
  return 0;
}



#ifndef N_
# define N_(A) A
#endif

#ifndef _
# define _(A) A
#endif
 
/*
 * NAME:  show_id3()
 * DESCRIPTION: display an ID3 tag
 */
static
void show_id3(playa_info_t * info3, struct id3_tag const *tag)
{
  unsigned int i;
  struct id3_frame const *frame;
  id3_ucs4_t const *ucs4;
  id3_latin1_t *latin1;

  /* $$$  Care of order in mp3dc.h */
  struct {
    char const *id;
    char const *name;
  } const info[] = {
    {ID3_FRAME_ARTIST, N_("Artist")},
    {ID3_FRAME_ALBUM, N_("Album")},
    {ID3_FRAME_TRACK, N_("Track")},
    {ID3_FRAME_TITLE, N_("Title")},
    {ID3_FRAME_YEAR, N_("Year")},
    {ID3_FRAME_GENRE, N_("Genre")},
  };

  /* text information */
  for (i = 0; i < sizeof(info) / sizeof(info[0]); ++i) {
    union id3_field const *field;
    unsigned int nstrings, namelen, j;
    char const *name;

    frame = id3_tag_findframe(tag, info[i].id, 0);
    if (frame == 0)
      continue;

    field = &frame->fields[1];
    nstrings = id3_field_getnstrings(field);

    name = info[i].name;
    namelen = name ? strlen(name) : 0;
    //    assert(namelen < sizeof(spaces));

    for (j = 0; j < nstrings; ++j) {
      ucs4 = id3_field_getstrings(field, j);
      assert(ucs4);

      if (strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
	ucs4 = id3_genre_name(ucs4);

      latin1 = id3_ucs4_latin1duplicate(ucs4);
      if (latin1 == 0)
	return;

      if (j == 0 && name) {
	info3->info[PLAYA_INFO_ARTIST+i].s = latin1;
      } else {
	free (latin1);
      }
    }
  }

  /* comments */
  i = 0;
  while ((frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, i++))) {
    id3_latin1_t *ptr;

    ucs4 = id3_field_getstring(&frame->fields[2]);
    assert(ucs4);

    if (*ucs4)
      continue;

    ucs4 = id3_field_getfullstring(&frame->fields[3]);
    assert(ucs4);

    latin1 = id3_ucs4_latin1duplicate(ucs4);
    if (latin1 == 0)
      break;

    {
      int c;
      ptr = latin1;
      while (c=*ptr, c) {
	if (c < ' ' || c>127) *ptr = ' ';
	ptr++;
      }
    }
    if (strlen(latin1) > 0) {
      info3->info[PLAYA_INFO_COMMENTS].s = latin1;
      break; /* One comment only ! */
    } else {
      free (latin1);
    }
  }
}
