/* */

#include <stdio.h>
#include <string.h>
#include "gp.h"

#include "songmenu.h"
#include "entrylist.h"
#include "controler.h"
#include "option.h"

#include "m3u.h"
#include "driver_list.h"
#include "filetype.h"

#define FILE_BROWSER_LINES    13
#define LOAD_RECURSIVE_DIR    0x80000000

/* File Browser entry */
typedef struct {
  int  type;      /* File Type */ 
  int  size;      /* Size of entry in bytes (-1 for dir) */
  char name[60];  /* Name (displayed) */  
  char fn[60];    /* Filename (leaf only) */
} entry;

/* Play list entry */
typedef struct {
  int  type;      /* File Type */ 
  int  size;      /* Size of entry in bytes (-1 for dir) */
  char name[60];  /* Name (displayed) */
  char fn[256];   /* Filename (Full path)*/
  int  pos;       /* Position in directory */
} lst_entry;

typedef struct
{
  int top;
  int selected;
  int play;
  float x;
  float y;
  entrylist_t *list;
} entry_window_t;

static entry_window_t wins[2], *curwin, *dirwin, *playwin;
static entrylist_t direntry;
char songmenu_selected[64];

/* Decoder driver */
//static const inp_driver_list_t * drivers;

/* From dreamcast68.c */
extern int dreamcast68_loaddisk(const char *fn, int imm);
extern int dreamcast68_isplaying(void);
extern controler_state_t controler68;
extern float fade68;

static char playdir[512];
static char curdir[512];
static char loaddir[512];
static entry find_entry;

static int framecnt = 0;
static float throb = 0.2f, dthrob = 0.01f;
static float global_alpha_goal = 1.0f, global_alpha = 0.0f;


/* Here we hold the playlist */
static entrylist_t playlist;
static int playlist_start_idx;

static volatile int running = 0;
static volatile kthread_t * loader_thd = 0; 

/* M3U */
static int mycookie;

static void * mymalloc(void * cookie, int bytes)
{
  return malloc(bytes);
}

static void myfree(void * cookie, void * data)
{
  if (data) free(data);
}

static int myread(void * cookie, char *data, int max)
{
  return fs_read(*(int*)cookie, data, max);
}

static M3Udriver_t driver = {
  (void*)&mycookie,
  mymalloc,
  myfree,
  myread
};

/* Entry sorting */

static int mytoupper(int c)
{
  c &= 255;
  if (c >= 'a' && c <= 'z') {
    c += 'A'-'a';
  }
  return c;
}

static int myisdigit(int c) {
  return (c <= '9') && (c >= '0');
}

static const char * mydecimal(const char *s, int *v)
{
  int c, r = 0;
  
  while (myisdigit(c=*s)) {
    r = (r * 10) + (c - '0');
    ++s;
  }
  *v = r;
  return s;
}  

/* Sort with '.' before */
static int mystricmp(const char *a, const char *b)
{
  int av, bv, digit;
  
  av = mytoupper(*a++);
  bv = mytoupper(*b++);
  digit = myisdigit(av) && myisdigit(bv);
  
  while (av && av==bv && !digit) {
    av = mytoupper(*a++);
    bv = mytoupper(*b++);
    digit = myisdigit(av) && myisdigit(bv);
  }
  
  if (digit) {
    /* Natural sorting */
    a = mydecimal(a-1, &av);
    b = mydecimal(b-1, &bv);
    if (av == bv) {
      return mystricmp(a, b);
    }
  } else {
    /* Hack for <root> , '.' and '..' */
    /* $$$ filetype should be enought for that job */ 
    if (av == '<') av = 30;
    if (bv == '<') bv = 30;
    if (av == '.') av = 31;
    if (bv == '.') bv = 31;
  }
  
  return av-bv;
}

static int cmp_entry(const entry *a, const entry *b)
{
  int res;
  
  res = a->type - b->type;
  if (res == 0) {
    res = mystricmp(a->name, b->name);
  }
  return res;
}

static int cmp_entry_fn(const entry *a, const entry *b)
{
  return mystricmp(a->fn, b->fn);
}

static int file_type_inp(const char *name)
{
  const char *e;
  int type = FILETYPE_UNKNOWN;

  e = filetype_ext(name);
  if (e[0]) {
    inp_driver_t *d = inp_driver_list_search_by_extension(e);

    if (d) {
      type = d->id;
    }
  }
  return type;  
}

/* */
static int filter_entry(const char *name)
{
  int type = file_type_inp(name);
 
  if (option_filter() && type == FILETYPE_UNKNOWN) {
    dbglog(DBG_DEBUG," ** " __FUNCTION__ "(%s) : FILTER OUT\n", name);
    return FILETYPE_ERROR;
  }
  return type;
}

