/**
 * @file      mikmod_driver.c
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/09/20
 * @brief     mikmod input plugin for dcplaya
 *
 * $Id: mikmod_driver.c,v 1.6 2003-03-10 22:55:33 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcplaya/config.h"
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
static int prev_pos;
static char * usedpos;
static int usedpos_size;


/* $$$ ben: Get this form mikmod_internals.h ... Sorry :) */
typedef struct DCMLOADER_S {
  struct DCMLOADER_S * next;
  const char * type;
} DCMLOADER;

/* defined in drv_dcplaya.c */
extern volatile int dcmikmod_status;

static int disk_info(playa_info_t *info, MODULE * mod);

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

/*     SDDEBUG("loader [%s]\n", l->type); */
    
    if (!type) {
      continue;
    }
    ext[0] = '.';
    strcpy(ext+1, type);
    j = i;
    i += add_ext(extensions + i, ext, max-i);
/*     if (i>j) { */
/*       SDDEBUG("--> [%s]\n", extensions + j); */
/*     } */
    j = i;
    strcat(ext, ".gz");
    i += add_ext(extensions + i, ext, max-i);
/*     if (i>j) { */
/*       SDDEBUG("--> [%s]\n", extensions + j); */
/*     } */
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

  usedpos = 0;
  usedpos_size = 0;
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
  prev_pos = -1;
  
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
  if (usedpos) {
    free(usedpos);
    usedpos_size = 0;
  }

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

 error:
  if (dcplaya_mreader.data) {
    free(dcplaya_mreader.data);
  }

  if (!mod) {
    SDERROR("Load error : [%s]\n", MikMod_strerror(MikMod_errno));
  } else {
    SDDEBUG("Module loaded:\n" " - [%s]\n" " - [#%d]\n" " - [%s]\n",
	    mod->modtype, mod->numchn, mod->songname);
  }

  SDUNINDENT;
  SDDEBUG("<< %s(%s) := [%p]\n", __FUNCTION__, fn, mod);

  return mod;
}

static int start(const char *fn, int track, playa_info_t *info)
{
  int err = -1;
  MODULE *mod = 0;

  SDDEBUG(">> %s(%s, %d)\n", __FUNCTION__, fn, track);
  SDINDENT;
  track = track;
  stop();
  mod = dc_load_mod(fn);
  if (mod) {
    Player_Start(mod);
    prev_pos = -1;
    if ((usedpos_size<<3) < mod->numpos) {
      int need = (mod->numpos + 7) >> 3;
      char * b = realloc(usedpos, need);
      if (b) {
	usedpos_size = need;
	usedpos = b;
      }
    }
    if (usedpos) {
      memset(usedpos,0,usedpos_size);
    }
    err = 0;
  }
  disk_info(info,mod);

  SDUNINDENT;
  SDDEBUG("<< %s(%s) := [%d]\n", __FUNCTION__, fn, err);

  return err;
}

static int decoder(playa_info_t *info)
{
  int status = 0;

  if (!Player_Active()) {
    prev_pos = -1;
    status = INP_DECODE_END;
  } else {
    MikMod_Update();

    status = dcmikmod_status;
    if (status != INP_DECODE_ERROR) {
      int p = Player_GetPosition();

      if (p >= 0 && p != prev_pos) {
	int idx = p>>3, bit = p&7;
	prev_pos = p;

	if (usedpos && idx < usedpos_size) {
	  if (usedpos[idx] & (1<<bit)) {
	      SDDEBUG("Auto loop detected at position #%d\n", p);
	      return INP_DECODE_END;
	  } else {
	    usedpos[idx] |= (1<<bit);
	  }
	}

	if (!disk_info(info, 0)) {
	  status |= INP_DECODE_INFO;
	}
      }
    }
  }

  return status;
}

static driver_option_t * options(any_driver_t * d, int idx,
				 driver_option_t * o)
{
  return o;
}

static char * track_info(MODULE *mod, char *tmp)
{
  int pos = -1;

  if (Player_Active()) {
    pos = Player_GetPosition();
  }
  if (pos>=0) {
    sprintf(tmp, "%d/%d", pos+1, mod->numpos);
  } else {
    sprintf(tmp, "%d", mod->numpos);
  }
  return tmp;
}

static char * desc_info(MODULE *mod, char *tmp)
{
  sprintf(tmp, "%s - %d channels", mod->modtype, mod->numchn);
  return tmp;
}

static int update_info(playa_info_t *info, MODULE *mod, char *tmp)
{
  info->update_mask = 0;
  if (!mod) {
    return -1;
  }
  playa_info_track(info, track_info(mod, tmp));
  return 0;
}

static int disk_info(playa_info_t *info, MODULE * mod)
{
  char tmp[512];

  //  SDDEBUG("%s(%p,%p)\n", __FUNCTION__, info, mod);

  if (!mod) {
/*     SDDEBUG("[%s] : update track only\n", __FUNCTION__); */
    return update_info(info, Player_GetModule(), tmp);
  }

/*   SDDEBUG("[%s] : update all\n", __FUNCTION__); */

  if (update_info(info, mod, tmp)) {
/*     SDDEBUG("[%s] : failed all\n", __FUNCTION__); */
    return -1;
  }

  playa_info_bits(info,1);
  playa_info_stereo(info,1);
  playa_info_frq(info,44100);
  playa_info_time(info, mod->sngtime);
  playa_info_bps(info,0);
  playa_info_desc(info,desc_info(mod, tmp));

  playa_info_artist(info,0);
  playa_info_album(info,0);
  playa_info_title(info,mod->songname);
  playa_info_year(info,0);
  playa_info_genre(info,"tracker");
  playa_info_comments(info,mod->comment);

  //  SDDEBUG("%s(%p,%p) := 0\n");

  return 0;
}

// $$$ ben: Load whole module file. There must be better functions in mikmod
static int file_info(playa_info_t *info, const char *fname)
{
  int err;
  MODULE * mod;

  mod = dc_load_mod(fname);
  if (!mod) {
    err = -1;
  } else {
    err = disk_info(info,  mod);
    free(mod);
  }

  return err;
}

static int info(playa_info_t *info, const char *fname)
{
  SDDEBUG("%s(%s)\n", __FUNCTION__, fname);
  if (fname) {
    return file_info(info, fname);
  } else {
    return disk_info(info, Player_GetModule());
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
