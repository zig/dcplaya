/**
 *  @file   mkbox.cxx
 *  @author benjamin gerard <ben@sashipa.com>
 *  @date   2002/12/20
 *  @brief  Find box in TGA images
 *
 *  $Id: mktgabox.cxx,v 1.1 2002-12-20 23:57:45 ben Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

typedef unsigned char uint8;

class uint16
{
 protected:
  uint8 v0, v1;

 public:
  uint16() {
    v0 = v1 = 0;
  }
  uint16(int w) {
    v0 = w&255;
    v1 = w>>8;
  }
  operator int(void) const {
    return v0 + (v1<<8);
  }
    
};

class uint32
{
 protected:
  uint16 v0, v1;

 public:
  uint32() {
    v0 = v1 = 0;
  }
  uint32(int w) :
    v0(w), v1(w>>16) {
  }
  operator int(void) const {
    return v0 + (v1<<16);
  }

};

struct pix32_t
{
  uint8 b;  uint8 g;  uint8 r;  uint8 a;
  operator int(void) const {
    return (a<<24) | (r<<16) | (g<<8) | b;
  }

  pix32_t(int v)
  {
	a = v>>24;	r = v>>16;	g = v>>8;	g = v;
  }

};

typedef struct 
{
  uint8 b;  uint8 g;  uint8 r;
} pix24_t;

/* TGA pixel format */
typedef enum {
  NODATA   = 0,
  PAL      = 1,
  RGB      = 2,
  GREY     = 3,
  RLEPAL   = 9,
  RLERGB   = 10,
  RLEGREY  = 11
}  TGA_format_e;

typedef struct {
  uint8 idfield_size[1];  ///< id field lenght in byte
  uint8 colormap_type[1]; ///< 0:no color table, 1:color table, >=128:user def.
  uint8 type[1];          ///< pixel encoding format

  uint8 colormap_org[2];  ///< first color used in colormap
  uint8 colormap_n[2];    ///< number of colormap entries
  uint8 colormap_bit[1];  ///< number of bit per colormap entries [15,16,24,32]

  uint8 xorg[2];          ///< horizontal coord of lower left corner of image
  uint8 yorg[2];          ///< vertical coord of lower left corner of image
  uint8 w[2];             ///< width of image in pixel
  uint8 h[2];             ///< height of image in pixel
  uint8 bpp[1];           ///< bit per pixel
  uint8 descriptor[1];    ///< img desc bits [76:clr|5:bot/top|4:l/r|3-0:alpha]

} TGAfileHeader;

#define TGA8(F)  (*(uint8 *)tga->F)
#define TGA16(F) Word(tga->F)
#define TGA32(F) Dword(tga->F)


static unsigned int Word(uint8 *a)
{
  return a[0] + (a[1] << 8);
}

static unsigned int Dword(uint8 *a)
{
  return Word(a+0) + (Word(a+2) << 16);
}


static int readARGB32(pix32_t *p, FILE *f)
{
  if (fread(p,1,4,f) != 4) return -1;
  return 0;
}

static int readRGB32(pix32_t *p, FILE *f)
{
  if (fread(p,1,4,f) != 4) return -1;
  p->a = 255;
  return 0;
}

static int readRGB24(pix32_t *p, FILE *f)
{
  if (fread(p,1,3,f) != 3) return -1;
  p->a = 255;
  return 0;
}

static int readRGB565(pix32_t *p, FILE *f)
{
  uint8 tmp[2];
  uint v;
  int r,g,b;

  if (fread(tmp,1,2,f) != 2) return -1;
  v = tmp[0] + (tmp[1]<<8);

  r = (v>>11)&31;
  g = (v>> 5)&63;
  b = (v>> 0)&31;

  r = (r<<3) + (r>>2);
  g = (g<<2) + (g>>4);
  b = (b<<3) + (b>>2);

  p->r = r;  p->g = g;  p->b = b;  p->a = 255;

  return 0;
}

static int readARGB1555(pix32_t *p, FILE *f)
{
  uint8 tmp[2];
  uint v;
  int a,r,g,b;

  if (fread(tmp,1,2,f) != 2) return -1;
  v = tmp[0] + (tmp[1]<<8);

  r = (v>>10)&31;
  g = (v>> 5)&31;
  b = (v>> 0)&31;
  a = (-((v>>15)&1)) & 255;

  r = (r<<3) + (r>>2);
  g = (g<<3) + (g>>2);
  b = (b<<3) + (b>>2);

  p->r = r;  p->g = g;  p->b = b;  p->a = a;

  return 0;
}

static int readARGB4444(pix32_t *p, FILE *f)
{
  uint8 tmp[2];
  uint v;
  int a,r,g,b;

  if (fread(tmp,1,2,f) != 2) return -1;
  v = tmp[0] + (tmp[1]<<8);

  r = (v>> 8) & 15;
  g = (v>> 4) & 15;
  b = (v>> 0) & 15;
  a = (v>>12) & 15;

  r = (r<<4) + r;
  g = (g<<4) + g;
  b = (b<<4) + b;
  a = (a<<4) + a;

  p->r = r;  p->g = g;  p->b = b;  p->a = a;

  return 0;
}

