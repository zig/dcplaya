
#include <string.h>
#include "filetype.h"

typedef struct {
  const char *ext;
  int type;
} _ext_list_t;

/* Get filename extension. */
const char *filetype_ext(const char *fname)
{
  const char * e = strrchr(fname,'.');
  const char * p = strrchr(fname,'/');

  return (e>p) ? e : fname + strlen(fname);
}

/* Find extension ext in extension list exts */
static int find_ext(const char *ext, const _ext_list_t *exts)
{
  int i;

  for (i=0; exts[i].ext && stricmp(ext, exts[i].ext); ++i)
    ;
  return exts[i].type;
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
      {      0, FILETYPE_UNKNOWN }
    };
  const char * e;

  e = filetype_ext(fname);
  if (e[0]) {
    type = find_ext(e, ext);
  }
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
