/**
 * @file    screen_shot.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/14
 * @brief   Takes TGA screen shot.
 * 
 * $Id: screen_shot.c,v 1.5 2003-03-10 22:55:35 ben Exp $
 */

//#include <kos/fs.h>
#include <dc/ta.h>
#include <stdio.h>
#include <stdlib.h>

#include "dcplaya/config.h"
#include "zlib.h"
#include "sysdebug.h"

const char screen_shot_id[] = "dcplaya " DCPLAYA_VERSION_STR " - " DCPLAYA_URL;

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
  char idfield_size[1];  ///< id field lenght in byte
  char colormap_type[1]; ///< 0:no color table, 1:color table, >=128:user def.
  char type[1];          ///< pixel encoding format

  char colormap_org[2];  ///< first color used in colormap
  char colormap_n[2];    ///< number of colormap entries
  char colormap_bit[1];  ///< number of bit per colormap entries [15,16,24,32]

  char xorg[2];          ///< horizontal coord of lower left corner of image
  char yorg[2];          ///< vertical coord of lower left corner of image
  char w[2];             ///< width of image in pixel
  char h[2];             ///< height of image in pixel
  char bpp[1];           ///< bit per pixel
  char descriptor[1];    ///< img desc bits [76:clr|5:bot/top|4:l/r|3-0:alpha]

} TGAfileHeader;

static void Word(uint8 *a, uint16 v)
{
  a[0] = v;
  a[1] = v>>8;
}

static void Dword(uint8 *a, uint32 v)
{
  Word(a+0,v);
  Word(a+2,v>>16);
}


#define TGA8(F, N) *(uint8 *)tga->F = N
#define TGA16(F, N) Word(tga->F,N)
#define TGA32(F, N) Dword(tga->F, N)

static void set_tga(TGAfileHeader * tga, int w, int h, int idsize)
{
  TGA8(idfield_size, idsize);
  TGA8(colormap_type,0);
  TGA8(type, RGB);

  TGA16(colormap_org,0);
  TGA16(colormap_n,0);
  TGA8(colormap_bit,0);

  TGA16(xorg, 0);
  TGA16(yorg, 0);

  TGA16(w,w);
  TGA16(h,h);
  TGA8(bpp,16);
  TGA8(descriptor, (1<<5));
  
};

static int rgb565_1555(int v)
{
  int r,g,b;
  r = (v >> 11) & 0x1f;
  g = (v >> 6)  & 0x1f;
  b = v & 0x1f;

  return (r << 10) | (g << 5) | b;
}

/*
static int rgb1555_565(int v)
{
  int r,g,b;
  r = (v >> 10) & 0x1f;
  g = (v >> 5)  & 0x1f;
  b = v & 0x1f;

  return (r << 11) | (g << 6) | b;
}
*/
static void swap16(uint16 *d, uint16 *b, int n)
{
  while (n--) {
    *d++ = rgb565_1555(*b++);
  }
} 


/* This is a 16 bit TGA header 
 0000 0002 0000 0000 0000 0000 0280 01e0
 2010
*/
int screen_shot(const char *basename)
{
  static int shot=0;
  char tmp[2048];
  int err = -1;
  //  uint32 fd = 0;
  uint8 * vram = (uint8 *) 0xa5000000 + ta_state.buffers[1].frame;
  uint8 * vcpy = 0;
  int w = ta_state.w;
  int h = ta_state.h;
  TGAfileHeader tga;
  gzFile fd;


  SDDEBUG(">> %s(%s)\n", __FUNCTION__, basename);
  sysdbg_indent(1,0);

  ++shot;
  sprintf(tmp, DCPLAYA_HOME "/%s%03d.tga.gz", basename, shot);

  SDDEBUG("Opening [%s] for writing\n", tmp);
  fd = gzopen(tmp,"wb");//fs_open(tmp, O_WRONLY);
  if (!fd) {
    SDERROR("Open error.\n");
    goto error;
  }

  /* Alloc temporary screen buffer */
  vcpy = (uint8 *)malloc(w*h*2);
  if (!vcpy) {
    SDERROR("Alloc error.\n");
    goto error;
  }
  /* Copy VRAM to temporary with conversion. */
  swap16((uint16 *)vcpy, (uint16 *)vram, w*h);
  
  /* Build and save TGA header. */
  set_tga(&tga, w, h, sizeof(screen_shot_id));
  SDDEBUG("Write TGA header [%dx%dx16] [1555] (%d bytes)\n",
	  w, h, sizeof(tga));
  //  if (fs_write(fd, &tga, sizeof(tga)) != sizeof(tga)) {
  if (gzwrite(fd, &tga, sizeof(tga)) != sizeof(tga)) {
    SDERROR("Write error.\n");
    goto error;
  }

  /* Save identifier string. */
  SDDEBUG("Write TGA identifier string [%s] (%d bytes)\n",
	  screen_shot_id, sizeof(screen_shot_id));
  //  if (fs_write(fd, screen_shot_id, sizeof(screen_shot_id)) !=
  if (gzwrite(fd, screen_shot_id, sizeof(screen_shot_id)) !=
      sizeof(screen_shot_id)) {
    SDERROR("Write error.\n");
    goto error;
  }

  /* Save pixels. */
  SDDEBUG("Write Pixel data.\n", w, h, vcpy);
  //  if (fs_write(fd, vcpy, w*h*2) != w*h*2) {
  if (gzwrite(fd, vcpy, w*h*2) != w*h*2) {
    SDERROR("Write error.\n");
    goto error;
  }
  err = 0;

 error:
  if (fd) {
    //    fs_close(fd);
    gzclose(fd);
  }
  if (vcpy) {
    free(vcpy);
  }

  sysdbg_indent(-1,0);
  SDDEBUG("<< %s() = %d\n", __FUNCTION__, err);

  return err;
}
