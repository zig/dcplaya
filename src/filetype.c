/**
 * @file    filetype.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   Deal with file types and extensions.
 *
 * $Id: filetype.c,v 1.7 2002-11-14 23:40:29 benjihan Exp $
 */

#include <string.h>
#include "filetype.h"
#include "filename.h"

typedef struct {
  const char *ext;
  int type;
} _ext_list_t;

#define MAX_PLAY_TYPES 16u
static const char * playables[MAX_PLAY_TYPES];

static int filetype_findfree(void)
{
  int i;
  for (i=0; i<MAX_PLAY_TYPES; ++i) {
	if (!playables[i]) {
	  return i;
	}
  }
  return -1;
}

int filetype_add(const char *exts)
{
  int i;
  i = filetype_findfree();
  if (i < 0) {
	return i;
  }
  playables[i] = exts;

  {
	int len;
	while(len = strlen(exts), len > 0) {
/* 	  printf("add as %d : '%s'\n", i+ FILETYPE_PLAYABLE, exts); */
	  exts+= len+1;
	}
  }


  return i + FILETYPE_PLAYABLE;
}

void filetype_del(int type)
{
  type -= FILETYPE_PLAYABLE;
  if ((unsigned int)type >= MAX_PLAY_TYPES) {
	return;
  }
  playables[type] = 0;
}

/* Find extension ext in extension list exts */
static int find_ext(const char *ext, const _ext_list_t *exts)
{
  int i;

  for (i=0; exts[i].ext && stricmp(ext, exts[i].ext); ++i)
    ;
  return exts[i].type;
}

static int extfind(const char * extlist, const char * ext)
{
  if (extlist && ext) {
	int len;
	while (len = strlen(extlist), len > 0) {
/* 	  printf("cmp ('%s','%s')\n", ext,extlist);  */
	  if (!stricmp(ext,extlist)) {
		return 1;
	  }
	  extlist += len+1;
	}
  }
  return 0;
}

static int find_playable(const char *ext)
{
  int i;

  for (i=0; i<MAX_PLAY_TYPES; ++i) {
	if (extfind(playables[i], ext)) {
	  return FILETYPE_PLAYABLE + i;
	}
  }
  return FILETYPE_UNKNOWN;
}

/* Get type for a regular file (not a dir!) */
int filetype_regular(const char * fname)
{
  int type = FILETYPE_UNKNOWN;
  const _ext_list_t ext[] = 
    {
      { ".m3u", FILETYPE_M3U },
      { ".pls", FILETYPE_PLS },
      { ".elf", FILETYPE_ELF },
      { ".lef", FILETYPE_LEF },
      { ".lez", FILETYPE_LEF },
	  { ".gz",  FILETYPE_UNKNOWN | FILETYPE_GZ },
      {      0, FILETYPE_UNKNOWN }
    };
  const char * e, * e2;

/*   printf("find-type [%s] : ",fname); */

  e = fn_ext(fname);
  if (e && e[0]) {
/* 	printf("ext:[%s] ", e); */

    type = find_ext(e, ext);
	if (FILETYPE(type) == FILETYPE_UNKNOWN) {
	  e2 = e;
	  if (type & FILETYPE_GZ) {
		e2 = fn_secondary_ext(fname,0);
/* 		printf("ext2:[%s] ", e2); */
	  }
	  if (e2 && e2[0]) {
		int playtype = find_playable(e2);
		if (playtype) {
		  type = (type & FILETYPE_GZ) | playtype;
		}
	  }
	}

	if (type == (FILETYPE_GZ | FILETYPE_UNKNOWN)) {
	  unsigned int len;
	  e2 = fn_secondary_ext(fname,0);
	  if (len = e-e2, (len > 0 && len < 7u)) {
		char tmp[8];
		memcpy(tmp,e2,len);
		tmp[len] = 0;
		type = find_ext(tmp, ext) | FILETYPE_GZ;
	  }
	}
  }
/*   printf(" -> %04X\n", type); */
  return type;
}

int filetype_dir(const char * fname) 
{
  int type;

  if (!strcmp(fname, ".")) {
    type = FILETYPE_SELF;
  } else if (!strcmp(fname, "..")) {
    type = FILETYPE_PARENT;
  } else {
    type = FILETYPE_DIR;
  }
  return type;
}

int filetype_get(const char *fname, int size)
{
  int type;

  if (size == -1) {
    type = filetype_dir(fname);
  } else {
    type = filetype_regular(fname);
  }
  return type;
}
