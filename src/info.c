/* 2002/02/18 */

#include <kos/thread.h>
#include <arch/spinlock.h>

#include "gp.h"
#include "info.h"
#include "controler.h"
#include "songmenu.h"
#include "playa.h"

#include "sysdebug.h"

/* Help PAD button colors */
#define A_BUTTON_COLOR fade_argb(0xFFFF6363)
#define B_BUTTON_COLOR fade_argb(0xFF00bded)
#define X_BUTTON_COLOR fade_argb(0xFFfff021)
#define Y_BUTTON_COLOR fade_argb(0xFF6fe066)
#define DPAD_COLOR     fade_argb(0xFFCCCCCC)
#define JOY_COLOR      fade_argb(0xFFEEEEEE)

/* Number of frame to auto close help */
#define HELP_CLOSE_FRAMES (60 * 15)

static int fps_mode;
static int help_mode;
static int help_close_frames;

extern controler_state_t controler68;
extern float fade68;

//extern spinlock_t app68mutex;
//extern volatile unsigned int play_status;

static void info_controler(uint32 elapsed_frames)
{
  /* START pressed and press A toggle FPS */
  fps_mode ^=
    ((controler68.buttons & (CONT_START | CONT_A)) ==
     (CONT_START | CONT_A)) && (controler68.buttons_change & CONT_A);

  /* Press START toggle help mode */
  if ((controler68.buttons & controler68.buttons_change & CONT_START)) {
    help_mode ^= 1;
    help_close_frames = (help_mode) ? HELP_CLOSE_FRAMES : 0;
  }

  /* Help auto close */
  if (!(controler68.buttons & CONT_START) && help_close_frames
      && (help_close_frames -= elapsed_frames) <= 0) {
    help_close_frames = 0;
    help_mode = 0;
  }
	
  //$$$
  if (fps_mode) {
    dbglog_set_level( DBG_DEBUG );
  }
}

static unsigned int fade_any_argb(unsigned int argb, unsigned int factor)
{
  return (argb&0xFFFFFF) | ((((argb>>24)*factor)&0xFF00)<<16);
}

unsigned int fade_argb(unsigned int argb)
{
  return fade_any_argb(argb, (unsigned int)(fade68 * 256.0f));
}

static void render_fps(uint32 elapsed_frames, const float x, const float y)
{
  /*if (fps_mode)*/ {
    char tmp[128];
    
    sprintf(tmp,
	    "%%cFPS %02u "
	    "%%cSTATUS %s "
	    "%%cfade=%3.2f",
	    (unsigned int)elapsed_frames,
	    playa_statusstr(playa_status()),
	    (double)fade68);
    draw_poly_text (x,y,100.0f,1.0f,1.0f,1.0f,1.0f,
                    tmp,
                    fade_argb(0xFFFFFF00),
                    fade_argb(0xFFFF00FF),
                    fade_argb(0xFF00FFc0),
                    fade_argb(0xFF7F7FFF)
                    );
  }
}

static void render_help(const float xs, const float ys)
{
  /* Display help */
  //	const unsigned int argb = (0xFFF0F0F0);
  //	const unsigned int hi   = (0xFF00FFEE);
  const unsigned int argb = (0xFF064769);
  const unsigned int hi   = (0xFF466a7f);
  float x, y;
  const float space = 3.0f;
	
  const unsigned int fade0 = (unsigned int)(fade68 * 256.0f);
  unsigned int fade1 = fade0;
  unsigned int fade2 = (unsigned int)(fade68 * 256.0f * 0.40f);
  
  if (songmenu_current_window()) {
    fade1 ^= fade2;
    fade2 ^= fade1;
    fade1 ^= fade2;
  }
	
  /* Directory window help */

  x = WIN68_DIR_X;
  y = ys;

  y += space + draw_poly_text(x, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			      "%c%A  %cPlay%c or %cEnqueue",
			      fade_any_argb(A_BUTTON_COLOR, fade1),
			      fade_any_argb(hi, fade1),
			      fade_any_argb(argb, fade1),
			      fade_any_argb(hi, fade1));

  y += space + draw_poly_text(x, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			      "%c%B  %cStop%c. Reset play-list cursor",
			      fade_any_argb(B_BUTTON_COLOR, fade1),
			      fade_any_argb(hi,fade1),
			      fade_any_argb(argb,fade1),
			      fade_any_argb(hi,fade1));

  y += space + draw_poly_text(x, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			      "%c%X  %cInsert%c after cursor",
			      fade_any_argb(X_BUTTON_COLOR,fade1),
			      fade_any_argb(hi,fade1),
			      fade_any_argb(argb,fade1));

  /* Play list window help */

  x = WIN68_PLAYLIST_X;
  y = ys;

  y += space + draw_poly_text(x, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			      "%c%A  %cRun%c play-list at cursor",
			      fade_any_argb(A_BUTTON_COLOR,fade2),
			      fade_any_argb(hi,fade2),
			      fade_any_argb(argb,fade2));

  y += space + draw_poly_text(x, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			      "%c%B  %cRemove%c & %cStop%c if playing location",
			      fade_any_argb(B_BUTTON_COLOR,fade2),
			      fade_any_argb(hi,fade2),
			      fade_any_argb(argb,fade2),
			      fade_any_argb(hi,fade2),
			      fade_any_argb(argb,fade2));

  y += space + draw_poly_text(x, y, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f,
			      "%c%X  %cLocate%c file",
			      fade_any_argb(X_BUTTON_COLOR,fade2),
			      fade_any_argb(hi,fade2),
			      fade_any_argb(argb,fade2));

  y += space;
  y += space + draw_poly_center_text(y, 100.0f, 1.0f, 1.0f, 1.0f,1.0f,
				     "%cKeep %c%Y%c pressed for %coptions",
				     fade_any_argb(argb,fade0),
				     fade_any_argb(Y_BUTTON_COLOR,fade0),
				     fade_any_argb(argb,fade0),
				     fade_any_argb(hi,fade0));
	  
  y += 7;
  draw_poly_center_text(y, 100.0f, 1.0f, 1.0f, 1.0f,1.0f,
			"%c%+%c & %c%o%c move%c cursor, window focus & options",
			fade_any_argb(DPAD_COLOR,fade0),
			fade_any_argb(argb,fade0),
			fade_any_argb(JOY_COLOR,fade0),
			fade_any_argb(hi,fade0),
			fade_any_argb(argb,fade0));
}

