/**
 * @ingroup dcplaya_devel
 * @file    filetype.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @brief   Deal with file types and extensions.
 *
 * $Id: filetype.c,v 1.10 2002-12-15 16:15:03 ben Exp $
 */

#include <stdlib.h>
#include <string.h>
#include "filetype.h"
#include "filename.h"

#include "sysdebug.h"

typedef struct minor_type_s {
  struct minor_type_s * next; /* Next in list  */
  int type;                   /* File type     */
  const char * name;          /* Filetype name */
  const char * exts;          /* Extension list (double '\0' terminated) */
  char buffer[4];
} minor_type_t;

typedef struct major_type_s {
  const char * name;          /* Major name */
  minor_type_t * minor;       /* Mijor list */
} major_type_t;

static minor_type_t dir_minor    = { 0,             0x0003, "*",      "*\0"  };
static minor_type_t parent_minor = { &dir_minor,    0x0002, "parent", "..\0" };
static minor_type_t self_minor   = { &parent_minor, 0x0001, "self",   ".\0"  };
static minor_type_t root_minor   = { &self_minor,   0x0000, "root",   "\0"   };
static minor_type_t file_minor   = { 0,             0x1000, "*",      "*\0"  };

static major_type_t major[16] = {
  { "dir",  &root_minor },
  { "file", &file_minor },
};

const int filetype_root   = 0x0000;
const int filetype_self   = 0x0001;
const int filetype_parent = 0x0002;
const int filetype_dir    = 0x0003;
const int filetype_file   = 0x1000;

static minor_type_t * find_minor(int type)
{
  minor_type_t * m;
  if (type == -1) {
	return 0;
  }
  for (m = major[(type>>12)&15].minor; m && m->type != type; m = m->next)
	;
  return m;
}

static minor_type_t * find_minor_name(const minor_type_t * m,
									  const char * name)
{
  if (!name) {
	return 0;
  }
  for (; m && stricmp(name, m->name); m = m->next)
	;
  return (minor_type_t *)m;
}


static int findfree_minor(int majornum)
{
  int expected;
  minor_type_t * m;
  for (m = major[majornum].minor, expected = majornum << 12;
	   m && m->type == expected;
	   m = m->next, ++expected)
	;
  return (expected < ((majornum+1)<<12)) ? expected : -1;
}

static void insert_minor(minor_type_t * minor)
{
  minor_type_t * m, ** prev;
  int type = minor->type;
  major_type_t * maj = &major[(type>>12)&15];

  for (prev = &maj->minor, m = maj->minor;
	   m && m->type < type;
	   prev = &m->next, m = m->next)
	;

  *prev = minor;
  minor->next = m;
}

static int reserved_type(int type)
{
  return 0
	|| (type == -1)
	|| (type >= filetype_root && type <= filetype_dir)
	|| (type == filetype_file);
}

static void delete_minor(int type)
{
  minor_type_t * m, ** prev;
  major_type_t * maj = &major[FILETYPE_MAJOR_NUM(type)];
  type = FILETYPE(type);

  /* Protected filetype */
  if (reserved_type(type)) {
 	SDWARNING("Trying to remove reserved filetype [%04x]\n", type);
	return;
  }

  for (prev = &maj->minor, m = maj->minor; m; prev = &m->next, m = m->next) {
	if (m->type == type) {
	  *prev = m->next;
	  SDDEBUG("Removing filetype [%04x,%s:%s]\n", type, maj->name, m->name);
	  free(m);
	  return;
	}
  }

  SDWARNING("Removing filetype [%04x] failed.\n", type);
}

int filetype_major(const char * name)
{
  int i;
  for (i = 0; i < 16; ++i) {
	if ( ! stricmp(name, major[i].name)) {
	  return (i << 12);
	}
  }
  return -1;
}

int filetype_minor(const char * name, int type)
{
  int i;
  if (type < 0 || type > 0xFFFF) {
	return -1;
  }
  for (i = FILETYPE_MAJOR_NUM(type); i < 16; ++i) {
	const minor_type_t * m;
	for (m = major[i].minor; m; m = m->next) {
	  if (!stricmp(name, m->name) && m->type >= type) {
		return m->type;
	  }
	}
  }
  return -1;
}

const char * filetype_major_name(int type)
{
  return (type == -1) ? 0 : major[FILETYPE_MAJOR_NUM(type)].name;
}


const char * filetype_minor_name(int type)
{
  minor_type_t * m = find_minor(type);
  return m ? m->name : 0;
}

int filetype_names(int type, const char ** maj, const char ** min)
{
  minor_type_t * m;

  if (m = find_minor(type), m) {
	if (maj) *maj = major[FILETYPE_MAJOR_NUM(type)].name;
	if (min) *min = m->name;
	return 0;
  }
  return -1;
}

int filetype_major_add(const char * name)
{
  if (name) {
	int i;
	i = filetype_major(name);
	if (i != -1) {
	  SDDEBUG("Existing Major type [%04x, %s]\n", i, name);
	  return i;
	}
	for (i = 0; i < 16; ++i) {
	  if (!major[i].name) {
		major[i].name = strdup(name);
		major[i].minor = 0;
		SDDEBUG("Add Major type [%04x, %s]\n", i<<12, name); 
		return i << 12;
	  }
	}
  }
  return -1;
}

