/**
 * @file    playa_info.c
 * @author  benjamin gerard <ben@sashipa.com>
 * @date    2002/09/23
 * @brief   Music informations
 *
 * $Id: playa_info.c,v 1.3 2003-01-25 17:26:03 ben Exp $
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arch/spinlock.h>

#include "sysdebug.h"
#include "playa_info.h"

static spinlock_t infomutex;
static int info_id;
static playa_info_t curinfo;

static int ToUpper(int c)
{
  c &= 255;
  if (c>='a' && c<='z') c ^= 32;
  return c;
}

static int ToLower(int c)
{
  c &= 255;
  if (c>='A' && c<='Z') c ^= 32;
  return c;
}

static int next_info_id(void)
{
  /* $$$ LUA rules : 24 bit for floating point precision !!! */
  if (++info_id >= (1<<24)) {
    info_id = 1;
  }
  return info_id;
}

static void lockinfo(void)
{
  spinlock_lock(&infomutex);
}

void unlockinfo(void)
{
  spinlock_unlock(&infomutex);
}

playa_info_t * playa_info_lock(void)
{
  lockinfo();
  return &curinfo;
}

void playa_info_release(playa_info_t *i) {
  if (i == &curinfo) {
    unlockinfo();
  }
}

void info_clean(playa_info_t *i)
{
  if (i) {
    memset(i,0,sizeof(*i));
  }
}

void playa_info_clean(void)
{
  lockinfo();
  playa_info_free(&curinfo);
  // $$$ ben: We only see help here !
  //  curinfo.info[PLAYA_INFO_FORMAT].s = strdup("No music is playing");
  // $$$ben validate allow vmu to know it must to update its scroller.
  curinfo.valid = next_info_id();
  unlockinfo();
}

void playa_info_free(playa_info_t *info)
{
  int i;
  for (i=PLAYA_INFO_DESC; i<=PLAYA_INFO_COMMENTS; ++i) {
    if (info->info[i].s) {
      free(info->info[i].s);
    }
  }
  info_clean(info);
}

