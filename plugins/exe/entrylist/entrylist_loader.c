/**
 * @ingroup  exe_plugin
 * @file     entrylist_loader.c
 * @author   benjamin gerard <ben@sashipa.com> 
 * @date     2002/10/23
 * @brief    entry-list lua extension plugin
 * 
 * $Id: entrylist_loader.c,v 1.1 2002-10-24 18:57:07 benjihan Exp $
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
static el_list_t * loader_el = 0;
static const char * loader_path, * loader_save_path;


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
	  int err;
	  fu_dirent_t * dir;

	  printf("entrylist_loader_thread LOADING '%s'\n", loader_el->path);
	  err = fu_read_dir(loader_el->path, &dir, 0);
	  if (err < 0) {
		printf("entrylist_loader_thread '%s' : %s\n",
			   loader_el->path, fu_strerr(err));
		loader_el->path = loader_save_path;
	  } else {
		int i;
		entrylist_clear(loader_el);
		printf("ENTRIES=%d\n", err);
		for (i=0; i<err; ++i) {
		  el_entry_t e;
		  int namelen, eltsize;

		  printf("#%03d '%s' '%d'\n",i, dir[i].name, dir[i].size);

		  wait_a_while(100);


		  e.type = filetype_get(dir[i].name, dir[i].size);
		  e.size = dir[i].size;
		  e.iname = 0;
		  e.ifile = 0;
		  namelen = strlen(dir[i].name)+1;
		  eltsize = sizeof(e) - sizeof(e.buffer) + namelen;
		  memcpy(e.buffer, dir[i].name, namelen);
		  if (entrylist_set(loader_el, i, &e, eltsize) < 0) {
			printf("entrylist_loader_thread : set entry #%d '%s' failed\n",
				   i, dir[i].name);
			break;
		  }
		}
	  }
	  /* Finish loading. */
	  loader_el->loading = 0;
	  loader_path = 0;
	  loader_save_path = 0;
	  loader_el = 0;

	  if (dir) {
		free(dir);
	  }
	  loader_status = LOADER_READY;
	} break;

	case LOADER_STARTING:
	  printf("entrylist_loader_thread STARTING\n");
	  printf("entrylist_loader_thread : loading (%p,%s)\n",
			 loader_el, loader_path);

	  printf("entrylist_loader_thread : lock entry\n");
	  entrylist_lock(loader_el);

	  loader_el->loading = 1;
	  loader_save_path = loader_el->path;
	  loader_el->path = strdup(loader_path);
	  if (!loader_el->path) {
		printf("entrylist_loader_thread : entry-list path alloc error\n");
		loader_el->path = loader_save_path;
		loader_save_path = 0;
		loader_el->loading = 0;
		entrylist_unlock(loader_el);
		loader_el = 0;
		loader_status = LOADER_READY;
	  } else {
		printf("entrylist_loader_thread : entry locked\n");
		printf("entrylist_loader_thread : unlock entry\n");
		entrylist_unlock(loader_el);
		printf("entrylist_loader_thread : entry unlocked\n");
		loader_status = LOADER_LOADING;
	  }
	}
  }
  printf("loader_thread QUIT recieved...\n");
  loader_status = LOADER_DEAD;
}


int el_loader_loaddir(el_list_t * el, const char * path, el_filter_f filter,
						   int selected)
{
  printf("entrylist_loader_loaddir (%p,%s,%d)\n",el,path,selected);

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
  loader_el = el;
  loader_path = path;
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
  spinlock_init(&loader_mutex);
  loader_el = 0;
  loader_path = 0;
  loader_save_path = 0;
  loader_status = LOADER_INIT;
  loader_thd = 0;
  loader_sem = sem_create(0);
  if (!loader_sem) {
	printf("entrylist_load_init : can not create semaphore.\n");
	return -1;
  }
  loader_thd = thd_create(loader_thread, loader_sem);
  if (!loader_thd) {
	sem_signal(loader_sem);
	sem_destroy(loader_sem);
	loader_sem = 0;
	printf("entrylist_load_init : can not create loader thread.\n");
	return -1;
  }
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
