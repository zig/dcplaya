#include <kos.h>

#include "playa.h"
#include "inp_driver.h"
#include "sndstream.h"

#include "pcm_buffer.h"
#include "driver_list.h"
#include "file_wrapper.h"
#include "fifo.h"

//#include "id3tag.h"


#define PLAYA_THREAD


#define VCOLOR(R,G,B) //vid_border_color(R,G,B)

static int current_frq, next_frq;
static int current_stereo;

static inp_driver_t * driver = 0;

/** INFO */
static void dump_info();
static int info_id;
static playa_info_t curinfo;
static decoder_info_t decinfo;
static void free_info(playa_info_t *i);
int playa_info(playa_info_t * info, const char *fn);

kthread_t * playa_thread;
static semaphore_t *playa_haltsem;
static int playavolume = 255;
static volatile int playastatus = PLAYA_STATUS_INIT;
static volatile unsigned int play_samples_start;
static volatile unsigned int play_samples;
static volatile int streamstatus = PLAYA_STATUS_INIT;

const char * playa_statusstr(int status);

/** INFO **/

static spinlock_t infomutex;

static void lockinfo(void)
{
  spinlock_lock(&infomutex);
}

void unlockinfo(void)
{
  spinlock_unlock(&infomutex);
}

playa_info_t * playa_info_lock() {
  lockinfo();
  return &curinfo;
}

void playa_info_release(playa_info_t *i) {
  if (i == &curinfo) {
    unlockinfo();
  }
}

static int out_buffer[1<<14];
static int out_samples;
static int out_count;

void playa_get_buffer(int **b, int *samples, int * counter, int * frq)
{
  *b = (int *)out_buffer;
  *samples = out_samples;
  *counter = out_count;
  *frq = current_frq;
}


static unsigned int fade_out(int *d, int n, int last)
{
  if (n<=0) return last;

  if (!last) {
    do{
      *d++ = 0;
    } while (--n);
  } else {
    do {
      const int f = 0xf00;
      int l = (signed short)last;
      int r = last>>16;
      l = (l * f) >> 12;
      r = (r * f) >> 12;
      *d++ = last = (r << 16) | (l & 0xFFFF);
    } while (--n);
  }
  return last;
}

static void * sndstream_callback(int size)
{
  int last_sample = 0;

  int n;
  VCOLOR(255,0,0);

  size >>= 2;
  n = fifo_read(out_buffer, size);

  VCOLOR(0,0,0);
  if (n < 0) {
    return 0;
  } else {
    int pbs = play_samples_start;
    if (pbs) {
      pbs -= n;
      if (pbs < 0) {
        /* We reach the new music in the fifo.
            We had to change stream_parameters */
        if (next_frq != current_frq) {
          dbglog(DBG_DEBUG, "^^ On the fly sampling change : %d->%d\n",
          current_frq, next_frq);
          stream_frq(current_frq = next_frq);
        }
        play_samples = -pbs-n;
        pbs = 0;
      }
      play_samples_start = pbs;
    }
    play_samples += (unsigned int)n;
    if (n > 0) {
      last_sample = out_buffer[n-1];
    }
    last_sample = fade_out(out_buffer+n, size-n, last_sample);
  }
  out_samples = size;
  out_count++;
  return out_buffer;
}

void sndstream_thread(void *cookie)
{
  dbglog(DBG_DEBUG, ">> " __FUNCTION__"()\n");

  streamstatus = PLAYA_STATUS_STARTING;

  stream_init(sndstream_callback, 1<<14);
  stream_start(1200, current_frq=44100, playavolume, current_stereo=1);
  //  stream_start(256, 44100, playavolume, 1);

  streamstatus = PLAYA_STATUS_PLAYING;
  while (streamstatus == PLAYA_STATUS_PLAYING) {
    int e;

    e = stream_poll();
    thd_pass();

#if 0
    if (!e) {
      /* nothing happened */
      //thd_pass();
    } else if (e<0) {
      /* callback has return a null pointer */
    } else {
      /* audio data has been sent to hardware*/
    }
#endif
  }

  /* This is done by the stop function :
     streamstatus = PLAYA_STATUS_STOPPING;
  */

  stream_stop();
  stream_shutdown();

  streamstatus = PLAYA_STATUS_ZOMBIE;
  dbglog(DBG_DEBUG, "<< " __FUNCTION__"()\n");
}


