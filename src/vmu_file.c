/**
 * @file    vmu_file.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2003/03/10
 * @brief   VMU file load and save function.
 *
 * $Id: vmu_file.c,v 1.1 2003-03-10 22:51:48 ben Exp $
 */

#include <kos/thread.h>
#include <arch/spinlock.h>
#include <stdio.h>
#include <stdlib.h>

#include "dcplaya/config.h"
#include "sysdebug.h"
#include "vmu_file.h"
#include "dcar.h"
#include "file_utils.h"

static volatile vmu_trans_hdl_t hdl, next_hdl;
static spinlock_t mutex;
static kthread_t * thd; 
static volatile vmu_trans_status_e cur_status;

extern volatile int vmu_access_right;

int vmu_file_init(void)
{
  next_hdl = hdl = 0;
  spinlock_init(&mutex);
  thd = 0;
  cur_status = VMU_TRANSFERT_ERROR;
  return 0;
}

/** Shutdown the vmu file module. */
void vmu_file_shutdown(void)
{
  spinlock_lock(&mutex);
  if (thd) {
  }
  cur_status = VMU_TRANSFERT_ERROR;
  thd = 0;
}

vmu_trans_hdl_t vmu_file_save(const char * fname, const char * path)
{
  vmu_trans_hdl_t ret = 0;
  dcar_option_t opt;
  int err;
  char tmpfile[64];
  int save_right;
  const char * headerfile = "/rd/vmu_header.bin";

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
  opt.in.skip = 1664;
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

 error_free_handle:
  spinlock_lock(&mutex);
  hdl = 0;
  spinlock_unlock(&mutex);
  if (tmpfile[0]) {
    fu_remove(tmpfile);
  }
  return ret;
}

vmu_trans_hdl_t vmu_file_load(const char * fname, const char * path)
{
  vmu_trans_hdl_t ret = 0;
  dcar_option_t opt;
  int err;

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
  opt.in.skip = 1664;
  err = dcar_extract(fname, path, &opt);
  if (err < 0) {
    printf("[vmu_load_file] failed [%s]\n",opt.errstr);
    goto error_free_handle;
  }
  ret = hdl;
  cur_status = VMU_TRANSFERT_SUCCESS;

 error_free_handle:
  spinlock_lock(&mutex);
  hdl = 0;
  spinlock_unlock(&mutex);
  return ret;
}

vmu_trans_status_e vmu_file_status(vmu_trans_hdl_t transfer)
{
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
  case VMU_TRANSFERT_ERROR:
    s = "error";
    break;
  }
  return s;
}