static int direntry_filetype(const dirent_t *de)
{
  int type;

  type = filetype_get(de->name, de->size);
  if (type == FILETYPE_UNKNOWN) {
    type = filter_entry(de->name);
  }
  return type;
}

static void make_mp3_name(char *dst, const char * src, int max)
{
  
  if (option_filter()) {
    int i, ext;
    
    --max;
    for (i=0, ext=-1; i<max; ++i) {
      int c = src[i];
      if (!c) break;
      if (c == '.') ext = i;
      else if (c == '_') c = ' ';
      dst[i] = c;
    }
    dst[i] = 0;
    if (ext>0 && i-ext <= 5) dst[ext] = 0;
  } else {
    strncpy(dst, src, max);
    dst[max-1] = 0;
  }
}
  
static void locate_file(int selected, const char *verify)  
{
  int move;
  
//  entrylist_sync(&direntry, 1);
  
  if ((unsigned int)selected >= (unsigned int) direntry.nb) {
    selected = 0;
  } else {
    curwin = wins;
  }
  
  if (verify && verify[0]) {
    entry *e;
    
    dbglog(DBG_DEBUG,"** " __FUNCTION__ " : verify '%s'\n", verify);
    e = entrylist_addrof(&direntry, selected);
    if (!e || strcmp(e->fn, verify)) {
      entry findme;
      
      dbglog(DBG_DEBUG,"** " __FUNCTION__ " : '%s' not at #%d\n",
	     verify, selected);
      strncpy(findme.fn, verify, sizeof(findme.fn));
      selected = entrylist_find(&direntry,
				&findme,
				(int (*)(const void *, const void *))
				cmp_entry_fn);
      if (selected<0) {
        dbglog(DBG_DEBUG,"** " __FUNCTION__ " : '%s' not found.\n", verify);
        selected = 0;
      } else {
        dbglog(DBG_DEBUG,"** " __FUNCTION__ " : '%s' found at #%d.\n",
	       verify, selected);
      }
    }
  }
  
  dirwin->selected = dirwin->top = selected;
  
  move = FILE_BROWSER_LINES - (direntry.nb - selected);
  if (move > 0 && (dirwin->top -= move) < 0) {
    dirwin->top = 0;
  }
  
//  entrylist_sync(&direntry, 0);
}


static lst_entry staticentry;

static int add_entry_to_playlist(lst_entry *le, int insert);

static int r_load_song_dir(char *dirname)
{
  int cnt = 0, l;
  file_t d;
  dirent_t *de;

  dbglog(DBG_DEBUG, ">> " __FUNCTION__ "(%s)\n", dirname);
	
  /* Abort if loader thread has been asked to exit */
  if (!running) {
    return 0;
  }
	
  if (entrylist_isfull(&playlist)) {
    dbglog(DBG_DEBUG, "<< " __FUNCTION__ "(%s) : list full\n", dirname);
    return 0;
  }

  l = strlen(dirname);

  d = fs_open(dirname, O_RDONLY | O_DIR);
  if (!d) {
    goto error;
  }
    
  while (running && (de = fs_readdir(d), de)) {
    int type;
    
    if (de->size < 0) {
      /* Skip directory in first pass */
/*        dbglog(DBG_DEBUG, "** " __FUNCTION__  */
/*  	     "(%s) : Skip dir [%s] in 1st pass\n", dirname, de->name); */
      continue;
    }

    type = direntry_filetype(de);
/*      dbglog(DBG_DEBUG, "*** " __FUNCTION__  */
/*  	   "(%s) : [s:%d] ['%s'] [t:%x]\n", */
/*  	   dirname, de->size, de->name, type); */

    if (type >= FILETYPE_PLAYABLE) {
      staticentry.type = type;
      strcpy(staticentry.fn, dirname);
      strcat(staticentry.fn, "/");
      strcat(staticentry.fn, de->name);
      staticentry.size = de->size;
      staticentry.pos = 0; /* Can't guess without sorting */
      make_mp3_name(staticentry.name,de->name,sizeof(staticentry.name));
      if (add_entry_to_playlist(&staticentry,1) >= 0) {
	++cnt;
      }
    }
    entrylist_sort_part(&playlist, playlist.nb-cnt ,cnt,
			(int (*)(const void *, const void *))cmp_entry);
  }
  fs_close(d);

  d = fs_open(dirname, O_RDONLY | O_DIR);
  if (!d) {
    goto error;
  }
    
  while (running && (de = fs_readdir(d), de)) {
    //    int type;
    
    if (de->size >= 0) {
      /* Skip file in 2nd pass */
/*        dbglog(DBG_DEBUG, "** " __FUNCTION__  */
/*  	     "(%s) : Skip file [%s] in 2nd pass\n", dirname, de->name); */
      continue;
    }

    dirname[l] = '/';
    strcpy(dirname + l + 1, de->name);
    cnt += r_load_song_dir(dirname);
  }
  
 error:
  if (d) {
    fs_close(d);
  }
  dbglog(DBG_DEBUG, ">> " __FUNCTION__ " : [%d]\n", cnt);
  
  return cnt;
}

