/**
 * @ingroup  exe_plugin
 * @file     entrylist_loader.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_loader.c,v 1.6 2003-01-06 14:54:34 ben Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kos/thread.h>
#include <kos/sem.h>
#include "entrylist.h"
#include "entrylist_loader.h"

#include "file_utils.h"
#include "filetype.h"

#include "sysdebug.h"

typedef enum {
  LOADER_INIT = 0,
  LOADER_DEAD,
  LOADER_READY,
  LOADER_STARTING,
  LOADER_LOADING,
  LOADER_QUIT
} loader_status_e;

static semaphore_t * loader_sem = 0;
static spinlock_t loader_mutex;
static volatile loader_status_e loader_status = 0;
static kthread_t * loader_thd = 0; 

static struct loader_cookie_s {
  int filter;
  el_list_t  * el;
  el_path_t  * save_path;
  const char * path;
} loader;

static void wait_a_while(unsigned int timeout)
{
  timeout = jiffies + 500;
  while (jiffies < timeout) {
    thd_pass();
  }
}

static int loader_waitstatus(loader_status_e status, unsigned int timeout)
{
  if (timeout) {
    timeout += jiffies;
  }
  while (status != loader_status && (!timeout || jiffies < timeout)) {
    thd_pass();
  }
  if (status != loader_status) {
    printf("entrylist_loader : waiting on status %d timeout. status is %d.\n",
	   status, loader_status);
  }
  return status == loader_status;
}

static int loader_waitnotstatus(loader_status_e status, unsigned int timeout)
{
  if (timeout) {
    timeout += jiffies;
  }
  while (status == loader_status && (!timeout || jiffies < timeout)) {
    thd_pass();
  }
  if (status == loader_status) {
    printf("entrylist_loader : waiting on not status %d timeout..\n", status);
  }
  return status != loader_status;
}


static int loader_addentry_any(const fu_dirent_t * dir, void * cookie)
{
  el_list_t * el = (el_list_t *)cookie;
  el_entry_t e;
  int namelen, eltsize;
  int i;

  e.type = filetype_get(dir->name, dir->size);
  e.size = dir->size;
  e.iname = 0;
  e.ifile = 0;
  e.path = elpath_addref(el->path);
  namelen = strlen(dir->name)+1;
  eltsize = sizeof(e) - sizeof(e.buffer) + namelen;
  memcpy(e.buffer, dir->name, namelen);
  i = entrylist_insert(el, -1, &e, eltsize) < 0 ? -1 : 1;
  if (i < 0) {
    elpath_del(e.path);
  }
  return i;
}


static int loader_addentry(const fu_dirent_t * dir, void * cookie)
{
  struct loader_cookie_s * loader = (struct loader_cookie_s *)cookie;
  el_list_t * el = loader->el;
  el_entry_t e;
  int namelen, eltsize;
  int i;
  int type; 

  /*   SDDEBUG("[%s] [%s,%d]\n", __FUNCTION__, dir->name, dir->size); */
  e.type = filetype_get(dir->name, dir->size);
  /*   SDDEBUG("[%s] [%s,%d,%04x]\n", __FUNCTION__, dir->name, dir->size, e.type); */

  if (e.type < filetype_dir) {
    /* 	SDDEBUG("skipping [%s,%04x]\n",dir->name, e.type); */
    /* Remove error, root, '.', '..' */
    return 0;
  }
  type = FILETYPE_MAJOR_NUM(e.type);

  if (! (loader->filter & (1<<type))) {
    /* 	SDDEBUG("filter [%s,%04x]\n",dir->name, e.type); */
    /* Filter major type */
    return 0;
  }

  e.size = dir->size;
  e.iname = 0;
  e.ifile = 0;
  e.path = elpath_addref(el->path);
  namelen = strlen(dir->name)+1;
  eltsize = sizeof(e) - sizeof(e.buffer) + namelen;
  memcpy(e.buffer, dir->name, namelen);
  i = entrylist_insert(el, -1, &e, eltsize) < 0 ? -1 : 1;
  if (i < 0) {
    elpath_del(e.path);
  }
  return i;
}

