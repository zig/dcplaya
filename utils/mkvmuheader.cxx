/**
 *  @file   mkvmuheader.cxx
 *  @author benjamin gerard <ben@sashipa.com>
 *  @date   2002/10/02
 *  @brief  Make a SEGA VMS file header, including icon data
 *
 *  $Id: mkvmuheader.cxx,v 1.3 2002-12-12 18:35:24 zigziggy Exp $
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

typedef unsigned char uint8;

inline static unsigned int Sq(unsigned int a)
{
  return a*a;
}

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
  int idx, rel, cnt;

  void dump() const {
    printf("%02x %02x %02x %02x, i:%5d, rel:%5d, cnt:%5d\n",
	   a,r,g,b,idx,rel,cnt);
  }

  int dist(const pix32_t * p) const {
    return
      Sq(r - p->r) +
      Sq(g - p->g) +
      Sq(b - p->b) +
      0 * Sq(a - p->a); // Alpha does not seem to be handle by DC, forget it !
  }

  void blend(pix32_t * p) {
    if ( ! (cnt+p->cnt)) {
      printf("Divide by zero\n");
      dump();
      p->dump();
    }
    a = (a*cnt + p->a*p->cnt) / (cnt+p->cnt);
    r = (r*cnt + p->r*p->cnt) / (cnt+p->cnt);
    g = (g*cnt + p->g*p->cnt) / (cnt+p->cnt);
    b = (b*cnt + p->b*p->cnt) / (cnt+p->cnt);
    cnt += p->cnt;
    p->cnt = 0;
    p->rel = rel;
  }

  operator int(void) const {
    return (a<<24) | (r<<16) | (g<<8) | b;
  }

};

typedef struct 
{
  uint8 b;  uint8 g;  uint8 r;
} pix24_t;

struct vmu_fileheader {
  /** Description as shown in VMS file menu. Padded with 0x20.               */
  uint8 vms_desc[16];
  /** Description as shown in DC boot ROM file manager. Padded with 0x20.    */
  uint8 long_desc[32];
  /** Identifier of application that created the file. Padded with 0x00.     */
  uint8 appli[16];
  /** Number of icons [1..3] (>1 for animated icons). @see vmu_icon_bitmap_t */
  uint16 icons;
  /** Icon animation speed. Is it in frame unit ?                            */
  uint16 anim_speed;
  /** Graphic eyecatch type (0 = none). @see vmu_eyecatch_e.                 */
  uint16 eyecatch;
  /** CRC. Ignored for game files.                                           */
  uint16 crc;
  /** Number of bytes of actual file data following header, icon(s) and
      graphic eyecatch. Ignored for game files.                              */
  uint32 data_size;
  /** Reserved (fill with zeros).                                            */
  uint8 reserved[20];
  /** Icon palette (16 x ARGB4444). Alpha 0 is transparent, 15 is opaque.    */
  uint16 lut[16];

  static int set(uint8 *d, int max, const char * s, int padd) {
    int len, err = 0;

    len = s ? strlen(s) : 0;
    if (len > max) {
      len = max;
      err = -1;
    }
    memcpy(d, s, len);
    memset(d+len, padd, max-len);
    return err;
  }

  static int set_string(uint8 *d, int max, const char * s) {
    return set(d,max,s,0);
  }

  static int set_text(uint8 *d, int max, const char * s) {
    return set(d,max,s,0x20);
  }

  void clean()
  {
    set_text(vms_desc,sizeof(vms_desc), 0);
    set_text(long_desc,sizeof(long_desc),0);
    set_string(appli,sizeof(appli),0);
    icons = 1;
    anim_speed = 6;
    eyecatch = 0;
    crc = 0;
    data_size = 512;
    memset(reserved, 0, sizeof(reserved));
    memset(lut,0, sizeof(lut));
  }

  vmu_fileheader() {
    clean();
  }


  int set_vms_desc(const char *s) {
    set_text(vms_desc, sizeof(vms_desc), s);
  }

  int set_long_desc(const char *s) {
    set_text(long_desc, sizeof(long_desc), s);
  }

  int set_appli(const char *s) {
    set_string(appli, sizeof(appli), s);
  }

  void set_lut(const pix32_t * p) {
    for (int i=0; i<16; ++i) {
      lut[i] =
	((p[i].a>>4) <<12) |
	((p[i].r>>4) << 8) |
	((p[i].g>>4) << 4) |
	((p[i].b>>4) << 0);
    }
  }

  static int Crc(const uint8 * buf, int size, int n) {
    int i, c;

    for (i = 0; i < size; i++) {
      n ^= (buf[i]<<8);
      for (c = 0; c < 8; c++) {
	if (n & 0x8000) {
	  n = (n << 1) ^ 4129;
	} else {
	  n = (n << 1);
	}
      }
    }
    return n & 0xffff;
  }

  void set_crc(const uint8 * buf, int size) {
    int l;
    data_size = size;
    l = Crc((const uint8 *)this, sizeof(*this), 0);
    l = Crc(buf, size, l);
    crc = l;
    printf("CRC:%02x\n", l);
  }

  void dump() {
    char tmp[128];
    memcpy(tmp, vms_desc, sizeof(vms_desc));
    tmp[sizeof(vms_desc)] = 0;
    printf("vms-desc  [%s]\n", tmp);
    memcpy(tmp, long_desc, sizeof(long_desc));
    tmp[sizeof(long_desc)] = 0;
    printf("long-desc [%s]\n", tmp);
    memcpy(tmp, appli, sizeof(appli));
    tmp[sizeof(appli)] = 0;
    printf("appli     [%s]\n", tmp);
    printf("speed     [%d]\n", (int)anim_speed);
  }
};

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
  printf("TGA %dx%dx%d alpha:%d\n", w, h, bpp, alpha);

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