static void load_song_dir()
{
  int cnt;
  
  dbglog(DBG_DEBUG,">> " __FUNCTION__ "\n");
  entrylist_sync(&direntry, 1);

  if (!loaddir[0]) {
    strcpy(loaddir, "/");
  }
	
  cnt = r_load_song_dir(loaddir);
  entrylist_sync(&direntry, 0);
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ " [%d]\n", cnt);
}

static void load_song_list(int selected)
{
  file_t d;
  int pc;
  entry e;
	
  dirent_t *de;
  int clear_cache;
	
  dbglog(DBG_DEBUG,">> " __FUNCTION__ "\n");

  /* Abort if loader thread has been asked to exit */
  if (!running) {
    return;
  }
	
  entrylist_sync(&direntry, 1);
  entrylist_clean(&direntry);

  if (!loaddir[0]) {
    strcpy(loaddir, "/");
  }
	
  clear_cache = 0;
  if (!dreamcast68_isplaying()) {
    clear_cache   = !mystricmp(loaddir, "/cd");
    clear_cache  |= find_entry.fn[0]=='.' &&
      !find_entry.fn[1] &&
      strstr(loaddir,"/cd") == loaddir;
  } 
  for (d=0; !d && clear_cache < 3; ) {
    if (clear_cache) {
      dbglog( DBG_DEBUG, "!! " __FUNCTION__ " : Clear iso9660 cache [%d]\n",
	      clear_cache);
      iso_ioctl(0,0,0);  /* clear CD cache ! */
    }
	
    d = fs_open(loaddir, O_RDONLY | O_DIR);
    if (!d) {
      if (clear_cache) {
	dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Can't open %s. Try with /\n",
	       loaddir);
	strcpy(loaddir, "/");
      }
      ++clear_cache;
    }
  }

  if (!d) {
    strcpy(curdir,loaddir);
    entrylist_sync(&direntry, 0);
    find_entry.fn[0] = 0;
    dbglog(DBG_DEBUG,"<< " __FUNCTION__ " : FAILED\n");
    return;
  }
	
  strcpy(curdir,loaddir);
  pc = curdir[1]=='p' && curdir[2]=='c';  

  /* If not root : add special node */
  if (! (curdir[0] == '/' && !curdir[1])) {
    e.size = -1;
    e.type = FILETYPE_ROOT;
    strcpy(e.fn, "root");
    strcpy(e.name, "<root>");
    entrylist_add(&direntry, &e, -1);
    e.type = FILETYPE_SELF;
    strcpy(e.fn, ".");
    strcpy(e.name, "<refresh>");
    entrylist_add(&direntry, &e, -1);
    e.type = FILETYPE_PARENT;
    strcpy(e.fn, "..");
    strcpy(e.name, "<back>");
    entrylist_add(&direntry, &e, -1);
  }
	
  while (de = fs_readdir(d), de && running) {
    int l, type;
	  
    //thd_sleep(100);
    thd_pass();

    type = direntry_filetype(de);
	  
    if (pc) {
      dbglog(DBG_DEBUG,"READ entry '%s' [%d]\n", de->name, de->size);
    }
		
    /* $$$ */
    l = strlen (de->name);
    if (l >= sizeof(e.fn)) {
      continue;
    }

    e.type = type;
    e.size = de->size;
    /* Skip empty files and special dir entries (current[.] and parent [..]) */
    if (type == FILETYPE_DIR || type >= FILETYPE_FILE) {
      char tmpstr[256];
      strcpy(e.fn, de->name);
      e.size = de->size;
      strcpy(tmpstr,loaddir);
      strcat(tmpstr,"/");
      strcat(tmpstr,e.fn);
      make_mp3_name(e.name, e.fn, sizeof(e.name));
      entrylist_add(&direntry, &e, -1);
    }
  }

  entrylist_sort(&direntry, (int (*)(const void *, const void *))cmp_entry);
  locate_file(selected, find_entry.fn);

  entrylist_sync(&direntry, 0);
	
  fs_close(d);
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ "\n");
}

static volatile int load_list = 0;

static void loader_thread(void *dummy)
{
  dbglog(DBG_DEBUG,">> " __FUNCTION__ "\n");
  
  while (running) {
    if (load_list == LOAD_RECURSIVE_DIR) {
      load_song_dir();
      load_list = 0;
    } else if (load_list) {
      int new_selected = ~load_list;
      load_list = 0;
      dbglog(DBG_DEBUG,"** " __FUNCTION__
	     " : Asked to load a new list (cursor at %d)\n", new_selected);
      load_song_list(new_selected);
    } else {
      /* wait in ms */
      thd_sleep(200);
    }
  }
  
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Asked to exit\n");
  loader_thd = 0;
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ "\n");
}