void filetype_major_del(int type)
{
  major_type_t * maj;
  minor_type_t * m, * n;
  int major_num = FILETYPE_MAJOR_NUM(type);

  if (reserved_type(type)) return;
  maj = &major[major_num];
  for (m = maj->minor; m; m = n) {
	n = m->next;
	free(m);
  }
  maj->name = 0;
  maj->minor = 0;
}

static const char * default_exts = "\0";

int filetype_add(int major_type, const char * name, const char *exts)
{
  int i;
  minor_type_t * m;
  int namelen, extslen;
  const char * e;

  if (major_type == -1) {
	return -1;
  }

  exts = exts ? exts : default_exts;
  name = name ? name : (exts + (exts[0]=='.'));

  major_type = FILETYPE_MAJOR_NUM(major_type);
  m = find_minor_name(major[major_type].minor, name);
  if (m) {
 	SDDEBUG("filetype [%04x, %s:%s] already exist\n",
 			m->type, major[major_type].name, name);
	return -1;
/* 	SDDEBUG("Replacing filetype [%04x, %s:%s]\n", */
/* 			m->type, major[major_type].name, name); */
/* 	m->exts = exts; */
/* 	return m->type; */
  }

  i = findfree_minor(major_type);
  if (i < 0) {
	return -1;
  }

  for (e = exts; e[0] || e[1]; ++e)
	;

  extslen = (e - exts) + 2;
  namelen = strlen(name) + 1;

  m = malloc(sizeof(*m) - sizeof(m->buffer) + extslen + namelen);
  if (!m) {
	return -1;
  }
  m->type = i;
  m->name = m->buffer;
  m->exts = m->buffer + namelen;
  memcpy((void*)m->name, name, namelen);
  memcpy((void*)m->exts, exts, extslen);

  insert_minor(m);
  SDDEBUG("Adding filetype [%04x, %s:%s]\n",
		  m->type, major[major_type].name, m->name); 

  return i;
}

void filetype_del(int type)
{
  if (type == -1) {
	return;
  }
  delete_minor(type);
}

/* Return 0:not found, 1:exact match 2:wildcard match */
static int extfind(const char * extlist, const char * ext)
{
  int found = 0;
  if (extlist && ext) {
	int len;
	while (!found && (len = strlen(extlist), len > 0)) {
/* 	  SDDEBUG("   -- [%s] into [%s]\n", ext, extlist); */
	  found = !stricmp(ext,extlist);
	  if (!found && len == 1 && extlist[0] == '*') {
		found = 2;
	  }
	  extlist += len+1;
	}
  }
  return found;
}

static const minor_type_t * find_minor_ext(const minor_type_t * m,
										   const char * ext, int * alt)
{
  const minor_type_t * fm = 0;

  *alt = 1;

/*   SDDEBUG("find [%s] into [%s]\n", */
/* 		  ext, major[FILETYPE_MAJOR_NUM(m->type)].name); */

  for ( ; m ; m=m->next) {
	int r;
/* 	SDDEBUG(" -- [%s] into [%s]\n", ext, m->name); */

	r = extfind(m->exts, ext);
	if (r) {
	  fm = m;
	  if (r==1) {
		*alt = 0;
		break;
	  }
	}
  }
  return fm;
}

static int get_regular(const char * fname, int mask)
{
  const minor_type_t * m;
  const char * e;
  int i, type, alt;

  if (!fname) {
	return -1;
  }

  type = filetype_file;
  e = fn_secondary_ext(fname,".gz");
  if (e && e[0]) {
	for (i = 1; i < 16; ++i) {
	  if (!(mask&(1<<i)) || !major[i].minor) continue;
	  m = find_minor_ext(major[i].minor, e, &alt);
	  if (m) {
		type = m->type;
		if (!alt) break;
	  }
	}
  } else {
	/* No extension, may be a filetype defined by filename */
	fname = fn_basename(fname);
	for (i = 1; i < 16; ++i) {
	  if (!(mask&(1<<i)) || !major[i].minor) continue;
	  m = find_minor_ext(major[i].minor, fname, &alt);
	  if (m) {
		type = m->type;
		if (!alt) break;
	  }
	}
  }

  return type;
}

 

/* Get type for a regular file (not a dir!) */
int filetype_regular(const char * fname)
{
  return get_regular(fname, -1);
}

int filetype_directory(const char * fname)
{
  int type = -1;

  if (!fname) {
	return -1;
  }
  
  if (!fname[0]) {
    type = filetype_root;
  }	else {
	int alt;
	const minor_type_t * m;
	m = find_minor_ext(major[0].minor, fn_basename(fname), &alt);
	if (m) {
	  type = m->type;
	}
  }
  return type;
}

int filetype_get(const char *fname, int size)
{
  int type;

  if (size == -1) {
    type = filetype_directory(fname);
  } else {
    type = filetype_regular(fname);
  }
  return type;
}

int filetype_get_filter(const char *fname, int major_mask)
{
  int type;

  if (major_mask == 1) {
    type = filetype_directory(fname);
  } else {
	type = get_regular(fname, major_mask);
  }
  return type;
}