static int cmp_index(const void *a, const void *b)
{
  return ((const pix32_t *)a)->idx - ((const pix32_t *)b)->idx;
}

static int cmp_count(const void *a, const void *b)
{
  return ((const pix32_t *)b)->cnt - ((const pix32_t *)a)->cnt;
}

static int cmp_color(const void *a, const void *b)
{
  int v,w;

  v = ((*(const pix32_t *)a) & 0xf0f0f0f0) >> 4;
  w = ((*(const pix32_t *)b) & 0xf0f0f0f0) >> 4;

  return v-w;
}

static void sort_by_color(pix32_t *p, int npix)
{
  qsort(p,npix,sizeof(*p), cmp_color);
}

static void sort_by_count(pix32_t *p, int npix)
{
  qsort(p,npix,sizeof(*p), cmp_count);
}

static void sort_by_index(pix32_t *p, int npix)
{
  qsort(p,npix,sizeof(*p), cmp_index);
}

void dump_pix(const pix32_t *p) {
  p->dump();
}

void dump_pix(const pix32_t *p, int n) {
  while (n--) {
    p->dump();
    ++p;
  }
}


static int set_count(pix32_t *p, int npix)
{
  int i;
  int cnt = 0;

  for (i=0; i<npix;) {
    pix32_t *p2 = p+i;
    for (++i; i<npix && !cmp_color(p2,p+i); ++i) {
      p2->cnt += p[i].cnt;
      p[i].rel = p2->rel;
      p[i].cnt = 0;
    }
//     printf("%5d : ",p2-p); dump_pix(p2);
    ++cnt;
  }
  return cnt;
}

static int find_best(const pix32_t *p, int npix, const pix32_t * w)
{
  int i;
  unsigned int best=0, score = w->dist(p);

  for (i=1; i<npix; ++i) {
    unsigned int sc;

    sc = w->dist(p+i);
    if (sc < score) {
      sc = score;
      best = i;
    }
  }
  return best;
}

