#include <kos.h>

#include "sysdebug.h"
#include "playa.h"
#include "inp_driver.h"
#include "sndstream.h"

#include "pcm_buffer.h"
#include "driver_list.h"
#include "file_wrapper.h"
#include "fifo.h"

//#include "id3tag.h"


#define PLAYA_THREAD


#define VCOLOR(R,G,B) vid_border_color(R,G,B)

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
static void make_global_info(playa_info_t * info);

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

static void clean_info(playa_info_t *i)
{
  if (i) {
    memset(i,0,sizeof(*i));
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
  //  VCOLOR(255,0,0);

  size >>= 2;
  n = fifo_read(out_buffer, size);

  //  VCOLOR(0,0,0);
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
  SDDEBUG(">> %s()\n", __FUNCTION__);

  streamstatus = PLAYA_STATUS_STARTING;

  stream_init(sndstream_callback, 1<<14);
  stream_start(1200, current_frq=44100, playavolume, current_stereo=1);
  // $$$ Aprox sync VBL
  //stream_start(736, current_frq=44100, playavolume, current_stereo=1);
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
  SDDEBUG("<< %s()\n", __FUNCTION__);
}


static void real_playa_update(void)
{

  switch (playastatus) {

  case PLAYA_STATUS_INIT:
    playastatus = PLAYA_STATUS_READY;
    break;

  case PLAYA_STATUS_READY:
    SDDEBUG("%s() : waiting on semaphore\n", __FUNCTION__);
#ifdef PLAYA_THREAD
    sem_wait(playa_haltsem);
#endif
    SDDEBUG("%s() : released from semaphore\n", __FUNCTION__);
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
      int status;
      //      VCOLOR(255,255,0);
      status = driver ? driver->decode(&decinfo) : INP_DECODE_ERROR;
      //      VCOLOR(0,0,0);

      if (status & INP_DECODE_END) {
	playastatus = PLAYA_STATUS_STOPPING;
	break;
      }

      if (status & INP_DECODE_INFO) {
	playa_info_t info;

	if (!playa_info(&info, 0)) {
	  make_global_info(&info);
	  lockinfo();
	  free_info(&curinfo);
	  info.valid = curinfo.valid = 0;
	  curinfo = info;
	  curinfo.valid = ++info_id;
	  unlockinfo();
	}
      }

      if (! (status & INP_DECODE_CONT)) {
	thd_pass();
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
}

#ifdef PLAYA_THREAD
static
#endif
void playa_update(void)
{
#ifdef PLAYA_THREAD
  thd_pass();
#else
  real_playa_update();
#endif
}


#ifdef PLAYA_THREAD
void playadecoder_thread(void *blagh)
{
  int oldstatus = playastatus;

  playa_thread = thd_current;

  SDDEBUG(">> %s()\n", __FUNCTION__);

  /* Main command loop */
  while (playastatus != PLAYA_STATUS_QUIT) {
    if (playastatus != oldstatus) {
      SDDEBUG("%s() : STATUS [%s] -> [%s]\n", __FUNCTION__,
	      playa_statusstr(oldstatus), playa_statusstr(playastatus));
      oldstatus = playastatus;
    }
    real_playa_update();
  }

  /* Done: clean up */
  playastatus = PLAYA_STATUS_ZOMBIE;
  SDDEBUG("<< %s()\n", __FUNCTION__);
}
#endif

int playa_init()
{
  int e = 0;
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  pcm_buffer_init(0,0);

  clean_info(&curinfo);
  spinlock_init(&infomutex);
  fifo_init();
  playa_haltsem = sem_create(0);

  SDDEBUG("Create soundstream thread\n");
  streamstatus = PLAYA_STATUS_INIT;
  thd_create(sndstream_thread, 0);
  SDDEBUG("Waiting soundstream thread\n");
  while (streamstatus != PLAYA_STATUS_PLAYING)
    thd_pass();
  SDDEBUG("READY soundstream thread\n");

  SDDEBUG("Create PLAYA decoder thread\n");
  playastatus = PLAYA_STATUS_INIT;
#ifdef PLAYA_THREAD
  thd_create(playadecoder_thread, 0);
#endif
  SDDEBUG("Waiting PLAYA decoder thread\n");
  while (playastatus != PLAYA_STATUS_READY) {
    playa_update();
  }
  SDDEBUG("READY PLAYA decoder thread\n");

  SDUNINDENT;
  SDDEBUG("<< %s() := [%d]\n",__FUNCTION__, e);
  return e;
}

int playa_shutdown()
{
  int e = 0;
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  /* PLAYA decoder */
  SDDEBUG("Decoder stream\n");
  playastatus = PLAYA_STATUS_QUIT;
  sem_signal(playa_haltsem);
  while (playastatus != PLAYA_STATUS_ZOMBIE)
    playa_update();
  sem_destroy(playa_haltsem);

  /* Sound Stream */
  SDDEBUG("Shutdown sound stream\n");
  if (streamstatus == PLAYA_STATUS_PLAYING) {
    SDDEBUG("Waiting sound stream stop\n");
    streamstatus = PLAYA_STATUS_STOPPING;
    while (streamstatus != PLAYA_STATUS_ZOMBIE)
      ;
    SDDEBUG("STOPPED\n");
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

  SDUNINDENT;
  SDDEBUG("<< %s() := [%d]\n",__FUNCTION__, e);

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
  int err;
  clean_info(info);
  err = driver ? driver->info(info, fn) : -1;
  if (err < 0) {
    /* $$$ ben: Safety net to be sure the driver does not trash it ... 
     * Memory leaks are possible anyway ... 
     */
    clean_info(info);
  }
  return err;
}

int playa_stop(int flush)
{
  SDDEBUG(">> %s()\n", __FUNCTION__);
  
  /* Already stopped */
  if (playastatus == PLAYA_STATUS_READY) {
    goto end;;
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

 end:

  SDUNINDENT;
  SDDEBUG("<< %s() := [0]\n", __FUNCTION__);

  return 0;
}

char * playa_make_time_str(unsigned int ms)
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
  info->time = playa_make_time_str(decinfo->time);
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
  int e = -1;
  playa_info_t info;
  inp_driver_t *d;

  SDDEBUG(">> %s('%s',%d)\n",__FUNCTION__, fn, immediat);
  SDINDENT;

  /* $$$ Try to find a driver for this file. A quick glance at file extension
     will be suffisant right now. Later the driver should support an is_mine()
     function. */
  d = inp_driver_list_search_by_extension(fn);
  /* $$$ Here, the previous play is not stopped. May be songmenu.c expects
     it is !!! */
  if (!d) {
    SDWARNING("No driver for this file !\n");
    goto error;
  } else {
    SDDEBUG("Driver [%s] found\n", d->common.name);
  }

  // Can't start again if already playing
  playa_stop(immediat);

  //$$$
  if (!immediat) {
    int pbs = fifo_used();

    if (pbs < 0) {
      SDWARNING("fifo used : %d !!!\n", pbs);
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

  SDUNINDENT;
  SDDEBUG("<< %s([%s],%d) := [%d] \n",__FUNCTION__, fn, immediat, e);

  return e;
}

int playa_loaddisk(const char *fn, int immediat)
{
  /* No filename : stop, no error */
  if (!fn) {
    playa_stop(immediat);
    return 0;
  }
  return playa_start(fn,immediat);
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
  if(info->format)    free(info->format);
  if(info->time)      free(info->time);
  if(info->album)     free(info->album);
  if(info->title)     free(info->title);
  if(info->artist)    free(info->artist);
  if(info->year)      free(info->year);
  if(info->genre)     free(info->genre);
  if(info->track)     free(info->track);
  if(info->comments)  free(info->comments);
  clean_info(info);
}


static char *null(char *a) {
  return a ? a : "<null>";
}

static void dump_info() {
  playa_info_t *info = &curinfo;

  lockinfo();
  if (info->valid) {

    SDDEBUG(" FORMAT   : %s\n", null(info->format));
    SDDEBUG(" TIME     : %s\n", null(info->time));

    SDDEBUG(" ALBUM    : %s\n", null(info->album));
    SDDEBUG(" TITLE    : %s\n", null(info->title));
    SDDEBUG(" ARTIST   : %s\n", null(info->artist));
    SDDEBUG(" YEAR     : %s\n", null(info->year));
    SDDEBUG(" GENRE    : %s\n", null(info->genre));
    SDDEBUG(" TRACK    : %s\n", null(info->track));
    SDDEBUG(" COMMENTS : %s\n", null(info->comments));
  } else {
    SDDEBUG("Invalid info\n");
  }

  unlockinfo();
}