void append(char * dest, char *what)
{
  if (what) {
    strcat(dest, " - ");
    strcat(dest, what);
  }
}

static void render_diskinfo(const float xs, const float ys)
{
  float x, y = ys, ystep = 0;

  static float title_color[4] = { 0,  1.0f, 1.0f, 1.0f };
  static float info_color[4] =  { 0,  1.0f, 1.0f, 1.0f  };
	
  playa_info_t *info = playa_info_lock();
	
  static char tmp[2048]; 

  /* ID3 info */
  /*
    char * album;
    char * title;
    char * artist;
    char * year;
    char * genre;
    char * track;
    char * comments;
  */
  
  float *rgb = info_color;
  title_color[0] = info_color[0] = fade68 * 0.55f;

  {
    int h,m,s,t;
    
    t = playa_playtime();
    if (t < 0) {
      SDDEBUG("BUGS time = %d\n",t);
      t = 0;
    }
    
    h = t / (3600<<10);
    m = t / (60<<10) % 60;
    s = t % (60<<10) / 1000;
    if (h) {
      sprintf(tmp,"%u:%02u:%02u",h,m,s);
    } else {
      sprintf(tmp,"%02u:%02u",m,s);
    }
    x =  600.0f;
  }

  y = 340.0f;
  
  /* Track time */  

  if (info->valid && info->info[PLAYA_INFO_TIMESTR].s) {
    float tw, th; 
    const char * timestr = info->info[PLAYA_INFO_TIMESTR].s;
    x = 600.0f;
    draw_poly_get_text_size(timestr, &tw, &th);
    x -= tw; 
    draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3], timestr);
  }
	
  /* Playing time */
  {
    float tw, th;
    x = 525;
    draw_poly_get_text_size(tmp, &tw, &th);
    x -= tw;
    draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3], tmp);
  }
	
  /* Track */
  if (info->valid && info->info[PLAYA_INFO_TRACK].s) {
    char * trkstr = info->info[PLAYA_INFO_TRACK].s;
    float tw, th;
    x = 451;
    draw_poly_get_text_size(trkstr, &tw, &th);
    x -= tw;
    draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3], trkstr);
  }
	

  x = 92;
  rgb = info_color;

  /* Format */	
  if (info->valid && info->info[PLAYA_INFO_FORMAT].s) {
    draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3],
		   info->info[PLAYA_INFO_FORMAT].s);
  }

  y = 363;
  /* Album */
  if (info->valid && info->info[PLAYA_INFO_ALBUM].s) {
    draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3],
		   info->info[PLAYA_INFO_ALBUM].s);
  }
	
  /* Genre */
  if (info->valid && info->info[PLAYA_INFO_GENRE].s) {
    draw_poly_text(413, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3],
		   info->info[PLAYA_INFO_GENRE].s);
    
  }
  
  /* Year */
  if (info->valid && info->info[PLAYA_INFO_YEAR].s) {
    draw_poly_text(560, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3],
		   info->info[PLAYA_INFO_YEAR].s);
  }
	
  /* Artist */
  if (info->valid && info->info[PLAYA_INFO_ARTIST].s) {
    y = 386;
    ystep = draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3], 
			   info->info[PLAYA_INFO_ARTIST].s);
  }

  /* Title */
  if (info->valid && info->info[PLAYA_INFO_TITLE].s) {
    y = 409;
    ystep = draw_poly_text(x, y, 100.0f, rgb[0], rgb[1], rgb[2], rgb[3],
			   info->info[PLAYA_INFO_TITLE].s);
  }
	
  rgb = title_color;
  if (info->valid && info->info[PLAYA_INFO_COMMENTS].s) {
    y = 434;
    x = xs+4;
    rgb = info_color;
    ystep = draw_poly_text(x, y, 100.0f, rgb[0], 0,0, 0,
			   info->info[PLAYA_INFO_COMMENTS].s);
  }

  playa_info_release(info);

}

int info_is_help(void)
{
  return help_mode;
}

void info_render(uint32 elapsed_frames, int is_playing)
{
  const float xs = 38.0f;
  const float ys = 336.0f;
  
  info_controler(elapsed_frames);

  if (fps_mode) {
    render_fps(elapsed_frames, xs, 20.0f);
  }
  if (help_mode || !is_playing) {
    render_help(xs, ys);
  } else {
    render_diskinfo(xs, ys);
  }
}  

static void change_something(char *dest, const char * src, int max)
{
  if (!dest || !src || !max) {
    return;
  }
  dest[max-1] = 0;
  strncpy (dest, src, max-1);
}

int info_setup(void)
{
  fps_mode = 0;
  help_mode = 1;
  help_close_frames = HELP_CLOSE_FRAMES;
  return 0;  
}
