/**
 * @file    texture.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/27
 * @brief   texture manager
 *
 * $Id: texture.c,v 1.3 2002-10-23 02:09:05 benjihan Exp $
 */

#include <stdlib.h>
#include <string.h>

#include <arch/spinlock.h>
#include <dc/ta.h>

#include "texture.h"
#include "translator.h"
#include "SHAtranslator/SHAtranslatorBlitter.h"
#include "allocator.h"
#include "filename.h"
#include "sysdebug.h"


/** Texture allocator. */
static allocator_t *texture;

/** Global locked texture counter */
static int texture_locks;

/** Global referenced texture counter */
static int texture_references;

/* Get exact power of 2 for a value or -1 */
static int log2(int v)
{
  int i;
  for (i=0; i<(sizeof(int)<<3); ++i) {
	if ((1<<i) == v) {
	  return i;
	}
  }
  return -1;
}

/* Get power of 2 greater or equal to a value or -1 */
static int greaterlog2(int v)
{
  int i;
  for (i=0; i<(sizeof(int)<<3); ++i) {
	if ((1<<i) >= v) {
	  return i;
	}
  }
  return -1;
}

static void texture_clean(texture_t * t)
{
  memset(t,0, sizeof(*t));
}

static int texture_enlarge(void)
{
  return -1;
}

int texture_init(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  texture_locks = 0;
  texture_references = 0;
  texture = allocator_create(256, sizeof(texture_t));
  if (!texture) {
	return -1;
  }
  ta_txr_release_all();
  texture_create_flat("default", 0xFFFFFF);
  return 0;
}

static int cmp_name(const void *a, const void *b)
{
  return stricmp((const char *)a, ((const texture_t *)b)->name);
}

static texture_t * find_name(const char * name)
{
  return allocator_match(texture, name, cmp_name);
}  
  
void texture_shutdown(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);
  if (texture) {
	allocator_destroy(texture);
  }
  texture = 0;
}

static texture_t * get_texture(texid_t texid)
{
  allocator_elt_t * e;
  texture_t * t;
  const int data_size = sizeof(texture_t)+sizeof(allocator_elt_t);
  if (texid<0 || texid>texture->elements) {
	return 0;
  }
  e = (allocator_elt_t *) (texture->buffer + texid * data_size);
  t = (texture_t *)(e+1);
  return t->addr ? t : 0;
}

texid_t texture_get(const char * texture_name)
{
  if (!texture_name) {
	return -1;
  }
  return allocator_index(texture, find_name(texture_name));
}

texid_t texture_create(texture_create_t * creator)
{
  texture_t * t = 0;
  int wlog2, hlog2, format;
  int size;
  void * addr;

  if (!creator || ! creator->reader) {
	return -1;
  }

  wlog2 = greaterlog2(creator->width);
  hlog2 = greaterlog2(creator->height);

  SDDEBUG("[%s] : [%s] %dx%d -> %dx%d %s\n",
		  __FUNCTION__, creator->name, creator->width, creator->height,
		  1<<wlog2, 1<<hlog2, creator->formatstr);

  /* Check texture dimension */
  if (wlog2 < 3 || wlog2 > 10 || hlog2 < 3 || hlog2 > 10) {
	SDERROR("Invalid texture size %dx%d\n", creator->width, creator->height);
	goto error;
  }

  /* Check texture format */
  format = texture_strtoformat(creator->formatstr);
  if (format<0) {
	SDERROR("Invalid texture format [%s]\n", creator->formatstr);
	goto error;
  }

  /* Look for an existing texture */
  t = find_name(creator->name);
  if (t) {
	SDERROR("Texture [%s] already exist.\n", creator->name);
	t = 0;
	goto error;
  }

  /* Alloc a texture. */ 
  t = allocator_alloc_inside(texture);
  if (!t) {
	goto error;
  }
  texture_clean(t); /* $$$ safety net, texture should have been cleaned */

  strncpy(t->name, creator->name, sizeof(t->name)-1);
  t->width  = creator->width;
  t->height = creator->height;
  t->wlog2  = wlog2;
  t->hlog2  = hlog2;
  t->format = format;

  size = creator->height << (wlog2);
  t->ta_tex = ta_txr_allocate(size<<1);
  /* $$$ TODO check alloc error */ 
  addr = ta_txr_map(t->ta_tex);
  /** $$$ Don't know if it is a good things to do ... */
  t->ta_tex += ta_state.texture_base;
  if (creator->width == 1<<wlog2) {
	int n;
	size = creator->reader(addr, n = size, creator);
	if (size != n) {
	  size = -1;
	}
  } else {
	int y;
	int h = creator->height;
	int pixelperline = creator->width;
	int bytesperline = 1 << (wlog2 + 1);
	int n = pixelperline;
	uint8 * dest = addr;
	for (y=0;
		 y<h && n==pixelperline;
		 ++y, dest+=bytesperline) {
	  n = creator->reader(dest, pixelperline, creator);
	  SDDEBUG("copied line %d %p -> %d\n",y,dest,n);  
	}
	if (n != pixelperline) {
	  size = -1;
	}
  }

  if (size < 0) {
	SDERROR("Copy texture bitmap failed\n");
	goto error;
  }
	
  t->addr = addr;
  return allocator_index(texture, t);

 error:
  if (t) {
	texture_clean(t);
	allocator_free(texture, t);
  }
  return -1;
}