#ifdef PLAYA_THREAD
static
#endif
void playa_update(void)
{
#ifdef PLAYA_THREAD
  thd_pass();
#else
  switch (playastatus) {

    case PLAYA_STATUS_INIT:
      playastatus = PLAYA_STATUS_READY;
      break;

    case PLAYA_STATUS_READY:
      dbglog(DBG_DEBUG,
	     "** " __FUNCTION__ " : sndserver: waiting on semaphore\r\n");
      //sem_wait(playa_haltsem);
      dbglog(DBG_DEBUG,
	     "** " __FUNCTION__
	     " : sndserver: released from semaphore\r\n");
      break;

    case PLAYA_STATUS_STARTING:
      playastatus = PLAYA_STATUS_PLAYING;
      break;

    case PLAYA_STATUS_REINIT:
      /* Re-initialize streaming driver */
      /* ??? */
      playastatus = PLAYA_STATUS_READY;
      break;

    case PLAYA_STATUS_PLAYING:
      {
	int sj;

	sj = driver ? driver->decode(&decinfo) : -1;
	if (!sj) {
	  thd_pass();
	} else if (sj < 0) {
	  playastatus = PLAYA_STATUS_STOPPING;
	}
      } break;

    case PLAYA_STATUS_QUIT:
      break;

    case PLAYA_STATUS_STOPPING:
      if (driver) {
	driver->stop();
      }
      lockinfo();
      free_info(&curinfo);
      unlockinfo();
      playastatus = PLAYA_STATUS_READY;
      break;
    }
#endif
}


#ifdef PLAYA_THREAD
void playadecoder_thread(void *blagh)
{
  int quit = 0;
  int oldstatus = playastatus;

  playa_thread = thd_current;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "\n");

  /* Main command loop */
  while (!quit) {
    if (playastatus != oldstatus) {
      dbglog(DBG_DEBUG, "** " __FUNCTION__ " : STATUS [%s] -> [%s]\r\n",
	     playa_statusstr(oldstatus), playa_statusstr(playastatus));
      oldstatus = playastatus;
    }

    switch (playastatus) {

    case PLAYA_STATUS_INIT:
      playastatus = PLAYA_STATUS_READY;
      break;

    case PLAYA_STATUS_READY:
      dbglog(DBG_DEBUG,
	     "** " __FUNCTION__ " : sndserver: waiting on semaphore\r\n");
      sem_wait(playa_haltsem);
      dbglog(DBG_DEBUG,
	     "** " __FUNCTION__
	     " : sndserver: released from semaphore\r\n");
      break;

    case PLAYA_STATUS_STARTING:
      playastatus = PLAYA_STATUS_PLAYING;
      break;

    case PLAYA_STATUS_REINIT:
      /* Re-initialize streaming driver */
      /* ??? */
      playastatus = PLAYA_STATUS_READY;
      break;

    case PLAYA_STATUS_PLAYING:
      {
	int sj;

	sj = driver ? driver->decode(&decinfo) : -1;
	if (!sj) {
	  thd_pass();
	} else if (sj < 0) {
	  playastatus = PLAYA_STATUS_STOPPING;
	}
      } break;

    case PLAYA_STATUS_QUIT:
      quit = 1;
      break;

    case PLAYA_STATUS_STOPPING:
      if (driver) {
	driver->stop();
      }
      lockinfo();
      free_info(&curinfo);
      unlockinfo();
      playastatus = PLAYA_STATUS_READY;
      break;
    }
  }

  /* Done: clean up */
  playastatus = PLAYA_STATUS_ZOMBIE;
  dbglog(DBG_DEBUG, "<< " __FUNCTION__ "\n");
}
#endif