static void loader_thread(void *cookie)
{
  static loader_status_e old_status = -1;
  semaphore_t * sem = (semaphore_t *)cookie;

  printf("loader_thread starting\n");

  while (loader_status != LOADER_QUIT) {
    if (loader_status != old_status) {
      printf("loader_thread status %d -> %d\n", old_status, loader_status);
      old_status =loader_status;
    }

    switch (loader_status) {

    case LOADER_DEAD:
      printf("entrylist_loader_thread DEAD !!!\n");
      return;

    case LOADER_INIT:
      printf("entrylist_loader_thread INIT\n");
      loader_status = LOADER_READY;
      break;

    case LOADER_READY:
      printf("entrylist_loader_thread READY\n");
      printf("entrylist_loader_thread : waiting on semaphore.\n");
      sem_wait(sem);
      printf("entrylist_loader_thread : released from semaphore.\n");
      break;

    case LOADER_LOADING: {
      el_path_t * path = loader.el->path;
      printf("entrylist_loader_thread LOADING\n");

      if (!fu_is_dir(path->path)) {
	printf("entrylist_loader_thread : not a directory '%s'\n", path->path);
	elpath_del(path);
	loader.el->path = loader.save_path;  /* $$$ Atomik op */
	loader.save_path = 0;
      } else {
	int j;
	static struct {
	  const char * file, * name;
	} files[] = {
	  {"..", "<parent>" },
	  {".", "<refresh>" },
	  {0,0}
	};

	entrylist_clear(loader.el);
	entrylist_lock(loader.el);

	for (j = !strcmp(path->path,"/") ; files[j].file; ++j) {
	  el_entry_t e;
	  int eltsize, l;

	  eltsize = sizeof(e) - sizeof(e.buffer);

	  e.ifile=0;
	  l = strlen(files[j].file) + 1;
	  memcpy(e.buffer,files[j].file, l);

	  e.iname = l;
	  l = strlen(files[j].name) + 1;
	  memcpy(e.buffer+e.iname, files[j].name, l);

	  eltsize += e.iname + l;
	  e.type = filetype_directory(files[j].file);
	  e.size = -1;
	  e.path = elpath_addref(path);
	  if (entrylist_insert(loader.el, -1, &e, eltsize) < 0) {
	    elpath_del(path);
	  }
	}
	entrylist_unlock(loader.el);
	
	printf("ENTERING CB [%s]\n",path->path);
	{
	  int err = 
	    fu_read_dir_cb(path->path, loader_addentry, &loader);
	  if (err < 0) {
	    printf("Error loading [%s] : %s\n", path->path, fu_strerr(err));
	  } else {
	    printf("-> %d entries added\n", err);
	  }
	}
	printf("EXITING CB\n");
      }
      /* Finish loading. */
      loader.el->loading = 2; /* $$$ Atomik op */
      elpath_del(loader.save_path);
      loader.save_path = 0;
      loader.el = 0;
      loader_status = LOADER_READY;
    } break;

    case LOADER_STARTING:
      printf("entrylist_loader_thread STARTING\n");
      printf("entrylist_loader_thread : loading (%p,%s)\n",
	     loader.el, loader.path);

      printf("entrylist_loader_thread : lock entry\n");
      entrylist_lock(loader.el);

      loader.el->loading = 1;
      loader.save_path = loader.el->path;
      loader.el->path = elpath_add(loader.path);
      loader.path = 0;

      if (!loader.el->path) {
	printf("entrylist_loader_thread : entry-list path alloc error\n");
	loader.el->path = loader.save_path;
	loader.save_path = 0;
	loader.el->loading = -1;
	entrylist_unlock(loader.el);
	loader.el = 0;
	loader_status = LOADER_READY;
      } else {
	printf("entrylist_loader_thread : entry locked\n");
	printf("entrylist_loader_thread : unlock entry\n");
	entrylist_unlock(loader.el);
	printf("entrylist_loader_thread : entry unlocked\n");
	loader_status = LOADER_LOADING;
      }
    }
  }
  printf("loader_thread QUIT recieved...\n");
  loader_status = LOADER_DEAD;
}


