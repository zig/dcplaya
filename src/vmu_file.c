/**
 * @file    vmu_file.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2003/03/10
 * @brief   VMU file load and save function.
 *
 * $Id: vmu_file.c,v 1.4 2004-07-04 14:16:45 vincentp Exp $
 */

#include <kos/thread.h>
#include <arch/spinlock.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dcplaya/config.h"
#include "sysdebug.h"
#include "vmu_file.h"
#include "dcar.h"
#include "file_utils.h"

#define VMUFILE_PATH_MIN (8+1)  /* "/vmu/??/" + leaf-min [1]  */
#define VMUFILE_PATH_MAX (8+11) /* "/vmu/??/" + leaf-max [11] */

static volatile vmu_trans_hdl_t hdl, next_hdl;
static spinlock_t mutex;
static kthread_t * thd; 
static volatile vmu_trans_status_e cur_status = VMU_TRANSFERT_INIT_ERROR;
static int header_size;
static const char header_name[] = "/rd/vmu_header.bin";
static volatile int init = 0;
static char vmu_file_default_path[VMUFILE_PATH_MAX+1]; /* +1 for '\0' */

#ifdef NOTYET
extern volatile int vmu_access_right;
#else
volatile int vmu_access_right;
#endif

int vmu_file_header_size(void)
{
  return (!init) ? -1 : header_size;
}

const char * vmu_file_get_default(void)
{
  return vmu_file_default_path[0] ? vmu_file_default_path : 0;
}

const char * vmu_file_set_default(const char * default_path)
{
  if  (default_path) {
    int len = strlen(default_path);
    /*           012345678 */
    /* At least "/vmu/??/? */
    if (len < VMUFILE_PATH_MIN || len > VMUFILE_PATH_MAX) {
      return 0;
    }
    /* Must begin by "/vmu/" */
    if (strstr(default_path,"/vmu/") != default_path) {
      return 0;
    }
    /* Must be a valid vmu unit/port */
    if (default_path[5] < 'a'
	|| default_path[5] > 'd'
	|| default_path[6] < '1'
	|| default_path[6] > '4'
	|| default_path[7] != '/') {
      return 0;
    }
    /* Desative path for the time of the copy. */
    vmu_file_default_path[0] = 0;
    /* Copy '\0' but forget first char */
    memcpy(vmu_file_default_path+1, default_path+1, len); 
    /* Set first char (active new path) (atomik !) */
    vmu_file_default_path[0] = '/';
  }
  return vmu_file_default_path;
}

int vmu_file_init(void)
{
  int err = 0;
  if (init) return 0;

  next_hdl = hdl = 0;
  spinlock_init(&mutex);
  thd = 0;
  cur_status = VMU_TRANSFERT_INIT_ERROR;
  header_size = 0;
  vmu_file_default_path[0] = 0;
  err = fu_size(header_name);
  if (err < 0) {
    SDERROR("[vmu_file_init] : [%s] [%s]\n",
	    header_name, fu_strerr(err));
  } else {
    header_size = err;
    err = 0;
  }

  if (!err) {
    cur_status = VMU_TRANSFERT_ERROR; /* set a global error */
    init = 1;
    if (header_size != 1664) {
      SDNOTICE("[vmu_file_init] : !! Weird !! [%s] [%d bytes]\n",
	       header_name, header_size);
    }
  }
  return init-1;
}

/** Shutdown the vmu file module. */
void vmu_file_shutdown(void)
{
  if (!init) return;
  spinlock_lock(&mutex);
  if (thd) {
  }
  vmu_file_default_path[0] = 0;
  cur_status = VMU_TRANSFERT_ERROR;
  thd = 0;
  init = 0;
}