int playa_init()
{
  int e = 0;
  dbglog(DBG_DEBUG, ">> " __FUNCTION__"\n");

  pcm_buffer_init(0,0);

  memset(&curinfo, 0, sizeof(curinfo));
  spinlock_init(&infomutex);
  fifo_init();
  playa_haltsem = sem_create(0);

  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Create soundstream thread\n");
  streamstatus = PLAYA_STATUS_INIT;
  thd_create(sndstream_thread, 0);
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Waiting soundstream thread\n");
  while (streamstatus != PLAYA_STATUS_PLAYING)
    thd_pass();
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : READY soundstream thread\n");

  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Create PLAYA decoder thread\n");
  playastatus = PLAYA_STATUS_INIT;
#ifdef PLAYA_THREAD
  thd_create(playadecoder_thread, 0);
#endif
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : Waiting PLAYA decoder thread\n");
  while (playastatus != PLAYA_STATUS_READY)
    playa_update();
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : READY PLAYA decoder thread\n");

  dbglog(DBG_DEBUG, "<< " __FUNCTION__ " := %d\n", e);
  return e;
}

int playa_shutdown()
{
  int e = 0;
  dbglog(DBG_DEBUG, ">> " __FUNCTION__"\n");

  /* PLAYA decoder */
  dbglog(DBG_DEBUG, "** " __FUNCTION__" : Decoder stream\n");
  playastatus = PLAYA_STATUS_QUIT;
  sem_signal(playa_haltsem);
  while (playastatus != PLAYA_STATUS_ZOMBIE)
    playa_update();
  sem_destroy(playa_haltsem);

  /* Sound Stream */
  dbglog(DBG_DEBUG, "** " __FUNCTION__" : Shutdown sound stream\n");
  if (streamstatus == PLAYA_STATUS_PLAYING) {
    dbglog(DBG_DEBUG, ">> " __FUNCTION__" : Waiting sound stream stop\n");
    streamstatus = PLAYA_STATUS_STOPPING;
    while (streamstatus != PLAYA_STATUS_ZOMBIE)
      ;
    dbglog(DBG_DEBUG, ">> " __FUNCTION__" : STOPPED\n");
  }

  /* $$$ May be it is not the better place to do that :
     driver was initialized in an other file */
  {
    inp_driver_t *d;
    for (d=(inp_driver_t *)inp_drivers.drivers;
	 d;
	 d=(inp_driver_t *)d->common.nxt) {
      e |= d->common.shutdown(&d->common);
    }
  }

  lockinfo();
  free_info(&curinfo);
  unlockinfo();
  dbglog(DBG_DEBUG, "<< " __FUNCTION__" := %d\n", e);

  return e;
}

int playa_isplaying() {
  return playastatus == PLAYA_STATUS_PLAYING;
}

int playa_status() {
  return playastatus;
}

const char * playa_statusstr(int status) {
  static const char * statusstr[] =
  {
    "INIT",
    "READY",
    "STARTING",
    "PLAYING",
    "STOPPING",
    "QUIT",
    "ZOMBIE",
    "REINIT",
  };
  return (status < 0 || status > 7) ? "???" : statusstr[status];
}

static int ToUpper(int c)
{
  if (c>='a' && c<='z') c ^= 32;
  return c;
}

static int ToLower(int c)
{
  if (c>='A' && c<='Z') c ^= 32;
  return c;
}

static int StrLen(const char *s)
{
  return s ? strlen(s) : 0;
}