static int palette(img_t *img, pix32_t * pal, int n)
{
  int npix = img->w*img->h;
  int i;
  int cnt = npix;

  /* Init :
   - set idx to the original pixel location so that original picture could be
     restored.
   - set rel to original pixel location. This map a pixel to */
  for (i=0; i<npix; ++i) {
    img->pix[i].idx = i;
    img->pix[i].rel = i;
    img->pix[i].cnt = 1;
  }
  sort_by_color(img->pix, npix);
  cnt = set_count(img->pix, npix);
  sort_by_count(img->pix, npix);
  printf("Color count: %d\n",cnt);

  while (cnt > n) {
    int victime, best;
    sort_by_count(img->pix, cnt);
//     printf("Color count: %d\n",cnt);
//     dump_pix(img->pix, cnt);
    victime = cnt-1;
    best = find_best(img->pix, victime, img->pix+victime);
    img->pix[best].blend(img->pix + victime);
    cnt = cnt - 1;
  }
  // Sort last...
  sort_by_count(img->pix, cnt);

  // Create palette...
  memcpy(pal, img->pix, cnt * sizeof(*pal));
  memset(pal+cnt, 0, (n-cnt) * sizeof(*pal));

  // Make palette index (negative relocation)
  for (i=0; i<cnt; ++i) {
    if (img->pix[i].idx != img->pix[i].rel) {
      fprintf(stderr, "Internal : index [%d] differs from relocation [%d]\n",
	      img->pix[i].idx,img->pix[i].rel);
      dump_pix(&img->pix[i]);
      return -1;
    }
    img->pix[i].rel = ~i;
  }

  // Make relocation
  sort_by_index(img->pix, npix);
  for (i=0; i<npix; ++i) {
    int rel;
    for (rel = i; rel >= 0; rel = img->pix[rel].rel)
      ;
    img->pix[i].rel = rel;
  }

  // Make more friendly index (>=0)
  for (i=0; i<npix; ++i) {
    img->pix[i].rel = ~img->pix[i].rel;
    if ((unsigned int)img->pix[i].rel >= n) {
      fprintf(stderr, "Internal : Palette index out of range [%d]\n",
	      img->pix[i].rel);
      dump_pix(img->pix+i);
      return -1;
    }
  }


  // Test count :
//   sort_by_count(img->pix, npix);
//   {
//     int c;
//     for (i=c=0; i<n; ++i) {
//       c += img->pix[i].cnt;
//     }
//     if (c != npix) {
//       fprintf(stderr, "Internal error : wrong count %d differs from %d\n",
// 	      c, npix);
//       return -1;
//     }
//   }

  for (i=0; i<n; ++i) {
    printf("pal[%3d] : ",i); dump_pix(pal + i);
  }

  sort_by_index(img->pix, npix);

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
	 "%s : Make SEGA(tm) Dreamcast(tm) VMU file header.\n"
	 "(c) 2002 Benjamin Gerard <ben@sashipa.com>\n"
	 "\n"
	 "Usage : %s <name> <desc> <appli> <icons.tga> <speed> <output>\n"
	 "\n"
	 "  name      : VMS file name (max 16 char).\n"
	 "  desc      : File description (max 32 char).\n"
	 "  appli     : Application name (max 16 char).\n"
	 "  icons.tga : Icons source (TGA no RLE width:32 Height:32,64,96)\n"
	 "              Only tested with 32bit pixel format,\n"
	 "              but should support 15,16 and 24.\n"
	 "  speed     : Animation speed (in frame).\n"
	 "  output    : Output file.\n"
	 "\n",
	 name, name);
  return 1;
}

static uint8 data[32*3][16];

// static int get_option(const char *opt, const char *name, const char **res)
// {
//   const char * s = strstr(opt,name);
//   if (s == opt) {
//     *res = opt+strlen(name);
//     return 1;
//   }
//   return 0;
// }

// static int get_option(const char *opt, const char *name, int  **res)
// {
//   const char * s = strstr(opt,name);
//   if (s == opt) {
//     s += strlen(name);
//     if (isdigit(*s)) {
//       **res = atoi(s);
//       return 1;
//     }
//   }
//   return 0;
// }

int main(int na, char **a)
{
  const char * bname = basename(a[0]);
  vmu_fileheader hd;
  img_t * img = 0;
  pix32_t pal[16];

  const char * vms_desc, * long_desc, * appli, * tga_file, * output;
  int speed = 4;

  if (na != 7) {
    return usage(bname);
  }

  vms_desc  = a[1];
  long_desc = a[2];
  appli     = a[3];
  tga_file  = a[4];
  speed     = atoi(a[5]);
  output    = a[6];
    
  hd.set_vms_desc(vms_desc);
  hd.set_long_desc(long_desc);
  hd.set_appli(appli);
  hd.anim_speed = speed;
  hd.dump();

  img = load_tga(tga_file);
  if (!img) {
    return -2;
  }

  printf("Loaded image [%dx%d]\n", img->w, img->h);
  if (img->w != 32 || (img->h < 32) || (img->h > 32*3) || (img->h&31)) {
    fprintf(stderr,
	    "TGA bad dimension: %dx%d\n Valid dimension are:\n"
	    " - 32x32 for single icon mode.\n"
	    " - 32x64 for 2 frames animation mode.\n"
	    " - 32x96 for 3 frames animation mode.\n",
	    img->w, img->h);
  }


  if (palette(img, pal, 16)) {
    return -3;
  }
  hd.set_lut(pal);
  hd.icons = img->h >> 5;

  pix32_t * p = img->pix;
  for (int y=0; y<img->h; ++y) {
    for (int x=0; x<16; ++x, p+=2) {
      data[y][x] = (p[0].rel << 4) + p[1].rel;
    }
  }

  int data_sz = 16 * img->h;
  hd.set_crc(data[0], data_sz);
  
  FILE * f;

  f = fopen(output ,"wb");
  if (!f) {
    perror(output);
    return -3;
  }

  if (fwrite(&hd, 1, sizeof(hd), f) != sizeof(hd)) {
    perror(output);
    return -4;
  }

  if (fwrite(data[0], 1, data_sz, f) != data_sz) {
    perror(output);
    return -4;
  }
  fclose(f);

  if(img) free(img);
  return 0;
}
