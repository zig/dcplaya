/**
 * @file      mikmod_driver.c
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/09/20
 * @brief     mikmod input plugin for dcplaya
 *
 * $Id: mikmod_driver.c,v 1.2 2002-09-24 18:29:42 vincentp Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inp_driver.h"
#include "fifo.h"
#include "playa.h"

#include "mikmod.h"
#include "reader.h"
#include "gzip.h"

#include "sysdebug.h"

#define MAX_CHANNEL 64
extern MDRIVER drv_dcplaya;

static char extensions[400];
static const int curiosity = 0;

/* $$$ ben: Get this form mikmod_internals.h ... Sorry :) */
typedef struct DCMLOADER_S {
  struct DCMLOADER_S * next;
  const char * type;
} DCMLOADER;

/* defined in drv_dcplaya.c */
extern volatile int dcmikmod_status;

/* $$$ ben: for load_it.c */
int isalnum(int c) {
  c &= 255;
  return (c>='0' && c<'9') || (c>='a' && c<='z') || (c>='A' && c<='z');
}

static int tolower(int c)
{
  c &= 255;
  if (c>='A' && c<='Z') c -= 'A'-'a';
  return c;
}

static int add_ext(char *d, const char *ext, int max)
{
  int len = strlen(ext)+1, c;

  if (len >= max) {
    return 0;
  }
  while (c=*ext++, c) {
    *d++ = tolower(c);
  }
  *d++ = 0;
  return len;
}

static void build_extensions(void)
{
  DCMLOADER * l;
  int i=0;
  const int max = sizeof(extensions)-1;

  SDDEBUG("%s\n", __FUNCTION__);
  SDINDENT;

  for (l=(DCMLOADER *)MikMod_FirstLoader(); l && i<max; l=l->next) {
    int j;
    const char *type = l->type;
    char ext[128];

    /* $$$ ben : MODULE type does not correspond to extension for some
       loaders... Just hack them ! */
    if (!strcmp(type, "Standard module")) {
      type = "mod";
    } else if (!strcmp(type, "15-instrument module")) {
      type = "m15";
    }

    SDDEBUG("loader [%s]\n", l->type);
    
    if (!type) {
      continue;
    }
    ext[0] = '.';
    strcpy(ext+1, type);
    j = i;
    i += add_ext(extensions + i, ext, max-i);
    if (i>j) {
      SDDEBUG("--> [%s]\n", extensions + j);
    }
    j = i;
    strcat(ext, ".gz");
    i += add_ext(extensions + i, ext, max-i);
    if (i>j) {
      SDDEBUG("--> [%s]\n", extensions + j);
    }
    
    i += add_ext(extensions + i, ext, max-i);
  }

  extensions[i++] = 0;
  SDUNINDENT;
}

static int init(any_driver_t *d)
{
  int err = 0;

  SDDEBUG(">> %s(%s)\n", __FUNCTION__, d->name);
  SDINDENT;

  /* Register driver our driver */
  SDDEBUG("Register driver\n");
  MikMod_RegisterDriver(&drv_dcplaya);

  /* Register all module loaders */
  SDDEBUG("Register loaders\n");
  MikMod_RegisterAllLoaders();
  build_extensions();

  SDDEBUG("Init\n");
  if (err = MikMod_Init(0), err) {
    SDERROR("failed : [%s]\n", MikMod_strerror(MikMod_errno));
    goto error;
  }

 error:
  if (err) {
    MikMod_Exit();
  }

  SDUNINDENT;
  SDDEBUG(">> %s() := [%d]\n", __FUNCTION__ , err);
  return err;
}

static int stop(void)
{
  int err = 0;
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  /* Stop and Free modules. */
  SDDEBUG("Player free\n");
  Player_Free(Player_GetModule());

  SDUNINDENT;
  SDDEBUG("<< %s() := [%d]\n", __FUNCTION__ , err);
  return 0;
}
  
static int shutdown(any_driver_t *d)
{
  int err = 0;

  SDDEBUG(">> %s(%s)\n", __FUNCTION__, d->name);
  SDINDENT;

  stop();
  MikMod_Exit();

  SDUNINDENT;
  SDDEBUG("<< %s(%s) := [%d]\n", __FUNCTION__, d->name, err);

  return err;
}

