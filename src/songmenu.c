/**
 * @file    songmenu.c
 * @author  ben(jamin) gerard <ben@sashipa.com>
 * @date    2002/02/10
 * @brief   file and playlist browser
 * 
 * $Id: songmenu.c,v 1.9 2002-10-10 06:05:37 benjihan Exp $
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "gp.h"

#include "songmenu.h"
#include "entrylist.h"
#include "controler.h"
#include "option.h"

#include "m3u.h"
#include "driver_list.h"
#include "filetype.h"
#include "playa.h"

#include "sysdebug.h"

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
  return fs_read(*(int *)cookie, data, max);
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

static const char *nullstr(const char *s) {
  return s ? s : "<null>";
}

static int file_type_inp(const char *name)
{
  //  const char *e;
  int type = FILETYPE_UNKNOWN;

  /* $$$ ben : don't use this becoz it does not handle .gz correctly. */
  /*
  e = filetype_ext(name);
  if (e[0]) {
  */

  inp_driver_t *d = inp_driver_list_search_by_extension(name);

  if (d) {
    type = d->id;
  }
/*   } */
  return type;  
}

/* */
static int filter_entry(const char *name)
{
  int type = file_type_inp(name);
 
  if (option_filter() && type == FILETYPE_UNKNOWN) {
    SDDEBUG("%s(%s) : FILTER OUT\n", __FUNCTION__, name);
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

static int make_full_path_name(char *d, const char *path, const char *leaf,
			       int max)
{
  int len1, len2;
  
  len1 = strlen(path);
  len2 = strlen(leaf);
  if (len1 + len2 + 1 < max) {
    strcpy(d, path);
    if (len1 > 0 && d[len1-1] != '/') {
      strcat(d, "/");
    }
    strcat(d, leaf);
    return 0;
  }
  return -1;
}

static void make_mp3_name(char *dst, const char * src, int max)
{
  const char * s;

  /* Get leaf name */
  s = strrchr(src,'/');
  if (s) {
    src = s;
  }
  
  if (option_filter()) {
    int i, ext, ext2;
    int lim = 5;
    
    --max;
    for (i=0, ext=ext2=-1; i<max; ++i) {
      int c = src[i];
      if (!c) break;
      if (c == '.') ext2 = ext, ext = i;
      else if (c == '_') c = ' ';
      dst[i] = c;
    }
    dst[i] = 0;
    if (ext2 > 0) {
      if (!stricmp(src+ext, ".gz")) {
	dst[ext] = 0;
	ext = ext2;
	lim += 3;
      }
    }
    if (ext>0 && i-ext <= lim) dst[ext] = 0;

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
    
    SDDEBUG("Verify '%s'\n", verify);
    e = entrylist_addrof(&direntry, selected);
    if (!e || strcmp(e->fn, verify)) {
      entry findme;
      
      SDDEBUG("'%s' not at #%d\n", verify, selected);
      strncpy(findme.fn, verify, sizeof(findme.fn));
      selected = entrylist_find(&direntry,
				&findme,
				(int (*)(const void *, const void *))
				cmp_entry_fn);
      if (selected<0) {
        SDDEBUG("'%s' not found.\n", verify);
        selected = 0;
      } else {
        SDDEBUG("'%s' found at #%d.\n", verify, selected);
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

  SDDEBUG(">> %s(%s)\n", __FUNCTION__ , dirname);
	
  /* Abort if loader thread has been asked to exit */
  if (!running) {
    return 0;
  }
	
  if (entrylist_isfull(&playlist)) {
    SDWARNING("<< %s(%s) : list full\n", __FUNCTION__ , dirname);
    return 0;
  }

  l = strlen(dirname);

  d = fs_open(dirname, O_RDONLY | O_DIR);
  if (!d) {
    goto error;
  }
    
  while (running && (de = fs_readdir(d), de)) {
    int type;
    
    type = direntry_filetype(de);

    if (type <= FILETYPE_DIR) {
      /* Skip directory in first pass */
/*        SDDEBUG(, "** " __FUNCTION__  */
/*  	     "(%s) : Skip dir [%s] in 1st pass\n", dirname, de->name); */
      continue;
    }

/*      SDDEBUG(, "*** " __FUNCTION__  */
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
    int type;
    
    type = direntry_filetype(de);
    if (type != FILETYPE_DIR) {
      /* Only keep dir in 2nd pass, remove self, parent ... */
/*        SDDEBUG(, "** " __FUNCTION__  */
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
  SDDEBUG(">> %s() : [%d]\n", __FUNCTION__, cnt);
  
  return cnt;
}

static void load_song_dir()
{
  int cnt;
  
  SDDEBUG(">> %s()\n", __FUNCTION__);
  entrylist_sync(&direntry, 1);

  if (!loaddir[0]) {
    strcpy(loaddir, "/");
  }
	
  cnt = r_load_song_dir(loaddir);
  entrylist_sync(&direntry, 0);
  SDDEBUG("<< %s() : [%d]\n", __FUNCTION__ , cnt);
}

static void load_song_list(int selected)
{
  file_t d;
  int pc;
  entry e;
	
  dirent_t *de;
  int clear_cache;
	
  SDDEBUG(">> %s(%d)\n", __FUNCTION__, selected);

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
  if (!playa_isplaying()) {
    clear_cache   = !mystricmp(loaddir, "/cd");
    clear_cache  |= find_entry.fn[0]=='.' &&
      !find_entry.fn[1] &&
      strstr(loaddir,"/cd") == loaddir;
  } 
  for (d=0; !d && clear_cache < 3; ) {
    if (clear_cache) {
      SDDEBUG("Clear iso9660 cache [%d]\n", clear_cache);
      iso_ioctl(0,0,0);  /* clear CD cache ! */
    }
	
    d = fs_open(loaddir, O_RDONLY | O_DIR);
    if (!d) {
      if (clear_cache) {
	SDDEBUG("Can't open %s. Try with /\n", loaddir);
	strcpy(loaddir, "/");
      }
      ++clear_cache;
    }
  }

  if (!d) {
    strcpy(curdir,loaddir);
    entrylist_sync(&direntry, 0);
    find_entry.fn[0] = 0;
    SDDEBUG("<< %s() : FAILED.", __FUNCTION__);
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
	  
    if (0 && pc) {
      SDDEBUG("READ entry '%s' [%d]\n", de->name, de->size);
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
  SDDEBUG("<< %s()\n", __FUNCTION__);
}

static volatile int load_list = 0;

static void loader_thread(void *dummy)
{
  SDDEBUG(">> %s()\n", __FUNCTION__);
  
  while (running) {
    if (load_list == LOAD_RECURSIVE_DIR) {
      load_song_dir();
      load_list = 0;
    } else if (load_list) {
      int new_selected = ~load_list;
      load_list = 0;
      SDDEBUG("Asked to load a new list (cursor at %d)\n", new_selected);
      load_song_list(new_selected);
    } else {
      /* wait in ms */
      thd_sleep(200);
    }
  }
  
  SDNOTICE("Asked to exit\n");
  loader_thd = 0;
  SDDEBUG("<< %s()\n", __FUNCTION__);
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

  //  SDDEBUG(," [%.3f %.3f]", global_alpha, halftint);
	
  if (halftint < 1E-3) {
    return 0;
  }

  /* List is busy ... skip refresh */
  if (!win->list->sync) {
	text_set_color(fade68, 1.0f, 1.0f, 1.0f);
    draw_poly_text(xs, ys, 100.0f, 
		   "> Please wait . . . busy . . . <");
    return 1;
  }
	
  /* Lock list for this op. */
  entrylist_sync(win->list, 1);

  num_entries = win->list->nb;
	
  if (num_entries <= 0) {
	text_set_color(colors[0].a * halftint, colors[0].r, colors[0].g,
				   colors[0].b);
    draw_poly_text(xs, y, 100.0f, "<empty>");
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

	/* 2nd chance : try to find it again */
	if (!d) {
	  e->type = file_type_inp(e->fn);
	  d = inp_driver_list_search_by_id(e->type);
	}

	if (d) {
	  tmp[0] = 2;
	  tmp[1] = 0;
	  //strcpy(tmp, d->extensions);
	  strcat(tmp, " ");
	  strcat(tmp, s);
	  s = tmp;
	  //SDDEBUG(, "!! driver found for %s\n", s);
	} else {
	  SDERROR("Driver not found for [%s]\n", s);
	}
      } else if (e->type == FILETYPE_UNKNOWN) {
	color_idx = (color_idx & 1) + 4;
      } else if (e->type == FILETYPE_PLAYLIST) {
	color_idx = (color_idx & 1) + 6;
      }

      if (color_idx >= sizeof(colors) / sizeof(*colors)) {
	SDCRITICAL("Bad color idx:%d\n", color_idx);
	color_idx = 0;
      }

	  text_set_color(colors[color_idx].a * halftint, colors[color_idx].r,
					 colors[color_idx].g, colors[color_idx].b);
      ystep = draw_poly_text(xs, y, 100.0f, s);
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
  if (playlist_start_idx < 0 || playa_isplaying()) {
    return;
  }
  
  /* Start playlist */
  if (imm = playwin->play < 0, imm) {
    SDDEBUG("Start playlist at #%d\n", playlist_start_idx);
    playwin->play = playlist_start_idx - 1;
    if (option_shuffle()) {
      entrylist_shuffle(&playlist, playlist_start_idx);
    }
  }
  
  /* Find next valid idx */
  while (++playwin->play < playlist.nb) {
    le = entrylist_addrof(&playlist, playwin->play);
    SDDEBUG("Playlist load #%d '%s'\n", playwin->play, le->fn);
    if (!playa_start(le->fn, -1, imm)) {
      return;
    }
    SDERROR("Failed loading #%d '%s'\n", playwin->play, le->fn);
  }
  SDDEBUG("End of list reached\n");
  
  /* End of list */
  playlist_start_idx = playwin->play = -1;
}

static int add_entry_to_playlist(lst_entry * le, int insert)
{
  int idx;

  idx = insert ? playwin->selected + 1 : -1;
/*    SDDEBUG(, "** " __FUNCTION__ " : #%d %s ['%s' %d]\n", idx, */
/*  	 insert ? "INSERT" : "ENQUEUE", le->fn, le->size); */
  idx = entrylist_add(&playlist, le, idx);
  if (idx >= 0) {
/*      SDDEBUG(, "** " __FUNCTION__ " : ADDED #%03d ['%s' %d]\n", idx, */
/*  	   le->fn, le->size); */
    
    if (playwin->play >= 0 && idx <= playwin->play) {
      /* Insert before current playlist, shift */
      ++playwin->play;
    }			 
    if (insert) {
      ++playwin->selected;
    }
  } else {
    SDERROR("FAILED #%03d ['%s' %d]\n",
	    playwin->selected, le->fn, le->size);
  }
  return idx;
}

static int m3u_make_filepath(char *path, int max,
			     const char *fname, const char * m3upath)
{
  int i = 0;
  int c;

  c = mytoupper(fname[0]); 
  if (c>='A' && c<='Z'  && fname[1] == ':') {
    /* Looks like a DOS path ! Assume it was a CDROM device !!! */
    fname += 2;
    if (fname[0] == '/') {
      strncpy(path, "/cd", max);
      i = 3;
    }
  }

  if (fname[0] != '/') {
    /* Relatif path : add m3u path. */
    while (i<max && *m3upath) {
      path[i++] = *m3upath++;
    }
    /* Append missing '/' */
    if (i>0 && i<max && path[i-1] != '/') {
      path[i++] = '/';
    }
  }

  /* Append leaf name */
  while (i<max && *fname) {
    path[i++] = *fname++;
  }

  if (i<max) {
    path[i] = 0;
    return 0;
  }

  return -1;
}

static int m3u_make_listentry(lst_entry *le, const char *m3upath,
			  const M3Uentry_t * e)
{
  SDDEBUG("%s([%s] [[%s] [%s]])\n", __FUNCTION__, m3upath,
	  nullstr(e->path) , nullstr(e->name));

  /* Verify */
  if (!e->path || !e->path[0]) {
    SDERROR("Invalid m3u entry path\n");
    return -1;
  }

  /* Build size */
  le->size = 0; /* $$$ Don't known real size, but it should be ok */

  /* Build type */
  le->type = file_type_inp(e->path);
  if (le->type == FILETYPE_UNKNOWN) {
    /* $$$ Ben: Here entry should not be remove. We could let it a last
       chance when playing starts. */
    SDERROR("Unknown filetype.\n");
    return -1;
  }

  /* Build name */
  if (e->name && e->name[0]) {
    /* m3u has an extended name info. */
    strncpy(le->name, e->name, sizeof(le->name));
  } else {
    /* use path to build name */
    make_mp3_name(le->name, e->path, sizeof(le->name));
  }
  le->name[sizeof(le->name)-1] = 0; /* safety net. */

  /* Build path */
  if (m3u_make_filepath(le->fn, sizeof(le->fn), e->path, m3upath)) {
    SDERROR("Can't build m3u entry path.\n");
    return -1;
  }
  SDDEBUG("-->[%s]\n", le->fn);
  
  return 0;
}

static int add_m3u_to_playlist(char *m3ufile, int insert)
{
  int fd = 0;
  M3Ulist_t * m3u = 0; 
  int err = -1;
  char *s;
  int i;
    
  SDDEBUG(">> %s([%s],[%s])\n", __FUNCTION__, m3ufile,
	  insert ? "INSERT" : "ENQUEUE");
  SDINDENT;

  mycookie = fd = fs_open(m3ufile, O_RDONLY);
  if (!fd) {
    SDERROR("open error\n");
    goto error;
  }
  
  m3u = M3Uprocess();
  if (!m3u) {
    SDERROR("load error\n");
    goto error;
  }

  SDDEBUG("m3u list loaded : %d entries\n", m3u->n);

  /* Get m3u file path. */
  s = strrchr(m3ufile,'/');
  if (s) *s = 0;

  err = 0;
  for (i=0; i<m3u->n; ++i) {
    M3Uentry_t *e = m3u->entry + i;
    lst_entry le;
    int err2;

    SDDEBUG("#%02d [%s] [%s] [%d]\n", i+1,
	    nullstr(e->path), nullstr(e->name), e->time);
    err2 = m3u_make_listentry(&le, m3ufile, e);
    err -= err2<0;
    if (err2<0) {
      continue;
    }
    SDDEBUG("-> [%s]\n", le.fn);
    err2 = add_entry_to_playlist(&le, insert);
    err -= err2<0;
  }
  err = 0; /* $$$ Discard error */

 error:
  if (fd) {
    fs_close(fd);
  }
  if (m3u) {
    M3Ukill(m3u);
  }
  mycookie = 0;
  return err;
}

static int add_to_playlist(int selected, int insert)
{
  const entry * e = (const entry *)entrylist_addrof(&direntry, selected);
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
    if (!make_full_path_name(le.fn, curdir, e->fn, sizeof(le.fn))) {
      strcpy(le.name, e->name);
      le.size = e->size;
      le.pos = selected;
      return add_entry_to_playlist(&le, insert);
    } else {
      SDWARNING("Skipped (filename too long) '%s/%s'\n", curdir, e->fn);
      return -1;
    }
  }

  /* Playlist file : $$$ TODO */
  if (e->type >= FILETYPE_PLAYLIST) {
    if (!make_full_path_name(le.fn, curdir, e->fn, sizeof(le.fn))) {
      SDDEBUG("%s playlist [%s]\n", insert?"Insert":"Enqueue", le.fn); 
      return add_m3u_to_playlist(le.fn, insert);
      
    } else {
      SDWARNING("Skipped (filename too long) '%s/%s'\n", curdir, e->fn);
      return -1;
    }

  }

  SDWARNING("Nothing to do with %s\n", e->fn); 
  return -1;

}

static int del_from_playlist(int idx)
{
  int err = entrylist_del(&playlist, idx);
  
  if (!err) {
    if (idx == playwin->play) {
      SDDEBUG("Delete current play : stop play list\n");
      playlist_start_idx = playwin->play = -1;
      //playa_loaddisk(0,0);
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
    SDDEBUG("Item #%d deleted\n", idx);
    return 0;
  } else {
    SDERROR("Item not in playlist\n");
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
      playa_stop(1);
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
	      playa_start(tmpstr, -1, 1) < 0 ? -1 : selected;
	  }
	}
      } else if (e->type >= FILETYPE_PLAYLIST) {
	/* Playlist : insert/enqueue */
	add_to_playlist(selected, playwin->play < 0);
      } else if (e->type <= FILETYPE_DIR) {
	/* Directory */
	SDDEBUG("(A) Entering dir '%s'\n", e->fn);
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
	SDDEBUG("cd '%s'\n", loaddir);
	loadlist = ~0; //~(selected <= 3 ? selected : 3);
      } else {
	/* Other (should be FILETYPE_UNKNOWN) */
	SDDEBUG("(A) nothing to do with [%s]\n", e->fn);
      }
    } else {
      /* A pressed in playlist : start/restart at index */
      playa_stop(1);
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
    SDDEBUG("Post load list, find '%s' at #%d\n",
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
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  running = 1;
  load_list = ~0;
  
  /* Entry loader thread */
  SDDEBUG("Starting loader thread\n");
  
  loader_thd = thd_create(loader_thread, 0);
  if (loader_thd) {
    SDDEBUG("OK loader thread [%p]:\n", loader_thd);
    //    thd_set_prio(loader_thd, PRIO_DEFAULT / 2);
    thd_set_label((kthread_t *)loader_thd, "Loader-thd");
  } else {
    SDERROR("FAILED entry loader thread\n");
  }
  
  SDUNINDENT;
  SDDEBUG("<< %s()\n", __FUNCTION__);
  return 0;
}

int songmenu_init(void)
{
  int i;
  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;
  
  framecnt = 0;
  running = 0;
  load_list = 0;

  /* Init menu text fade in/out */
  global_alpha_goal = 1.0f;
  global_alpha = 0.0f;
  timeout = timemax;
  
  /* Set M3U driver */
  M3Udriver(&driver);
  mycookie = 0;

  /* Browser entries */
  SDDEBUG("Init browser entries\n");
  if (entrylist_create(&direntry, 1024, sizeof(entry)) < 0) {
    SDERROR("browser entry failed\n");
    return -1;
  }

  /* Playlist entry */
  SDDEBUG("Init playlist entries\n");
  if (entrylist_create(&playlist, 1500, sizeof(lst_entry)) < 0) {
    SDERROR("playlist entry failed\n");
    return -1;
  }
  
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
  
  SDUNINDENT; 
  SDDEBUG("<< %s()\n", __FUNCTION__);
  return 0;
}

/* Shutdown loader-thread, kill browser and play lists  
 */
int songmenu_kill(void)
{
  SDDEBUG(">> %s()\n");
  SDINDENT;
  
  if (loader_thd) {
    int cnt = 50000;
    
    SDDEBUG("Ask loader thread to end.\n");
    running = 0;
    for (running=0; loader_thd && cnt; running=0, --cnt) {
      thd_pass();
    }
    SDDEBUG("Count down : %d\n", cnt);
  }
  
  if (loader_thd) {
    SDWARNING("Explicit kill of loader thread.\n");
    thd_destroy((kthread_t *)loader_thd);
    loader_thd = 0;
  }
  
  running = 0;
  entrylist_kill(&direntry);
  entrylist_kill(&playlist);

  SDUNINDENT; 
  SDDEBUG("<< %s()\n", __FUNCTION__);
  return 0;
}

/*  Get the current active window [0:browser 1:play-list] 
 */
int songmenu_current_window(void)
{
  return curwin == wins + 1;
}