static char * frq_1000(int frq, const char *append)
{
  static char s[16], *s2;
  int v;

  v = frq / 1000;
  sprintf(s,"%d",v);
  s2 = s + strlen(s);
  v = frq % 1000;
  if (v) {
    int div;

    *s2++='.';
    for (div=100; v && div>0; div /= 10) {
      int d = v / div;
      *s2++ = '0' + d;
      v -= d * div;
    }
  }
  *s2=0;
  if (append) {
    strcpy(s2, append);
  }
  return s;
}


int playa_info(playa_info_t * info, const char *fn)
{
  return driver ? driver->info(info, fn) : -1;
}

int playa_stop(int flush)
{
  dbglog(DBG_DEBUG, ">> " __FUNCTION__"\n");

  /* Already stopped */
  if (playastatus == PLAYA_STATUS_READY) {
    return 0;
  }

  /* Ask thread to stop */
  playastatus = PLAYA_STATUS_STOPPING;
  while (playastatus != PLAYA_STATUS_READY)
    playa_update();

  /* If flush : clean PCM FIFO */
  if (flush) {
    fifo_start(0);
    play_samples = play_samples_start = 0;
  }
  driver = 0;
  dbglog(DBG_DEBUG, "<< " __FUNCTION__"\n");

  return 0;
}

static char * make_time_str(unsigned int ms)
{
  char time[16];
  unsigned int h,m,s;

  h = ms / (3600*1000);
  ms -= h * (3600*1000);
  m = ms / (60*1000);
  ms -= m * (60*1000);
  s = ms / 1000;

  if (h) {
    sprintf(time, "%d:%02d:%02d",h,m,s);
  } else {
    sprintf(time, "%02d:%02d",m,s);
  }
  return strdup(time);
}

static void make_decoder_info(playa_info_t * info,
			     const decoder_info_t * decinfo)
{
  char tmp[256];

  strcpy(tmp, frq_1000(decinfo->bps, "kbps "));
  strcat(tmp, decinfo->desc);
  strcat(tmp, " ");
  strcat(tmp, frq_1000(decinfo->frq, "khz"));
  info->format = strdup(tmp);
  info->time = make_time_str(decinfo->time);
}

static void make_global_info(playa_info_t * info)
{
  char tmp[1024];
  char **p, *d;
  int i;
  static char *names[] = {
    "format", "time",
    "artist", "album",
    "track", "title",
    "year", "genre",
    "comments",
    0
  };

  strcpy(tmp, "            ");
  for (d=tmp+strlen(tmp), i=0, p=&info->format; names[i];
       ++p, ++i) {
    char *s;
    int c;

    if (!*p) continue;

    s = names[i];
    *d++ = '*';
    *d++ = '*';
    *d++ = ' ';
    while (c = ToUpper(*s++), c) {
      *d++ = c;
    }
    *d++ = ':';
    *d++ = ' ';
    s = *p;
    while (c = ToUpper(*s++), c) {
      *d++ = c;
    }
    *d++ = ' ';
    *d++ = ' ';
    *d++ = ' ';
  }
  *d++ = 0;
  info->info = strdup(tmp);
}