static MODULE * dc_load_mod(const char *fn)
{
  MODULE *mod = 0;

  SDDEBUG(">> %s(%s)\n", __FUNCTION__, fn);
  SDINDENT;

  dcplaya_mreader.data = gzip_load(fn, &dcplaya_mreader.max);
  if (!dcplaya_mreader.data) {
    SDERROR("gzip_load error\n");
    goto error;
  }
  dcplaya_mreader.pos = 0;
  SDDEBUG("Module loaded [%p : %d]\n",
	  dcplaya_mreader.data, dcplaya_mreader.max);

  /* Last parameters is curiosity of loaded : search hidden pattern ! */
  mod = Player_LoadGeneric(&dcplaya_mreader.mreader, MAX_CHANNEL, curiosity);

  if (mod) {
    SDDEBUG("Start module [%s] [#%d] [%s]\n",
	    mod->modtype, mod->numchn, mod->songname);
    Player_Start(mod);
  } else {
    SDERROR("Load error : [%s]\n", MikMod_strerror(MikMod_errno));
  }

 error:
  if (dcplaya_mreader.data) {
    free(dcplaya_mreader.data);
  }

  SDUNINDENT;
  SDDEBUG("<< %s(%s) := [%p]\n", __FUNCTION__, fn, mod);

  return mod;
}

static int start(const char *fn, decoder_info_t *info)
{
  int err = -1;
  MODULE *mod = 0;

  SDDEBUG(">> %s(%s)\n", __FUNCTION__, fn);
  SDINDENT;

  stop();
  mod = dc_load_mod(fn);
  if (mod) {
    static char desc[256];

    sprintf(desc, "%s - #%d channels", mod->modtype, mod->numchn);
    info->bps    = 0;
    info->desc   = desc;
    info->frq    = 44100;
    info->bits   = 16;
    info->stereo = 1;
    info->time   = (mod->sngtime*1000) >> 10;

    err = 0;
  }

  SDUNINDENT;
  SDDEBUG("<< %s(%s) := [%d]\n", __FUNCTION__, fn, err);

  return err;
}

static int decoder(decoder_info_t *info)
{
  static int pos = -1;

  int status = 0;

  if (!Player_Active()) {
    pos = -1;
    status = INP_DECODE_END;
  } else {
    int p = Player_GetPosition();
    if (p != pos) {
      pos = p;
      SDDEBUG("Position:%d\n",pos);
    }

    MikMod_Update();

    

    status = dcmikmod_status;
    if (status != INP_DECODE_ERROR) {
      /* $$$ ben: Can check end & pos-change here */
    }
  }

  return status;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}


static int disk_info(playa_info_t *info, MODULE * mod)
{
  char tmp[512];
  int pos = 0;

  if (!mod) {
    mod = Player_GetModule();
    if (mod && Player_Active()) {
      pos = Player_GetPosition();
    }
  }

  if (!mod) {
    return -1;
  }

  sprintf(tmp, "%s - #%d channels", mod->modtype, mod->numchn);
  info->format = strdup(tmp);
  info->time   = playa_make_time_str((mod->sngtime*1000) >> 10);
  info->artist = 0;
  info->album  = 0;
  sprintf(tmp, "%d/%d", pos, mod->numpos);
  info->track  = strdup(tmp);
  info->title = strdup(mod->songname);
  info->year = 0;
  info->genre = strdup("tracker");
  info->comments = strdup(mod->comment);
  return 0;
}

static int file_info(playa_info_t *info, const char *fname)
{
  return -1;
}


static int info(playa_info_t *info, const char *fname)
{
  SDDEBUG("%s(%s)\n", __FUNCTION__, fname);
  if (fname) {
    return file_info(info, fname);
  } else {
    return disk_info(info, 0);
  }
}

inp_driver_t mikmod_driver =
{
  /* Any driver */
  {
    NEXT_DRIVER,          /**< Next driver (see any_driver.h)  */
    INP_DRIVER,           /**< Driver type */      
    0x0100,               /**< Driver version */
    "mikmod",             /**< Driver name */
    "Benjamin Gerard, "   /**< Driver authors */
    "Jean-Paul Mikkers (MikMak), "
    "Jake Stine (Air Richter), "
    "Miodrag Vallat <miod@mikmod.org>, "
    "Many others...",
    "multi module tracker", /**< Description */
    0,                    /**< DLL handler */
    init,                 /**< Driver init */
    shutdown,             /**< Driver shutdown */
    options,              /**< Driver options */
  },
  
  /* Input driver specific */
  
  0,                      /**< User Id */
  extensions,             /**< Extension list */
  start,
  stop,
  decoder,
  info,
};

EXPORT_DRIVER(mikmod_driver)