typedef int (*tgaread_f)(pix32_t *p, FILE *f);

typedef struct 
{
  int w, h;
  pix32_t pix[1];
} img_t;

static img_t * load_tga(const char * name)
{
  FILE *f;
  TGAfileHeader hd, *tga = &hd;
  img_t * img = 0;
  pix32_t * pix;
  tgaread_f reader = 0;
  int w,h,bpp,alpha,desc,npix;

  f = fopen(name, "rb");
  if (!f) {
    perror (name);
    goto error;
  }

  /* Load tga header. */
  if (fread(&hd, 1, sizeof(hd), f) != sizeof(hd)) {
    perror(name);
    goto error;
  }

  switch(TGA8(type)) {
  case RGB:
    break;
  default:
    fprintf(stderr,"Invalid or unsupported TGA file (%d)\n", TGA8(type));
    goto error;
  }

  desc = TGA8(descriptor);
  w = TGA16(w);
  h = TGA16(h);
  bpp = TGA8(bpp);
  alpha = desc & 15;
  npix = w*h;
  fprintf(stderr, "TGA %dx%dx%d alpha:%d\n", w, h, bpp, alpha);

  switch(bpp) {
  case 32:
    if (alpha == 8) {
      reader = readARGB32;
    } else {
      reader = readRGB32;
    }
    break;
  case 24:
    reader = readRGB24;
    break;
  case 15:
    reader = readARGB1555;
    break;
  case 16:
    if (alpha == 0) {
      reader = readRGB565;
    } else if (alpha == 1) {
      reader = readARGB1555;
    } else if (alpha == 4) {
      reader = readARGB4444;
    }
    break;
  }

  if (!reader || npix < 1 || npix>(1<<20)) {
    fprintf(stderr, "Invalid or unsupported TGA file.\n");
    goto error;
  }

  /* Seek header. */
  if (fseek(f, TGA8(idfield_size), SEEK_CUR) == -1) {
    perror(name);
    goto error;
  }

  /* Allocate image */
  img = (img_t *)malloc(sizeof(img_t) + (npix-1) * sizeof(pix32_t));
  if (!img) {
    perror(name);
    goto error;
  }

  img->w = w;
  img->h = h;

  for (int i=0, sz=w*h; i<sz; ++i) {
    if (reader(img->pix+i, f)) {
      perror(name);
      goto error;
    }
  }
 
  // $$$ Add optionnal horizontal/vertical flip here $$$

  return img;

 error:
  if (f) {
    fclose(f);
  }
  if (img) {
    free(img);
  }
  return 0;
}

static void find_box(img_t *img, int x, int y, int mask)
{
  int offset = y * img->w;
  int color = img->pix[offset+x] & mask;
  int w,h;

//   fprintf(stderr,"Entering at [%d,%d]\n", x,y);


  /* First line give the width */
  for (w=x; w<img->w; ++w) {
	int c = img->pix[offset+w] & mask;
	if (c != color) {
	  break;
	}
	img->pix[offset+w] = img->pix[offset+w] & ~mask;
  }

  /* First line give the width */
  for (h=y+1, offset += img->w; h<img->h; ++h, offset += img->w) {
	int c = img->pix[offset+x] & mask;
	if (c != color) {
	  break;
	}
	// CLear line
	for (int x2=x; x2<w; ++x2) {
	  img->pix[x2+offset] = img->pix[x2+offset] & ~mask;
	}
  }


  printf("  { %3d, %3d, %3d, %3d, %3.2lf },\n",
		 x, y, w-x, h-y, (double)color/mask);
}

static void find_boxes_channel(img_t *img, int mask)
{
  int i=0;
  printf("{  -- %08x \n",mask);
  for (int y=0; y<img->h; ++y) {
	for (int x=0; x<img->w; ++x, ++i) {
	  if (img->pix[i] & mask) {
		find_box(img, x, y, mask);
	  } 
	}
  }
  printf("},\n");
}

static int find_boxes(img_t *img)
{
  // ignore alpha channel
  for (int i = 0; i<3; ++i) {
	find_boxes_channel(img, 0xFF << (i<<3));
  }
  return 0;
}

// VP : added this because basename is already defined in string.h with my 
// version of glibc
#define basename mybasename

static const char * basename(const char *name)
{
  const char * s = strrchr(name,'/');
  return s ? s+1 : name;
}

static int usage(const char *name)
{
  printf("\n"
	 "%s : Find color box in TGA image.\n"
	 "(c) 2002 Benjamin Gerard <ben@sashipa.com>\n"
	 "\n"
	 "Usage : %s <file.tga>\n"
		 "\n",
	 name, name);
  return 1;
}

int main(int na, char **a)
{
  const char * bname = basename(a[0]);
  const char * tga_file;
  img_t * img = 0;

  if (na != 2) {
    return usage(bname);
  }

  tga_file = a[1];

  img = load_tga(tga_file);
  if (!img) {
    return -2;
  }

  fprintf(stderr,"Loaded image [%dx%d]\n", img->w, img->h);
  find_boxes(img);

  if(img) free(img);
  return 0;
}