int playa_start(const char *fn, int immediat) {
  int e;
  playa_info_t        info;
  inp_driver_t *d;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__" ('%s',%d)\n",fn,immediat);

  /* $$$ Try to find a driver for this file. A quick glance at file extension
     will be suffisant right now. Later the driver should support an is_mine()
     function. */
  d = inp_driver_list_search_by_extension(fn);
  /* $$$ Here, the previous play is not stopped. May be songmenu.c expects
     it is !!! */
  if (!d) {
    dbglog(DBG_DEBUG, "!! " __FUNCTION__ ": No driver for this file !\n");
    return -1;
  } else {
    dbglog(DBG_DEBUG, "!! " __FUNCTION__ ": Driver [%s] found\n",
	   d->common.name);
  }

  // Can't start again if already playing
  playa_stop(immediat);

  //$$$
  if (!immediat) {
    int pbs = fifo_used();

    if (pbs < 0) {
      dbglog(DBG_DEBUG,"**** " __FUNCTION__ "() : fifo used : %d !!!\n", pbs);
      pbs = 0;
    }
    play_samples_start = pbs;
  } else {
    play_samples = play_samples_start = 0;
  }

  /* Start playa, get decoder info */
  driver = d;
  e = driver->start(fn, &decinfo);

  if (e) {
    lockinfo();
    free_info(&curinfo);
    unlockinfo();
    goto error;
  }

  // Get ID3 info in temp info
  playa_info(&info, fn);

  /* Setup decode info */
  make_decoder_info(&info, &decinfo);

  /* Set sampling rate for next music */
  if (!immediat) {
    next_frq = decinfo.frq;
  } else {
    stream_frq(current_frq = decinfo.frq);
  }

  make_global_info(&info);

  // Copy and valid new info when it is done.
  lockinfo();
  info.valid = curinfo.valid = 0;
  curinfo = info;
  curinfo.valid = ++info_id;
  unlockinfo();

  /* Wait for player thread to be ready */
  while (playastatus != PLAYA_STATUS_READY)
    playa_update();

  /* Tell it to start */
  playastatus = PLAYA_STATUS_STARTING;
  sem_signal(playa_haltsem);
  while (playastatus == PLAYA_STATUS_STARTING)
    playa_update();

 error:
  dump_info();
  dbglog(DBG_DEBUG, "<< " __FUNCTION__" ('%s',%d) := %d \n",fn, immediat, e);
  return e;
}


int playa_volume(int volume) {
  int old = playavolume;
  if (volume >= 0) stream_volume(playavolume = volume);
  return old;
}

unsigned int playa_playtime()
{
  return (long long)play_samples * 1000 / current_frq;
}


static void remove_extension(char *n)
{
  int len;
  char * s;

  s = strrchr(n,'.');
  if (!s) return;

  len = strlen(s);
  if (len <= 5 && s != n) {
    *s = 0;
  }
}

static void make_default_name(char * dest, int max, const char *fn)
{
  const char *s;

  /* Remove path */
  s = strrchr(fn,'/');
  if (!s) {
    s = fn;
  }
  while (*fn && max > 0) {
    int c = *fn++;
    if (c == '_') c = ' ';
    *dest++ = c;
    --max;
  }
  *dest = 0;
  remove_extension(dest);
}

static void free_info(playa_info_t *info)
{
  if(info->info)      free(info->info);
  if(info->format )   free(info->format);
  if(info->time)      free(info->time);
  if(info->album)     free(info->album);
  if(info->title)     free(info->title);
  if(info->artist)    free(info->artist);
  if(info->year)      free(info->year);
  if(info->genre)     free(info->genre);
  if(info->track)     free(info->track);
  if(info->comments)  free(info->comments);
  memset(info,0,sizeof(playa_info_t));
}


static char *null(char *a) {
  return a ? a : "<null>";
}

static void dump_info() {
  playa_info_t *info = &curinfo;

  lockinfo();
  if (info->valid) {

    dbglog(DBG_DEBUG,  " FORMAT   : %s\n", null(info->format));
    dbglog(DBG_DEBUG,  " TIME     : %s\n", null(info->time));

    dbglog(DBG_DEBUG,  " ALBUM    : %s\n", null(info->album));
    dbglog(DBG_DEBUG,  " TITLE    : %s\n", null(info->title));
    dbglog(DBG_DEBUG,  " ARTIST   : %s\n", null(info->artist));
    dbglog(DBG_DEBUG,  " YEAR     : %s\n", null(info->year));
    dbglog(DBG_DEBUG,  " GENRE    : %s\n", null(info->genre));
    dbglog(DBG_DEBUG,  " TRACK    : %s\n", null(info->track));
    dbglog(DBG_DEBUG,  " COMMENTS : %s\n", null(info->comments));
  }
  unlockinfo();
}