/* Draws the song listing */
static int draw_any_listing(entry_window_t * win)
{
  float ystep=0;
  const int nlines = FILE_BROWSER_LINES;
  
  static struct {
    float a,r,g,b;
  } colors[8] = 
      //    { {1.00f,0.40f,0.75f,0.40f},    /* NORMAL */
      //      {1.00f,0.60f,0.90f,0.60f},    /* SELECTED */
      //      {1.00f,0.90f,0.70f,0.70f},    /* PLAYING */
      //      {1.00f,1.00f,0.80f,0.80f},    /* PLAYING+SELECTED */
      //    };
      //    { {1.00f,1.00f,1.00f,0.40f},    /* NORMAL */
      //      {1.00f,1.00f,1.00f,0.80f},    /* SELECTED */
      //      {1.00f,0.90f,0.70f,0.70f},    /* PLAYING */
      //      {1.00f,1.00f,0.80f,0.80f},    /* PLAYING+SELECTED */
      //    };
    { {0.50f,1.00f,1.00f,1.00f},    /* NORMAL */
      {0.70f,1.00f,1.00f,1.00f},    /* SELECTED */
      {0.50f,1.00f,1.00f,0.00f},    /* PLAYING */
      {0.70f,1.00f,1.00f,0.00f},    /* PLAYING+SELECTED */
      {0.50f,0.50f,0.50f,0.50f},    /* UNKNOW-NORMAL */
      {0.70f,0.50f,0.50f,0.50f},    /* UNKNOW-SELECTED */
      {0.50f,0.50f,0.80f,0.80f},    /* PLAYLIST-NORMAL */
      {0.70f,0.50f,0.80f,0.80f},    /* PLAYLIST-SELECTED */
    };
  int color_idx;    
  
  const float xs = win->x;
  const float ys = win->y;
  const float halftint = global_alpha * fade68 *
    ((win == curwin) ? 1.0f : 0.5f);
  float y = win->y;
  int i, esel;
  int top, selected, num_entries;

  //  dbglog(DBG_DEBUG," [%.3f %.3f]", global_alpha, halftint);
	
  if (halftint < 1E-3) {
    return 0;
  }

  /* List is busy ... skip refresh */
  if (!win->list->sync) {
    draw_poly_text(xs, ys, 100.0f, fade68, 1.0f, 1.0f, 1.0f,
		   "> Please wait . . . busy . . . <");
    return 1;
  }
	
  /* Lock list for this op. */
  entrylist_sync(win->list, 1);

  num_entries = win->list->nb;
	
  if (num_entries <= 0) {
    draw_poly_text(xs, y, 100.0f,
		   colors[0].a * halftint,
		   colors[0].r, colors[0].g,
		   colors[0].b,
		   "<empty>");
  } else {
    top      = win->top;
    selected = win->selected;
    if (selected < 0) {
      selected = 0;
    } else if (selected >= num_entries) {
      selected = num_entries - 1;
    }
    esel      = (selected - top);
    if (esel < 0) {
      top  = selected; 
      esel = 0;
    } else if (esel >= nlines) {
      int move = esel - nlines + 1;
      top += move;
      esel = selected - top;
    }
    win->top      = top;
    win->selected = selected;
  	  	
    for (i=0; i<nlines && (top+i)<num_entries; i++, y += ystep) {
      static char tmp[256];
      entry * e = (entry *)entrylist_addrof(win->list, i+top);
      char * s;

      color_idx = ( (top+i == win->play) &&
		    (win != dirwin || !strcmp(curdir,playdir))
		    ) << 1;	  
      color_idx |= (i==esel);

      s = e->name;
      if (e->type == FILETYPE_DIR) {
	char *d = tmp;
	*d++ = '['; 
	while (*s) {
	  *d++ = *s++;
	}
	*d++ = ']'; 
	*d = 0;
	s = tmp;
      } else if (e->type >= FILETYPE_PLAYABLE) {
	inp_driver_t *d = inp_driver_list_search_by_id(e->type);
	if (d) {
	  tmp[0] = 2;
	  tmp[1] = 0;
	  //strcpy(tmp, d->extensions);
	  strcat(tmp, " ");
	  strcat(tmp, s);
	  s = tmp;
	  //dbglog(DBG_DEBUG, "!! driver found for %s\n", s);
	} else {
	  dbglog(DBG_DEBUG, "!! driver not found for %s\n", s);
	}
      } else if (e->type == FILETYPE_UNKNOWN) {
	color_idx = (color_idx & 1) + 4;
      } else if (e->type == FILETYPE_PLAYLIST) {
	color_idx = (color_idx & 1) + 6;
      }

      if (color_idx >= sizeof(colors) / sizeof(*colors)) {
	color_idx = 0;
	dbglog(DBG_ERROR,"!! " __FUNCTION__ " : bad color idx\n");
      }
  	
      ystep = draw_poly_text(xs, y, 100.0f,
			     colors[color_idx].a * halftint,
			     colors[color_idx].r, colors[color_idx].g,
			     colors[color_idx].b,
			     s);
    }
  	
    /* Put a highlight bar under one of them */
    draw_poly_box(xs, ys + esel * ystep - 1.0f,
		  xs+256.0f, ys + (esel+1) * ystep, 95.0f,
		  throb*halftint, throb*0.5f, throb*.5f, 0,
		  throb*halftint, throb, throb, 0);
  }

  entrylist_sync(win->list, 0);
  return 0;
}