int el_loader_loaddir(el_list_t * el, const char * path, int filter)
{
  printf("entrylist_loader_loaddir (%p,%s,%x)\n",el,path,filter);

  if (!loader_thd) {
    printf("entrylist_loader_loaddir : no loader thread\n");
    return -1;
  }
	
  printf("entrylist_loader_loaddir : status = %d\n", loader_status);
  printf("entrylist_loader_loaddir : waiting on loader\n");
  spinlock_lock(&loader_mutex);
  printf("entrylist_loader_loaddir : get loader !!\n");

  printf("entrylist_loader_loaddir : waiting loader ready\n");
  if(!loader_waitstatus(LOADER_READY, 1000)) {
    spinlock_unlock(&loader_mutex);
    printf("entrylist_loader_loaddir : timeout waiting on loader\n");
    return -1;
  }

  printf("entrylist_loader_loaddir : loader ready\n");
  printf("entrylist_loader_loaddir : lock entrylist\n");
  entrylist_lock(el);
  printf("entrylist_loader_loaddir : locked\n");
  loader.el = el;
  loader.path = path;
  loader.filter = filter;
  loader_status = LOADER_STARTING;
  printf("entrylist_loader_loaddir : ask a loaddir\n");
  sem_signal(loader_sem);

  /*   { */
  /* 	unsigned int timeout = jiffies + 500; */
  /* 	printf("entrylist_loader_loaddir : test wait loop\n"); */
  /* 	while (jiffies < timeout) { */
  /* 	  thd_pass(); */
  /* 	} */
  /* 	printf("entrylist_loader_loaddir : wait loop completed\n"); */
  /*   } */
  entrylist_unlock(el);

  printf("entrylist_loader_loaddir : waiting on loader to really start \n");
  loader_waitnotstatus(LOADER_STARTING, 0);
  printf("entrylist_loader_loaddir : started status = %d\n", loader_status);

  printf("entrylist_loader_loaddir : release loader\n");
  spinlock_unlock(&loader_mutex);
  
  return 0;
}

int el_loader_init(void)
{
  printf("entrylist_load_init...\n");

  spinlock_init(&loader_mutex);
  memset(&loader, 0, sizeof(loader));
  loader_status = LOADER_INIT;
  loader_thd = 0;
  loader_sem = sem_create(0);
  if (!loader_sem) {
    printf("entrylist_load_init : can not create semaphore.\n");
    return -1;
  }
  printf("entrylist_load_init : create thread.\n");
  loader_thd = thd_create(loader_thread, loader_sem);
  if (!loader_thd) {
    printf("entrylist_load_init : thread failed.\n");
    sem_signal(loader_sem);
    sem_destroy(loader_sem);
    loader_sem = 0;
    printf("entrylist_load_init : can not create loader thread.\n");
    return -1;
  }
  printf("entrylist_load_init : thread created, rename it.\n");
  thd_set_label((kthread_t *)loader_thd, "Loader-thd");
  printf("entrylist_load_init : complete (thd=%p).\n", loader_thd);
  return 0;
}

void el_loader_shutdown(void)
{
  if (!loader_thd) {
    printf("entrylist_loader_shutdown : mo loader_thread!!\n");
    return;
  }
  //  sem_signal(playa_haltsem);
  printf("entrylist_loader_shutdown : waiting for loader thread death.\n");

  loader_status = LOADER_QUIT;
  sem_signal(loader_sem);
  if (!loader_waitstatus(LOADER_DEAD, 500)) {
    printf("entrylist_loader_shutdown : explicit kill of loader thread\n");
    thd_destroy(loader_thd);
  }
  sem_destroy(loader_sem);
  loader_sem = 0;
  loader_thd = 0;
  printf("entrylist_loader_shutdown : complete.\n");
  return;
}
