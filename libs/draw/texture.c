/**
 * @file    texture.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @author  vincent penne <ziggy@sashipa.com>
 * @date    2002/09/27
 * @brief   texture manager
 *
 * $Id: texture.c,v 1.19 2003-03-18 16:11:10 ben Exp $
 */

#include <stdlib.h>
#include <string.h>

#include <arch/spinlock.h>
#include <dc/ta.h>

#include "dcplaya/config.h"
#include "draw/texture.h"
#include "translator/translator.h"
#include "translator/SHAtranslator/SHAtranslatorBlitter.h"
#include "allocator.h"
#include "filename.h"
#include "sysdebug.h"


/** Video memory heap */
static eh_heap_t * vid_heap;

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
  t->ta_tex = ~0;
}

static int texture_enlarge(void)
{
  return -1;
}


static eh_block_t * freeblock_alloc(eh_heap_t * heap)
{
  /*  printf("freeblock_alloc %x\n", ta_txr_map(heap->current_offset));*/

  /* use video memory to maintain free blocks structure */
  return ta_txr_map(heap->current_offset);
}

static void freeblock_free(eh_heap_t * heap, eh_block_t * b)
{
  /*  printf("freeblock_free %x\n", b);*/

}

static eh_block_t * usedblock_alloc(eh_heap_t * heap)
{
  /*  printf("usedblock_alloc %x\n", heap->userdata);*/

  /* use texture structure ehb entry to store used block structure */
  return (eh_block_t *) heap->userdata;
}

static void usedblock_free(eh_heap_t * heap, eh_block_t * b)
{
  /*  printf("usedblock_free %x\n", b);*/

}

static size_t vid_sbrk(eh_heap_t * heap, size_t size)
{
  size_t total = 8*1024*1024 - ta_state.texture_base;

  SDDEBUG("vid_sbrk : asked %d, gives %d\n", size, total);

  /* return the maximum amount of memory */
  /*  return 3*1024*1024; */
  return total;
}

int texture_init(void)
{
  SDDEBUG("[%s]\n", __FUNCTION__);

  /* create the video memory heap */
  vid_heap = eh_create_heap();
  if (vid_heap == NULL)
    return -1;
  vid_heap->freeblock_alloc = freeblock_alloc;
  vid_heap->freeblock_free = freeblock_free;
  vid_heap->usedblock_alloc = usedblock_alloc;
  vid_heap->usedblock_free = usedblock_free;
  vid_heap->sbrk = vid_sbrk;
  vid_heap->small_threshold = 10*1024; /* small allocs are at the end of memory */

  texture_locks = 0;
  texture_references = 0;
  texture = allocator_create(256, sizeof(texture_t), "texture");
  if (!texture) {
    return -1;
  }
  ta_txr_release_all();
  texture_create_flat("default", 8, 8, 0xFFFFFF);
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
    texture = NULL;
  }

  if (vid_heap) {
    eh_destroy_heap(vid_heap);
    vid_heap = NULL;
  }

  texture = 0;
}


/* Check for twiddlable texture and set twiddlable bit properly */
int texture_twiddlable(texture_t * t)
{
  return 
    t->twiddlable = 1
    && !(t->height > t->width)          /* this case is not tested yet      */
    && !( (t->height - 1) & t->height ) /* the height is not a power of two */
    && !( (t->width - 1) & t->width )   /* the width is not a power of two  */
    ;
}