static int draw_listings(void)
{
  int res;
  
  res  = draw_any_listing(wins + 0);
  draw_any_listing(wins + 1);
  
  return res;
}


static void check_playlist(void)
{
  int imm;
  lst_entry *le;

  /* Exit if playlist is not active or music is playing */
  if (playlist_start_idx < 0 || dreamcast68_isplaying()) {
    return;
  }
  
  /* Start playlist */
  if (imm = playwin->play < 0, imm) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__
	   " : Start playlist at #%d\n", playlist_start_idx);
    playwin->play = playlist_start_idx - 1;
    if (option_shuffle()) {
      entrylist_shuffle(&playlist, playlist_start_idx);
    }
  }
  
  /* Find next valid idx */
  while (++playwin->play < playlist.nb) {
    le = entrylist_addrof(&playlist, playwin->play);
    dbglog(DBG_DEBUG, "** " __FUNCTION__
	   " : Playlist load #%d '%s'\n", playwin->play, le->fn);
    if (!dreamcast68_loaddisk(le->fn, imm)) {
      return;
    }
    dbglog(DBG_DEBUG, "** " __FUNCTION__
	   " : Failed loading #%d '%s'\n", playwin->play, le->fn);
  }
  dbglog(DBG_DEBUG, "** " __FUNCTION__ " : End of list reached\n");
  
  /* End of list */
  playlist_start_idx = playwin->play = -1;
}

static int add_entry_to_playlist(lst_entry * le, int insert)
{
  int idx;

  idx = insert ? playwin->selected + 1 : -1;
/*    dbglog(DBG_DEBUG, "** " __FUNCTION__ " : #%d %s ['%s' %d]\n", idx, */
/*  	 insert ? "INSERT" : "ENQUEUE", le->fn, le->size); */
  idx = entrylist_add(&playlist, le, idx);
  if (idx >= 0) {
/*      dbglog(DBG_DEBUG, "** " __FUNCTION__ " : ADDED #%03d ['%s' %d]\n", idx, */
/*  	   le->fn, le->size); */
    
    if (playwin->play >= 0 && idx <= playwin->play) {
      /* Insert before current playlist, shift */
      ++playwin->play;
    }			 
    if (insert) {
      ++playwin->selected;
    }
  } else {
    dbglog(DBG_DEBUG, "!! " __FUNCTION__ " : FAILED #%03d ['%s' %d]\n",
	   playwin->selected, le->fn, le->size);
  }
  return idx;
}

static int add_to_playlist(int selected, int insert)
{
  const entry * e = (const entry *)entrylist_addrof(&direntry, selected);
  int len1, len2;
  lst_entry le;
  
  /* Directory : recursive load */
  if (e->type == FILETYPE_DIR) {
    strcpy(loaddir, curdir);
    if (strcmp(loaddir,"/")) {
      strcat(loaddir,"/");
    }
    strcat(loaddir,e->fn);
    load_list = LOAD_RECURSIVE_DIR;
    return 0;
  }

  /* Playable file : Enqueue/Insert file */
  if (e->type >= FILETYPE_PLAYABLE) {
    len1 = strlen(curdir);
    len2 = strlen(e->fn);
    if (len1 + len2 + 1 < sizeof(le.fn)) {
      strcpy(le.fn, curdir);
      strcat(le.fn, "/");
      strcat(le.fn, e->fn);
      strcpy(le.name, e->name);
      le.size = e->size;
      le.pos = selected;
      return add_entry_to_playlist(&le, insert);
    } else {
      dbglog(DBG_DEBUG,"** " __FUNCTION__
	     " : Skipped (filename too long) '%s/%s'\n",curdir,e->fn);
      return -1;
    }
  }

  /* Playlist file : $$$ TODO*/
  if (e->type >= FILETYPE_PLAYLIST) {
    dbglog(DBG_DEBUG, "** " __FUNCTION__
	   " : %s playlist [%s]\n", insert?"Insert":"Enqueue", e->fn); 
    return -1;
  }

  dbglog(DBG_DEBUG, "** " __FUNCTION__
	 " : Nothing to do with %s\n", e->fn); 
  return -1;

}