vmu_trans_hdl_t vmu_file_save(const char * fname, const char * path,
			      int set_default)
{
  vmu_trans_hdl_t ret = 0;
  dcar_option_t opt;
  int err;
  char tmpfile[64];
  int save_right;
  const char * headerfile = "/rd/vmu_header.bin";

  if (!init) return 0;

  tmpfile[0] = 0;

  spinlock_lock(&mutex);
  if (hdl) {
    SDERROR("[vmu_file_save] : already transferring [hdl:%08x].\n", hdl);
    spinlock_unlock(&mutex);
    return 0;
  }
  cur_status = VMU_TRANSFERT_ERROR;
  hdl = ++next_hdl; /* $$$ failed after 2^32 transfert ;) */
  spinlock_unlock(&mutex);

  sprintf(tmpfile,"/ram/vmu%08x", hdl);

  /* Try to remove this file, do not check for error. */
/*   fu_remove(tmpfile); */
  /* Create new temporary file */
/*   hdl = fopen(tmpfile,"wb"); */
/*   spinlock_unlock(&mutex); */

/*   if (!hdl) { */
/*     SDERROR("[vmu_file_save] : failed to create temporary file [%s].\n", */
/* 	    tmpfile); */
/*     return 0; */
/*   } */

  err = fu_copy(tmpfile, headerfile, 1);
  if (err < 0) {
    SDERROR("[vmu_file_save] : error copying header file [%s]\n",
	    fu_strerr(err));
    goto error_free_handle;
  }

  memset(&opt,0,sizeof(opt));
#ifdef DEBUG
  opt.in.verbose = 1;
#endif
  opt.in.compress = 9;
  opt.in.skip = header_size;
  err = dcar_archive(tmpfile, path, &opt);
  if (err < 0) {
    SDERROR("[vmu_file_save] : error creating archive file [%s]\n",
	    opt.errstr);
    goto error_free_handle;
  }

  save_right = vmu_access_right; 
  vmu_access_right |= 2;
  err = fu_copy(fname, tmpfile, 1);
  vmu_access_right = save_right; 
  if (err < 0) {
    SDERROR("[vmu_file_save] : error copying temporary to vmu [%s]\n",
	    fu_strerr(err));
    goto error_free_handle;
  }

  ret = hdl;
  cur_status = VMU_TRANSFERT_SUCCESS;
  if (set_default) {
    vmu_file_set_default(fname);
  }


 error_free_handle:
  spinlock_lock(&mutex);
  hdl = 0;
  spinlock_unlock(&mutex);
  if (tmpfile[0]) {
    fu_remove(tmpfile);
  }
  return ret;
}

vmu_trans_hdl_t vmu_file_load(const char * fname, const char * path,
			      int set_default)
{
  vmu_trans_hdl_t ret = 0;
  dcar_option_t opt;
  int err;

  if (!init) return 0;

  spinlock_lock(&mutex);
  if (hdl) {
    SDERROR("[vmu_file_load] : already transferring [hdl:%08x].\n", hdl);
    spinlock_unlock(&mutex);
    return 0;
  }
  cur_status = VMU_TRANSFERT_ERROR;
  hdl = ++next_hdl; /* $$$ failed after 2^32 transfert ;) */
  spinlock_unlock(&mutex);

  /* Extract */
  memset(&opt,0,sizeof(opt));
#ifdef DEBUG
  opt.in.verbose = 1;
#endif
  opt.in.skip = header_size;
  err = dcar_extract(fname, path, &opt);
  if (err < 0) {
    printf("[vmu_load_file] failed [%s]\n",opt.errstr);
    goto error_free_handle;
  }
  ret = hdl;
  cur_status = VMU_TRANSFERT_SUCCESS;
  if (set_default) {
    vmu_file_set_default(fname);
  }

 error_free_handle:
  spinlock_lock(&mutex);
  hdl = 0;
  spinlock_unlock(&mutex);
  return ret;
}

vmu_trans_status_e vmu_file_status(vmu_trans_hdl_t transfer)
{
  if (!init) {
    return VMU_TRANSFERT_INIT_ERROR;
  }
  if (transfer && transfer == next_hdl) {
    return cur_status;
  }
  return VMU_TRANSFERT_INVALID_HANDLE;
}

const char * vmu_file_statusstr(vmu_trans_status_e status)
{
  const char * s = "unknown";

  switch(status) {
  case VMU_TRANSFERT_SUCCESS:
    s = "success";
    break;
  case VMU_TRANSFERT_READ:
    s = "read";
    break;
  case VMU_TRANSFERT_WRITE:
    s = "write";
    break;
  case VMU_TRANSFERT_BUSY:
    s = "busy";
    break;
  case VMU_TRANSFERT_INVALID_HANDLE:
    s = "invalid handle";
    break;
  case VMU_TRANSFERT_INIT_ERROR:
    s = "not initialized";
    break;
  case VMU_TRANSFERT_ERROR:
    s = "error";
    break;
  }
  return s;
}