int texture_twiddle(texture_t * t, int wanted)
{
  /* twiddle or de-twiddle the texture, need a temporary buffer,
     this cannot be done in place unfortunatly ... */
  /* works only in 16bps for now ! */
  /* $$$ ben : Add a check for twiddlable */
  if (texture_twiddlable(t) && wanted != t->twiddled) {
    int w = t->width;
    int h = t->height;
    int bpp = 16; /* <---- just set this variable to change bits per pixel */
    int size = w*h*bpp/8;
    void * buf = malloc(size);
    if (buf == NULL)
      return;
    memcpy(buf, t->addr, size);
    if (!wanted) {
      int tmp = w;
      w = h;
      h = tmp;
    }
    txr_twiddle_copy_general(buf, 
			     t->ta_tex - ta_state.texture_base, 
			     w, h, 
			     bpp);
    free(buf);
    t->twiddled = wanted;
  }
  return t->twiddled;
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

static char * texture_built_name(char *name, const char *fname, int max)
{
  const char * fbase, * fext;
  int len;

  if (!fname) {
    *name = 0;
    return 0;
  }

  fbase = fn_basename(fname);
  fext  = fn_ext(fname);
  len   = fext-fbase;

  if (len >= max) {
    SDWARNING("Texture name truncated [%s]\n", fname);
    len = max-1;
  }
  memcpy(name,fbase,len);
  name[len] = 0;
  return name;
}

texid_t texture_get(const char * texture_name)
{
  char name[TEXTURE_NAME_MAX];

  if (!texture_built_name(name, texture_name, sizeof(name))) {
    return -1;
  }
  return allocator_index(texture, find_name(name));
}

texid_t texture_exist(texid_t texid)
{
  return !get_texture(texid) ? -1 : texid;
}

static void * vid_alloc(texture_t * t, size_t size)
{
  size = (size + 31) & (~31);

  vid_heap->userdata = &t->ehb;
  if (eh_alloc(vid_heap, size) == NULL) {
    return NULL;
  }
  t->ta_tex = t->ehb.offset;
  /*  t->ta_tex = ta_txr_allocate(size); */
  /* $$$ TODO check alloc error */ 
  t->addr = ta_txr_map(t->ta_tex);
  /** $$$ Don't know if it is a good things to do ... */
  t->ta_tex += ta_state.texture_base;
  
  return t->addr;
}

void vid_free(texture_t * t)
{
  if (t->ta_tex != ~0) {
    eh_free(vid_heap, &t->ehb);
  }
}

void texture_memstats()
{
  if (vid_heap) {
    printf("Video memory usage statistics :\n");
    eh_dump_freeblock(vid_heap);
  }
}

texid_t texture_dup(texid_t texid, const char * name)
{
  texture_t * ts, * t = 0;
  int size;

  SDDEBUG("[%s] : %d [%s]\n",
	  __FUNCTION__, texid, name);

  /* Look for an existing texture */
  ts = find_name(name);
  if (ts) {
    SDERROR("Texture [%s] already exist.\n", name);
    ts = 0;
    goto error;
  }

  ts = texture_fastlock(texid);
  if (!ts) {
    SDERROR("Texture [#%d] not found.\n", texid);
    goto error;
  }
  /* de-twiddle the texture if necessary */
  /* $$$ ben : What if twiddled ? Just copying the texture ? And set twiddle
               flags like source texture. */
/*   texture_twiddle(ts, 0); */

  /* Alloc a texture. */ 
  t = allocator_alloc_inside(texture);
  if (!t) {
    goto error;
  }
  texture_clean(t);
  strncpy(t->name, name, sizeof(t->name)-1);
  t->width  = ts->width;
  t->height = ts->height;
  t->wlog2  = ts->wlog2;
  t->hlog2  = ts->hlog2;
  t->format = ts->format;
  t->twiddlable = ts->twiddlable;
  t->twiddled = ts->twiddled;
  size = t->height << t->wlog2;

  if (vid_alloc(t, size<<1) == NULL)
    goto error;

  memcpy(t->addr, ts->addr, size<<1);
  texture_release(ts);
  return allocator_index(texture, t);

 error:
  if (ts) {
    texture_release(ts);
  }
  if (t) {
    texture_clean(t);
    allocator_free(texture, t);
  }
  return -1;
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

  addr = vid_alloc(t, size<<1);
  if (addr == NULL)
    goto error;

  texture_twiddlable(t);

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

    if (t->ta_tex != ~0) {
      eh_free(vid_heap, &t->ehb);
    }

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

/** Texture creator for flat texture. */
typedef struct {
  texture_create_t creator; /**< General purpose creator  */
  uint16 color;             /**< Colorvalue               */
} texture_create_flat_t;

static int flatreader(uint8 *buffer, int n, texture_create_t * tc)
{
  texture_create_flat_t * tcm = (texture_create_flat_t *)tc;
  int i;
  uint16 val;
  if (n <= 0) {
    return 0;
  }
  val = tcm->color;
  for (i=0; i<n; ++i) {
    ((uint16 *)buffer)[i] = val;
  }
  return n;
}

texid_t texture_create_flat(const char *name, int width, int height,
			    unsigned int argb)
{
  int alpha,dst_format;
  texture_create_flat_t flatcreator;

  width  = width  ? width  : 8;
  height = height ? height : 8;

  /* Setup memory creator */
  memset(&flatcreator, 0, sizeof(flatcreator));
  strncpy(flatcreator.creator.name, name, sizeof(flatcreator.creator.name)-1);

  /* Determine destination format depending on alpha value */
  alpha = argb & 0xFF000000;
  if (alpha && alpha != 0xFF000000) { 
    dst_format = TA_ARGB4444;
    ARGB32toARGB4444(&flatcreator.color, &argb, 1);
  } else {
    dst_format = TA_RGB565;
    ARGB32toRGB565(&flatcreator.color, &argb, 1);
  }

  flatcreator.creator.width     = width;
  flatcreator.creator.height    = height;
  flatcreator.creator.formatstr = texture_formatstr(dst_format);
  flatcreator.creator.reader    = flatreader;

  return texture_create(&flatcreator.creator);
}

texid_t texture_create_file(const char *fname, const char * formatstr)
{
  SHAwrapperImage_t *img = 0;
  texture_create_memory_t memcreator;
  texid_t texid = -1;
  int src_format, dst_format;
  texture_t * t;

  if (!fname) {
    SDERROR("Invalid NULL filename\n");
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
  memset(&memcreator, 0, sizeof(memcreator));
  texture_built_name(memcreator.creator.name, fname,
		     sizeof(memcreator.creator.name));

  /* Look for an existing texture */
  t = find_name(memcreator.creator.name);
  if (t) {
    SDERROR("Texture [%s] already exist.\n", memcreator.creator.name);
    goto error;
  }

  /* Load image file to memory. */
  img = LoadImageFile(fname,0);
  if (!img) {
    SDERROR("Load image file [%s] failed\n", fname);
    goto error;
  }
  SDDEBUG("type    : %x\n", img->type);
  SDDEBUG("width   : %d\n", img->width);
  SDDEBUG("height  : %d\n", img->height);
  SDDEBUG("lutSize : %d\n", img->lutSize);

  /* Convert source format. */
  switch (img->type) {
  case SHAPF_RGB565:
    src_format = TA_RGB565;
    break;
  case SHAPF_ARGB1555:
    src_format = TA_ARGB1555;
    break;
  case SHAPF_ARGB4444:
    src_format = TA_ARGB4444;
    break;
  case SHAPF_ARGB32:
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

      vid_free(t);

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


texture_t * texture_fastlock(texid_t texid)
{
  texture_t *t;

  allocator_lock(texture);
  if (t=get_texture(texid), t) {
    if (t->lock) {
      SDERROR("[%s] : #%d [%s] already locked\n",
	      __FUNCTION__, texid, t->name);
      t = 0;
    } else {
      t->lock = 1;
      texture_locks++;
    }
  }
  allocator_unlock(texture);
  return t;
}

texture_t * texture_lock(texid_t texid)
{
  texture_t * t = texture_fastlock(texid);
  if (t)
    /* make sure the texture is not twiddled */
    texture_twiddle(t, 0);
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