static int del_from_playlist(int idx)
{
  int err = entrylist_del(&playlist, idx);
  
  if (!err) {
    if (idx == playwin->play) {
	    dbglog(DBG_DEBUG,"** " __FUNCTION__ " : delete current play : stop play list\n");
      playlist_start_idx = playwin->play = -1;
      //dreamcast68_loaddisk(0,0);
    }
    if (idx >= playlist.nb) {
      idx = playlist.nb - 1;
    }
    if (playwin->play > idx) {
      --playwin->play;
    }
    if (playlist_start_idx > idx) {
      --playlist_start_idx;
    } 
    playwin->selected = idx;
	  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Item #%d deleted\n", idx);
	  return 0;
  } else {
	  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Item not in playlist\n");
	  return -1;
  }
}

const int timemax = 600;
static int timeout = 0;

/* Handle controller input */
static void check_controller(entry_window_t * win)
{
  static int up_moved = 0, down_moved = 0;
  char tmpstr[256];

  int loadlist = 0;

  int num_entries;
  int top = win->top;
  int bottom = top + FILE_BROWSER_LINES - 1;
  int selected = win->selected;
/*    int sel_save = selected; */


  /* Any move with pad : enable display  */
  if (controler_pressed(&controler68, -1)) {
    timeout = timemax;
    global_alpha_goal = 1.0f;
  }

  /* START and Y buttons are reserved for other interface actions */
  if (controler68.buttons & (CONT_START | CONT_Y)) {
    return;
  }

  entrylist_sync(win->list, 1);

  num_entries = win->list->nb;
  if (bottom >= num_entries) {
    bottom = num_entries - 1;
  }


  if (controler_pressed(&controler68, CONT_DPAD_LEFT)
      || controler68.joyx < -64) {
    curwin = wins + 0;
  } else if (controler_pressed(&controler68, CONT_DPAD_RIGHT)
	     || controler68.joyx > 64) {
    curwin = wins + 1;
  }

  {
    const int padwait = 8, joywait_dw = 28, joywait_up = 28;

    int wait = 0;

    if (controler68.buttons & CONT_DPAD_UP) {
      wait = padwait;
    } else if (controler68.joyy < 0) {
      wait = ((joywait_up * (128 + controler68.joyy)) >> 7) + 1;
    }
    if (wait > 0)
      timeout = timemax;

    if (wait > 0 && (framecnt - up_moved) > wait) {
      if (selected > 0) {
	selected--;
	if (selected < top) {
	  top = selected;
	}
      }
      up_moved = framecnt;
    }

    wait = 0;
    if (controler68.buttons & CONT_DPAD_DOWN) {
      wait = padwait;
    } else if (controler68.joyy > 0) {
      wait = ((joywait_dw * (128 - controler68.joyy)) >> 7) + 1;
    }
    if (wait > 0)
      timeout = timemax;

    if (wait > 0 && (framecnt - down_moved) > wait) {
      if (selected < (num_entries - 1)) {
	selected++;
	if (selected >= (top + FILE_BROWSER_LINES)) {
	  top++;
	}
      }
      down_moved = framecnt;
    }
  }

  if (controler68.ltrig > 127) {
    if ((framecnt - up_moved) > 10) {
      if (selected != top) {
	selected = top;
      } else {
	selected -= FILE_BROWSER_LINES;
	if (selected < 0)
	  selected = 0;
	if (selected < top)
	  top = selected;
      }
      up_moved = framecnt;
      timeout = timemax;
    }
  }
  if (controler68.rtrig > 127) {
    if ((framecnt - down_moved) > 10) {
      if (selected != bottom) {
	selected = bottom;
      } else {
	selected += FILE_BROWSER_LINES;
	if (selected >= num_entries)
	  selected = num_entries - 1;
	top = selected - (FILE_BROWSER_LINES - 1);
	if (top < 0)
	  top = 0;
      }
      down_moved = framecnt;
      timeout = timemax;
    }
  }

  /* Pressed B */
  if (controler_released(&controler68, CONT_B) && selected >= 0) {
    if (win->list == &direntry) {
      /* Stop the playback */
      dreamcast68_loaddisk(0, 1);
      if (playwin->play >= 0) {
	playwin->selected = playwin->play;
      }
      playlist_start_idx = dirwin->play = playwin->play = -1;
    } else {
      del_from_playlist(selected);
      selected = playwin->selected;
    }
  }

  /* Pressed X */
  if (controler_released(&controler68, CONT_X) && selected >= 0) {
    lst_entry *le =
      (lst_entry *) entrylist_addrof(&playlist, selected);
    if (win->list == &direntry) {
      /* Insert to playlist */
      add_to_playlist(selected, 1);
    } else {
      /* Pressed X in playlist-window : Locate file */
      if ((unsigned int) selected < (unsigned int) playlist.nb) {
	char *s = strrchr(le->fn, '/');
	if (!s) {
	  tmpstr[0] = '/';
	  tmpstr[1] = 0;
	} else {
	  int l = s - le->fn;
	  memcpy(tmpstr, le->fn, l);
	  tmpstr[l] = 0;
	}
	if (!strcmp(curdir, tmpstr)) {
	  entrylist_sync(&direntry, 1);
	  locate_file(le->pos, s + 1);
	  entrylist_sync(&direntry, 0);
	} else {
	  /* $$$ MT error, find_entry could be accessed by loader-thread !! */
	  find_entry.fn[0] = 0;
	  strcpy(loaddir, tmpstr);
	  if (s) {
	    strcpy(find_entry.fn, s + 1);
	  }
	  loadlist = ~le->pos;
	}
      }
    }
  }

  if (controler_pressed(&controler68, CONT_A) && selected >= 0) {
    if (win->list == &direntry) {

      /* Pressed A in DIR */

      entry *e = (entry *) entrylist_addrof(&direntry, selected);
      find_entry.fn[0] = 0;

      if (e->type == FILETYPE_ERROR) {
	/* Error */
	strcpy(loaddir, "/");
	loadlist = ~0;
      } else if (e->type >= FILETYPE_PLAYABLE) {
	/* Playable */
	if (playwin->play >= 0) {
	  /* Alreay playing : enqueue */
	  add_to_playlist(selected, 0);
	} else {
	  /* Start playback */
	  if (selected >= 0) {
	    strcpy(tmpstr, curdir);
	    strcat(tmpstr, "/");
	    strcat(tmpstr, e->fn);
	    strcpy(playdir, curdir);
	    dirwin->play =
	      dreamcast68_loaddisk(tmpstr, 1) < 0 ? -1 : selected;
	  }
	}
      } else if (e->type >= FILETYPE_PLAYLIST) {
	/* Playlist : insert/enqueue */
	add_to_playlist(selected, playwin->play < 0);
      } else if (e->type <= FILETYPE_DIR) {
	/* Directory */

	dbglog(DBG_DEBUG, "** " __FUNCTION__ " : (A) Entering dir '%s'\n",
	       e->fn);

	if (e->type == FILETYPE_ROOT) {
	  loaddir[0] = '/';
	  loaddir[1] = 0;
	} else {
	  strcpy(loaddir, curdir);

	  if (e->type == FILETYPE_SELF) {
	    /* nop */
	    find_entry.fn[0] = '.';
	    find_entry.fn[1] = 0;

	  } else if (e->type == FILETYPE_PARENT) {
	    char *s;

	    s = strrchr(loaddir, '/');
	    if (!s) {
	      loaddir[0] = '/';
	      loaddir[1] = 0;
	    } else {
	      *s = 0;
	      strcpy(find_entry.fn, s + 1);
	    }
	  } else {
	    if (strcmp(loaddir, "/")) {
	      strcat(loaddir, "/");
	    }
	    strcat(loaddir, e->fn);
	    selected = 0;
	  }
	}
	dbglog(DBG_DEBUG, "** " __FUNCTION__
	       "  : cd '%s'\n", loaddir);
	loadlist = ~0; //~(selected <= 3 ? selected : 3);
      } else {
	/* Other (should be FILETYPE_UNKNOWN) */
	dbglog(DBG_DEBUG, "** " __FUNCTION__
	       " : (A) nothing to do with [%s]\n", e->fn);
      }
    } else {
      /* A pressed in playlist : start/restart at index */
      dreamcast68_loaddisk(0, 1);
      dirwin->play = -1;
      playwin->play = -1;
      playlist_start_idx = selected;
    }
  }

  win->top = top;
  win->selected = selected;

  {
    /*      const int timemax = 10 * 60; */
    entry *e;

    /* time out has been setted to max : alpha goes to 1 */ 
    if (timeout == timemax) {
      global_alpha_goal = 1.0f;
    }

    if (timeout < 0) {
      timeout = 0;
      global_alpha_goal = 0.0f;
      songmenu_selected[0] = 0;
    } else {
      memset(songmenu_selected, 0, sizeof(songmenu_selected));
      if (e = entrylist_addrof(win->list, selected), e) {
	strcpy(songmenu_selected, e->name);
      } else {
	strcpy(songmenu_selected, "<empty>");
      }
    }  
  }

  if (loadlist) {
    dbglog(DBG_DEBUG,
	   "** " __FUNCTION__ " : Post load list, find '%s' at #%d\n",
	   find_entry.fn, ~loadlist);
    load_list = loadlist;
  }
  entrylist_sync(win->list, 0);
}