/** Texture creator for memory block. */
typedef struct {
  texture_create_t creator; /**< General purpose creator  */
  uint8 * org;              /**< Original memory pointer  */
  uint8 * cur;              /**< Current memory pointer   */
  uint8 * end;              /**< End of menory buffer     */
  int bpplog2;              /**< Log2 of byte per pixel   */
  void (*convertor)(void *d, const void *s, int n);
} texture_create_memory_t;

static int memreader(uint8 *buffer, int n, texture_create_t * tc)
{
  texture_create_memory_t * tcm = (texture_create_memory_t *)tc;
  int rem;
  if (n <= 0 || !tcm->convertor) {
	return 0;
  }
  rem = (tcm->end - tcm->cur) >> tcm->bpplog2;
  if (n > rem) {
	n = rem;
  }
  tcm->convertor(buffer, tcm->cur, n);
  tcm->cur += n << tcm->bpplog2;
  return n;
}

texid_t texture_create_flat(const char *name, unsigned int argb)
{
  int i,alpha,dst_format;
  uint32 texture[8*8];
  texture_create_memory_t memcreator;

  /* Setup texture bitmap */
  for (i=0;i <64; ++i) {
	texture[i] = argb;
  }

  /* Setup memory creator */
  memset(&memcreator, 0, sizeof(memcreator));
  strncpy(memcreator.creator.name, name, sizeof(memcreator.creator.name)-1);

  /* Determine destination format depending on alpha value */
  alpha = argb & 0xFF000000;
  if (alpha && alpha != 0xFF000000) { 
	dst_format = TA_ARGB4444;
	memcreator.convertor = ARGB32toARGB4444;
  } else {
	dst_format = TA_RGB565;
	memcreator.convertor = ARGB32toRGB565;
  }

  memcreator.bpplog2 = 2;
  memcreator.creator.width     = 8;
  memcreator.creator.height    = 8;
  memcreator.creator.formatstr = texture_formatstr(dst_format);
  memcreator.creator.reader    = memreader;
  memcreator.org =
  memcreator.cur = (uint8 *)texture;
  memcreator.end = memcreator.org + (8*8*4);

  return texture_create(&memcreator.creator);
}