static void make_timestr(playa_info_t *info)
{
  char time[32];
  playa_info_make_timestr(time, info->info[PLAYA_INFO_TIME].v);
  info->info[PLAYA_INFO_TIMESTR].s = strdup(time);
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

#define PLAYA_INFO_FORMAT_MASK \
 (1<<PLAYA_INFO_BPS) |\
 (1<<PLAYA_INFO_STEREO) |\
 (1<<PLAYA_INFO_DESC) |\
 (1<<PLAYA_INFO_FRQ)

static void make_format(playa_info_t * info)
{
  char tmp[512];
  const int max = sizeof(tmp)-1;
  tmp[0] = 0;
  tmp[max] = 0;

  if (info->info[PLAYA_INFO_BPS].v) {
    strncat(tmp, frq_1000(info->info[PLAYA_INFO_BPS].v, "kbps "), max);
  }
  strncat(tmp, info->info[PLAYA_INFO_STEREO].v ? "stereo":"mono", max);
  if (info->info[PLAYA_INFO_DESC].s) {
    strncat(tmp, " ", max);
    strncat(tmp, info->info[PLAYA_INFO_DESC].s, max);
  }
  if (info->info[PLAYA_INFO_FRQ].v) {
    strncat(tmp, " ", max);
    strncat(tmp, frq_1000(info->info[PLAYA_INFO_FRQ].v, "khz"), max);
  }

  info->info[PLAYA_INFO_FORMAT].s = strdup(tmp);
}

static void make_vmu(playa_info_t * info)
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
  for (d=tmp+strlen(tmp), i=0, p=&info->info[PLAYA_INFO_FORMAT].s ; names[i];
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
  info->info[PLAYA_INFO_VMU].s = strdup(tmp);
}


static int bits(int a, int b)
{
  return
    ((1<<(b+1))-1) &
    ~((1<<(a))-1);
}

int playa_info_update(playa_info_t *info)
{
  unsigned int format_mask = PLAYA_INFO_FORMAT_MASK;
  unsigned int vmu_mask = 
    bits(PLAYA_INFO_FORMAT, PLAYA_INFO_COMMENTS);
  unsigned int u;
  int i;

  if (!info) {
    return -1;
  }

  u = info->update_mask;
  //  SDDEBUG("update mask = [%X]\n", u);

  /* Format string depends on many things */
  u |= (!!(info->update_mask & format_mask)) << PLAYA_INFO_FORMAT;

  /* VMU string too */
  u |= (!!(info->update_mask & vmu_mask)) << PLAYA_INFO_VMU;

  //  SDDEBUG("update mask after = [%X]\n", u);

  if (!u) {
    /* Nothing to update */
    return 0;
  }

  /* Update time string */
  if ((u & (1<<PLAYA_INFO_TIME))) {
    make_timestr(info);
    u |= (1<<PLAYA_INFO_TIMESTR);
  }

  lockinfo();
  curinfo.valid = 0;

  /* Update numeric value */
  for (i = PLAYA_INFO_BITS; i <= PLAYA_INFO_BYTES; ++i) {
    if (u & (1<<i)) {
      curinfo.info[i].v = info->info[i].v;
    }
  }

  /* Update string value */
  for (i = PLAYA_INFO_TIMESTR; i <= PLAYA_INFO_COMMENTS; ++i) {
    if (u & (1<<i)) {
      if (curinfo.info[i].s) {
	free(curinfo.info[i].s);
      }
      curinfo.info[i].s = info->info[i].s;
    }
  }

  /* Update description string */
  i = PLAYA_INFO_DESC;
  if (u & (1<<i)) {
    if (curinfo.info[i].s) {
      free(curinfo.info[i].s);
    }
    curinfo.info[i].s = info->info[i].s;
  }

  /* Update format string */
  i = PLAYA_INFO_FORMAT;
  if (u & (1<<i)) {
    if (curinfo.info[i].s) {
      free(curinfo.info[i].s);
    }
    make_format(&curinfo);
  }

  /* Update vmu string */
  i = PLAYA_INFO_VMU;
  if (u & (1<<i)) {
    if (curinfo.info[i].s) {
      free(curinfo.info[i].s);
    }
    make_vmu(&curinfo);
  }

  curinfo.update_mask = u;
  curinfo.valid = next_info_id();
  *info = curinfo; 
  unlockinfo();

  return 0;
}

int playa_info_init(void)
{
  info_id = 0;
  info_clean(&curinfo);
  spinlock_init(&infomutex);
  return 0;
}

void playa_info_shutdown(void)
{
  lockinfo();
  playa_info_free(&curinfo);
  unlockinfo();
}

static char * null(char *a) {
  return a ? a : "<null>";
}

void playa_info_dump(playa_info_t * info)
{
  SDDEBUG(" ID       : %d\n", info->valid);
  SDDEBUG(" BITS     : %d\n", playa_info_bits(info,-1));
  SDDEBUG(" STEREO   : %d\n", playa_info_stereo(info,-1));
  SDDEBUG(" SAMPLING : %d\n", playa_info_frq(info,-1));
  SDDEBUG(" TIME     : %d\n", playa_info_time(info,-1)>>10);
  SDDEBUG(" BPS      : %d\n", playa_info_bps(info,-1));
  SDDEBUG(" BYTES    : %d\n", playa_info_bytes(info,-1));
  
  SDDEBUG(" FORMAT   : %s\n", playa_info_format(info));
  SDDEBUG(" TIME     : %s\n", playa_info_timestr(info));
  SDDEBUG(" DESC     : %s\n", playa_info_desc(info,(char*)-1));
  
  SDDEBUG(" ALBUM    : %s\n", playa_info_album(info,(char*)-1));
  SDDEBUG(" TITLE    : %s\n", playa_info_title(info,(char*)-1));
  SDDEBUG(" ARTIST   : %s\n", playa_info_artist(info,(char*)-1));
  SDDEBUG(" YEAR     : %s\n", playa_info_year(info,(char*)-1));
  SDDEBUG(" GENRE    : %s\n", playa_info_genre(info,(char*)-1));
  SDDEBUG(" TRACK    : %s\n", playa_info_track(info,(char*)-1));
  SDDEBUG(" COMMENTS : %s\n", playa_info_comments(info,(char*)-1));
}

static int value(playa_info_t * info, int idx, int v,
		 const int min, const int max)
{
  const int sign = (sizeof(int)<<3) - 1;
  int s;
  if (v == -1) {
    return info->info[idx].v;
  }
  info->update_mask |= 1 << idx;
  s = (v-min) >> sign;
  v = (v & ~s) | (min & s);
  s = (max-v) >> sign;
  v = (v & ~s) | (max & s);

  //  SDDEBUG("value(%d <- %d)\n", idx, v);

  return info->info[idx].v = v;
}

static char * string(playa_info_t * info, int idx, char * s)
{
  if (s == (char *)-1) {
    return info->info[idx].s;
  }
  info->update_mask |= 1 << idx;
  if (s) {
    s = strdup(s);
  }

  //  SDDEBUG("string(%d <- %p)\n", idx, s);

  return info->info[idx].s = s;
}

int playa_info_bits(playa_info_t * info, int v)
{
  return value(info, PLAYA_INFO_BITS, v, 0, 1);
}

int playa_info_stereo(playa_info_t * info, int v)
{
  return value(info, PLAYA_INFO_STEREO, v, 0, 1);
}

int playa_info_frq(playa_info_t * info, int v)
{
  return value(info, PLAYA_INFO_FRQ, v, 8000, 50000);
}

int playa_info_time(playa_info_t * info, int v)
{
  return value(info, PLAYA_INFO_TIME, v, 0, 0x7fffffff);
}

int playa_info_bps(playa_info_t * info, int v)
{
  return value(info, PLAYA_INFO_BPS, v, 0, 50000*32);
}

int playa_info_bytes(playa_info_t * info, int v)
{
  return value(info, PLAYA_INFO_BYTES, v, 0, 0x7fffffff);
}

char * playa_info_desc(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_DESC, v);
}

char * playa_info_artist(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_ARTIST, v);
}

char * playa_info_album(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_ALBUM, v);
}

char * playa_info_track(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_TRACK, v);
}

char * playa_info_title(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_TITLE, v);
}

char * playa_info_year(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_YEAR, v);
}

char * playa_info_genre(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_GENRE, v);
}

char * playa_info_comments(playa_info_t * info, char * v)
{
  return string(info, PLAYA_INFO_COMMENTS, v);
}

char * playa_info_format(playa_info_t * info)
{
  return string(info, PLAYA_INFO_FORMAT, (char *)-1);
}

char * playa_info_timestr(playa_info_t * info)
{
  return string(info, PLAYA_INFO_TIMESTR, (char *)-1);
}

char * playa_info_make_timestr(char * time, unsigned int ms)
{
  static char timebuf[32];
  unsigned int h,m,s;

  if (!time) {
    time = timebuf;
  }
  if (!ms) {
    strcpy(time, "--:--");
  } else {
    h = ms / (3600<<10);
    ms -= h * (3600<<10);
    m = ms / (60<<10);
    ms -= m * (60<<10);
    s = ms >> 10;

    if (h) {
      sprintf(time, "%d:%02d:%02d",h,m,s);
    } else {
      sprintf(time, "%02d:%02d",m,s);
    }
  }
  return time;
}