/* Check maple bus inputs */
void check_inputs()
{
  check_controller(curwin);
}

/* Main rendering of the song menu */
void songmenu_render(int elapsed_frames)
{
  int locked;

  /* Timeout for fade in / out */
  timeout -= elapsed_frames;

  /* Adjust the throbber */
  throb += dthrob;
  if (throb < 0.3f || throb > 0.7f) {
    dthrob = -dthrob;
    throb += dthrob;
  }

  /* Draw the song listing */
  locked = draw_listings();
  
  /* Check maple inputs */
  if (!locked) {
    check_inputs();
    check_playlist();
  }
  
  /* Smooth global alpha */
  {
    const float f = global_alpha_goal < 0.5f ? 0.05f : 0.25f;
    global_alpha = global_alpha_goal * f + global_alpha * (1.0 - f); 
  }

  framecnt += elapsed_frames;
}



int songmenu_start(void)
{
  dbglog(DBG_DEBUG,">> " __FUNCTION__ "\n");
  
  running = 1;
  load_list = ~0;
  
  /* Entry loader thread */
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Starting loader thread\n");
  
  loader_thd = thd_create(loader_thread, 0);
  if (loader_thd) {
    dbglog(DBG_DEBUG,"** " __FUNCTION__
	   " : OK loader thread [%p]:\n", loader_thd);
    //    thd_set_prio(loader_thd, PRIO_DEFAULT / 2);
    thd_set_label((kthread_t *)loader_thd, "Loader-thd");
  } else {
    dbglog(DBG_DEBUG,"** " __FUNCTION__ " : FAILED entry loader thread\n");
  }
  
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ "\n");
  return 0;
}