texid_t texture_create_file(const char *fname, const char * formatstr)
{
  const char * fbase, *fext;
  SHAwrapperImage_t *img = 0;
  int len;
  texture_create_memory_t memcreator;
  texid_t texid = -1;
  int src_format, dst_format;
  texture_t * t;


  if (!fname) {
	return -1;
  }

  /* Check the format string  */
  dst_format = -2;
  if (formatstr) {
	dst_format = texture_strtoformat(formatstr);
	if (dst_format < 0) {
	  SDERROR("Invalid texture format [%s]\n", formatstr);
	  goto error;
	}
  }

  /* Build texture name from filename */
  fbase = fn_basename(fname);
  fext  = fn_ext(fbase);
  len   = fext-fbase;
  if (len >= sizeof(memcreator.creator.name)) {
	SDWARNING("Texture truncated name\n");
	len = sizeof(memcreator.creator.name)-1;
  }
  memset(&memcreator, 0, sizeof(memcreator));
  memcpy(memcreator.creator.name,fbase,len);

  /* Look for an existing texture */
  t = find_name(memcreator.creator.name);
  if (t) {
	SDERROR("Texture [%s] already exist.\n", memcreator.creator.name);
	goto error;
  }

  /* Load image file to memory. */
  img = LoadImageFile(fname);
  if (!img) {
    goto error;
  }
  SDDEBUG("type    : %x\n", img->type);
  SDDEBUG("width   : %d\n", img->width);
  SDDEBUG("height  : %d\n", img->height);
  SDDEBUG("lutSize : %d\n", img->lutSize);


  /* Convert source format. */
  switch (img->type) {
  case SHAWIF_RGB565:
    src_format = TA_RGB565;
    break;
  case SHAWIF_ARGB1555:
    src_format = TA_ARGB1555;
    break;
  case SHAWIF_ARGB4444:
    src_format = TA_ARGB4444;
    break;
  case SHAWIF_ARGB32:
    src_format = -2;
    break;
  default:
	src_format = -1;
  }


  if (dst_format == -2) {
	/* Asked for a default format */
	dst_format = src_format;
  }
  if (dst_format == -2) {
	/* Source format was ARGB32. Need to choose another one.
	 * $$$ Later we can check for valid alpha info and choose the best format.
	 */
	int i, n;
	int alphav = 0;
	uint32 * data = (uint32 *)img->data;
	/* Scan alpha channel */
	for (i = 0, n=img->width*img->height; i < n && !(alphav & ~0x8001); ++i) {
	  alphav |= 1 << (data[i]>>28);
	}
	SDDEBUG("alpha scanning : %04x\n", alphav);
	if (alphav & ~0x8001) {
	  /* Found some alpha value : use 4444 */
	  dst_format = TA_ARGB4444;
	} else if (alphav == 0x8001) {
	  /* Found full opacity and full transparency : use 1555 */
	  dst_format = TA_ARGB1555;
	} else {
	  /* Full opacity or full transparency (meaningless !). */
	  dst_format = TA_RGB565;
	}
  }

  /** Choose the pixel convertor. */
  if (src_format == dst_format) {
	memcreator.convertor = ARGB16toARGB16;
	memcreator.bpplog2 = 1;
  } else if (src_format == -2) {
	memcreator.bpplog2 = 2;
	switch(dst_format) {
	case TA_ARGB1555:
	  memcreator.convertor = ARGB32toARGB1555;
	  break;
	case TA_RGB565:
	  memcreator.convertor = ARGB32toRGB565;
	  break;
	case TA_ARGB4444:
	  memcreator.convertor = ARGB32toARGB4444;
	  break;
	}
  }

  if (!memcreator.convertor) {
	SDERROR("Could not find a proper pixel convertor.\n");
	goto error;
  }

  memcreator.creator.width     = img->width;
  memcreator.creator.height    = img->height;
  memcreator.creator.formatstr = texture_formatstr(dst_format);
  memcreator.creator.reader    = memreader;
  memcreator.org = img->data;
  memcreator.cur = img->data;
  memcreator.end = img->data
	+ ((img->width*img->height) << memcreator.bpplog2);

  texid = texture_create(&memcreator.creator);
 error:
  if (img) {
	free(img);
  }
  return texid;
}

int texture_destroy(texid_t texid, int force)
{
  int err = -1;
  texture_t *t;
  /* $$$ TODO : Free texture mem !!! */
  allocator_lock(texture);
  if (t=get_texture(texid), t) {
	if (!force && t->lock) {
	  err = -2;
	} else if (!force && t->ref) {
	  err = -3;
	} else {
	  texture_clean(t);
	  allocator_free(texture,t);
	  err = 0;
	}
  }
  allocator_unlock(texture);
  return err;
}

int texture_reference(texid_t texid, int count)
{
  int ref = -1;
  texture_t *t;

  allocator_lock(texture);
  if (t=get_texture(texid), t) {
	ref = t->ref+count;
	if (ref < 0) {
	  ref = 0;
	}
	t->ref = ref;
  }
  allocator_unlock(texture);
  return ref;
}

texture_t * texture_lock(texid_t texid)
{
  texture_t *t;

  allocator_lock(texture);
  if (t=get_texture(texid), t) {
	if (t->lock) {
	  t = 0;
	} else {
	  t->lock = 1;
	  texture_locks++;
	}
  }
  allocator_unlock(texture);
  return t;
}

void texture_release(texture_t * t)
{
  if (!allocator_is_inside(texture,t)) {
	return;
  }
  allocator_lock(texture);
  if (t->lock) {
	t->lock--;
	texture_locks--;
  }
  allocator_unlock(texture);
}

void texture_collector(void)
{
}

static struct _argbflist_s  { char str[4]; int n; } flist[] =
  {
	{ "0565", TA_RGB565   },
	{ "1555", TA_ARGB1555 },
	{ "4444", TA_ARGB4444 },
	{ ""    , -1          }
  };

const char * texture_formatstr(int format)
{
  int i;
  for (i=0; flist[i].str[0]; ++i) {
	if (flist[i].n == format) {
	  return flist[i].str;
	}
  }
  return "????";
}

int texture_strtoformat(const char * formatstr)
{
  int i;
  if (!formatstr) {
	return -1;
  }
  for (i=0; flist[i].str[0] && strncmp(flist[i].str, formatstr, 4); ++i)
	;
  return flist[i].n;
}