int songmenu_init(void)
{
  int i;
  dbglog(DBG_DEBUG,">> " __FUNCTION__ "\n");
  
  framecnt = 0;
  running = 0;
  load_list = 0;

  /* Init menu text fade in/out */
  global_alpha_goal = 1.0f;
  global_alpha = 0.0f;
  timeout = timemax;
  
  /* Set M3U driver */
  M3Udriver(&driver);

#if 0
  /* Get driver list */
  {
    inp_driver_t *d;

    /* Set up drivers id */
    dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Setup driver ID\n");
    for (d=(inp_driver_t *)inp_drivers.drivers, i=FILETYPE_PLAYABLE;
	 d;
	 d=(inp_driver_t *)d->common.nxt) {
      d->id = i++;
    }
  }
#endif

  /* Browser entries */
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Init browser entries\n");
  if (entrylist_create(&direntry, 200, sizeof(entry)) < 0) {
    return -1;
  }
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : OK browser entries\n");

  /* Playlist entry */
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Init playlist entries\n");
  if (entrylist_create(&playlist, 1500, sizeof(lst_entry)) < 0) {
    return -1;
  }
  dbglog(DBG_DEBUG,"** " __FUNCTION__ " : OK playlist entries\n");
  
  /* Attach window */
  for (i=0; i<2; ++i) {
    wins[i].top = wins[i].selected = 0;
    wins[i].play = -1;
    if (!i) {
      wins[i].x = WIN68_DIR_X;
      wins[i].list = &direntry;
    } else {
      wins[i].x = WIN68_PLAYLIST_X;
      wins[i].list = &playlist;
    }
    wins[i].y = WIN68_Y;
  }
  curwin = dirwin = wins + 0;
  playwin = wins + 1; 
  
  loaddir[0] = playdir[0] = curdir[0] = '/';  
  loaddir[1] = playdir[1] = curdir[1] = 0;
  find_entry.fn[0] = 0;
  playlist_start_idx = -1;
  songmenu_selected[0] = 0;
  
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ "\n");
  return 0;
}

/* Shutdown loader-thread, kill browser and play lists  
 */
int songmenu_kill(void)
{
  dbglog(DBG_DEBUG,">> " __FUNCTION__ "\n");
  
  if (loader_thd) {
    int cnt = 50000;
    
    dbglog(DBG_DEBUG, "** " __FUNCTION__ " : ask loader thread to end.\n");
    running = 0;
    for (running=0; loader_thd && cnt; running=0, --cnt) {
      thd_pass();
    }
    dbglog(DBG_DEBUG,"** " __FUNCTION__ " : Count down : %d\n", cnt);
  }
  
  if (loader_thd) {
    dbglog(DBG_DEBUG,"** " __FUNCTION__
	   " : Explicit kill of loader thread.\n");
    thd_destroy((kthread_t *)loader_thd);
    loader_thd = 0;
  }
  
  running = 0;
  entrylist_kill(&direntry);
  entrylist_kill(&playlist);
  dbglog(DBG_DEBUG,"<< " __FUNCTION__ "\n");

  return 0;
}

/*  Get the current active window [0:browser 1:play-list] 
 */
int songmenu_current_window(void)
{
  return curwin == wins + 1;
}
