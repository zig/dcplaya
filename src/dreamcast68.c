/**
 * @file      dreamcast68.c
 * @author    ben(jamin) gerard <ben@sashipa.com>
 * @date      2002/02/08
 * @brief     sc68 for dreamcast - main for kos 1.1.x
 * @version   $Id: dreamcast68.c,v 1.30 2002-10-30 20:01:20 benjihan Exp $
 */

//#define RELEASE
#define SKIP_INTRO


#define CLIP(a, min, max) ((a)<(min)? (min) : ((a)>(max)? (max) : (a)))

#define SCREEN_W 640
#define SCREEN_H 480

/* generated config include */
#include "config.h"

#include <dc/fmath.h>
#include <stdio.h>

#include "sndstream.h"
#include "songmenu.h"
#include "gp.h"

/* dreamcast68 includes */
#include "file_wrapper.h"
#include "matrix.h"
#include "obj3d.h"
#include "controler.h"
#include "info.h"
#include "option.h"
#include "remanens.h"

#include "playa.h"
#include "vupeek.h"
#include "driver_list.h"
#include "inp_driver.h"
#include "playa.h"
#include "plugin.h"
#include "lef.h"
#include "fft.h"
#include "viewport.h"
#include "fs_ramdisk.h"
#include "screen_shot.h"

#include "sysdebug.h"
#include "syserror.h"
#include "console.h"
#include "shell.h"
#include "display_list.h"
#include "texture.h"

#include "exceptions.h"

float fade68;
uint32 frame_counter68 = 0;
int help_close_frames = 0;
controler_state_t controler68;

static obj_t * curlogo;

extern void vmu_lcd_update(int *b, int n, int cnt);
extern int vmu68_init(void);
extern int vmu_lcd_title();

static int warning_splash(void);
extern void warning_render();

static viewport_t viewport; 
static matrix_t projection;
static vis_driver_t * curvis;

#define ANIM_CONT   0
#define ANIM_CYCLE  1
#define ANIM_END    2

typedef struct
{
  float x,y,z;
  float ax,ay,az;
  float zoom;
  float flash;
  obj_t *o;
} anim_t;

static anim_t animdata;

typedef int (*anim_f)(anim_t *, unsigned int frames);


static float fade_step;

static void fade(unsigned int elapsed_frames)
{
  const float step = fade_step * (float)elapsed_frames;
  fade68 += step;
  if (fade68 < 0.0f) {
    fade68 = 0;
  } else if (fade68 > 1.0f) {
    fade68 = 1.0f;
  }
}

/* Create a empty triangle to avoid TA empty list */
static void pipo_poly(int mode)
{
  poly_hdr_t poly;
  vertex_oc_t vert;
  int i;
	
  ta_poly_hdr_col(&poly, !mode ? TA_OPAQUE : TA_TRANSLUCENT);
  ta_commit_poly_hdr(&poly);

  vert.a = vert.r = vert.g = vert.b = 0;
	
  for (i=0;i<3;++i) {
    vert.flags = (i!=2) ? TA_VERTEX_NORMAL : TA_VERTEX_EOL;
    vert.x = (float)(!!i);
    vert.y = (float)(i>1);
    vert.z = 1.0f;
    ta_commit_vertex(&vert, sizeof(vert));
  }
}

#ifndef RELEASE
#define VCOLOR my_vid_border_color
static void my_vid_border_color(int r, int g, int b)
{
  vid_border_color(r,g,b);
}
#else
# define my_vid_border_color(R,G,B)
#endif

/* Sound first init : load arm code and setup streaming */
static int sound_init(void)
{
  playa_init();
  return 0;
}

static void sature(float *a, const float min, const float max)
{
  float f = *a;

  if (f<min) f = min;
  else if (f>max) f=max;
  *a = f;
}


static void render_fx(int age_per_trace, int trace_age)
{
#if 0
  const float amax = 0.5f, amin = 0.1f;
  const unsigned int this_frame = frame_counter68;
  
  matrix_t proj;
  unsigned int tframe;
  int nb, wanted_age;
  float sa =  (amax - amin) / trace_age;
  float z = 80.0f;

  tframe = this_frame - trace_age + 1;
  if (tframe > this_frame) {
    tframe = 0;
  }
  remanens_remove_old(tframe);
  
  wanted_age = 0; 
  nb = remanens_nb();
  while (--nb >= 0) {
    remanens_t *r;
    int age;
     
    r = remanens_get(nb);
    if (!r) continue; /* safety net */
     
    age = this_frame - r->frame;
    while (age > wanted_age) {
      wanted_age += age_per_trace;
    }
    
    if (age == wanted_age) {
      float a = amax - (float)age * sa;
      DrawObject(r->o, r->mtx, proj, z, a * fade68, 0);
    }
  }
#endif
}


static int render_anim_object(uint32 elapsed_frames, anim_f anim, anim_t *data)
{
#if 0
  /* Trace constants */
  const int age_per_trace = 5;
  const int max_trace = 5;
  int trace_age;

  /* Frame adapt system */
  static uint32 pass_acu = 0;
  static uint32 elapsed_acu = 0;
  static int ntrace = max_trace;
  
  int err;

  /* Adapt number of trace acording to frame rate */    
  if (pass_acu == 128) {
    if (elapsed_acu > 130) {
      if (ntrace > 0) {
        --ntrace;
      }
    } else if (elapsed_acu == pass_acu && ntrace < max_trace) {
      ++ntrace;
    }
    pass_acu = elapsed_acu = 0;
  }
  elapsed_acu += elapsed_frames;
  pass_acu++;

  /* Anim & push to renanens */
  err = anim(data, elapsed_frames);
  
  if (data->o) {
    matrix_t m;
    MtxIdentity(m);
    MtxRotateZ(m, data->az);
    MtxRotateX(m, data->ax);
    MtxRotateY(m, data->ay);
    MtxScale(m,data->zoom);
    m[3][0] = data->x;
    m[3][1] = data->y;
    m[3][2] = data->z;
    remanens_push(data->o, m, frame_counter68);
  }

  if (ntrace > 0) {
    trace_age = ntrace * age_per_trace;
    render_fx(age_per_trace, trace_age);
  }
  return err;
#endif
}

static int render_simple_anim_object(uint32 elapsed_frames, anim_f anim,
				     anim_t *data)
{
#if 0
  int err;
  matrix_t m;

  /* Anim & push to renanens */
  err = anim(data, elapsed_frames);
  
  if (data->o) {
    MtxIdentity(m);
    MtxRotateZ(m, data->az);
    MtxRotateX(m, data->ax);
    MtxRotateY(m, data->ay);
    MtxVectMult(&tlight_normal.x, &light_normal.x, m);
    MtxScale(m,data->zoom*8*4);
    m[3][0] = data->x;
    m[3][1] = data->y;
    m[3][2] = data->z;

    matrix_t tmp;
    MtxProjection(tmp, 70*2.0*3.14159/360,
		  0.01, (float)SCREEN_W/SCREEN_H,
		  1000);
    MtxMult(m, tmp);
  }


  if(0) {
    DrawObject(data->o, m, 0, 80.0f, 1.0f, data->flash);
  } else {
    static int init = 0;

    if (!init) {
      //vlr_init();
      init = 1;
    }
    MtxIdentity(m);
    MtxRotateZ(m, 3.14159);
    MtxRotateY(m, 0.1*data->ay);
    MtxRotateX(m, 0.4);


    matrix_t tmp;

/*    MtxCopy(tmp, m);*/
    MtxIdentity(tmp);
    MtxRotateZ(tmp, 3.14159);
    MtxRotateY(tmp, -0.33468713*data->ay);
    MtxRotateX(tmp, 0.4);
    MtxTranspose(tmp);
    MtxVectMult(&tlight_normal.x, &light_normal.x, tmp);


    data->zoom = 1;
    MtxScale(m,data->zoom*32*4);
    m[3][0] = data->x;
    m[3][1] = data->y;
    m[3][2] = data->z;

    MtxProjection(tmp, 70*2.0*3.14159/360,
		  0.01, (float)SCREEN_W/SCREEN_H,
		  1000);
    MtxMult(m, tmp);

    //vlr_update();
    //vlr_render(m);
  }
  return err;
#endif
}



static int anim_intro(anim_t *a, unsigned int elapsed_frames)
{
#if 0
  static obj_t * objects[] = { &trois, &deux, &un, &zero, 0 };
  static int cur_obj = 0;
  //  const int recount = 200;
  //  static int frame_cnt = recount;

  const float sax =  0.0201f * 0.7f;
  const float say =  0.0212f * 3.07f;
  //  const float pi = 3.1415926f;
  const float deux_pi = 6.283185f;
  
  int err = ANIM_CONT;

  a->x = a->y = 0.0f;
  a->z = 80.0f;
  a->zoom = 1.7f;
  a->flash = 0;

  a->ay += say * elapsed_frames;
  a->ax += sax * elapsed_frames;
  if (a->ay > deux_pi) {
    a->ay -= deux_pi;
    cur_obj++;
    err = ANIM_CYCLE;
  }
  
  if (a->o = objects[cur_obj], !a->o) {
    err = ANIM_END;
  }
  /* else {
     dbglog( DBG_DEBUG, "-- " __FUNCTION__ " : Change object vtx:%d/%d tri:%d/%d\n",
     a->o->nbv, a->o->static_nbv, a->o->nbf, a->o->static_nbf);
     }*/
  

  /*  
      frame_cnt -= elapsed_frames;
      if (frame_cnt<=0) {
      frame_cnt += recount;
      cur_obj++;
      }
  */

  return err;
#else
  return ANIM_END;
#endif
}

static int render_intro(uint32 elapsed_frames)
{
  int code;
  
  code = render_anim_object(elapsed_frames, anim_intro, &animdata);
  
  return code == ANIM_END;
}

//extern unsigned int vmu_get_peek();

static int same_sign(float a, float b) {
  return (a>=0 && b>=0) || (a<0 && b<0);
}

/* extern inp_driver_t sidplay_driver; */

static int load_builtin_driver(void)
{
  any_driver_t **d, *list[] = {
/*     &sidplay_driver.common, */
    0
  };
  for (d=list; *d; ++d) {
    /* Init driver and register it. */
    if (!(*d)->init(*d)) {
      driver_register(*d);
    }
  }
  return 0;
}

static int driver_init(void)
{
  int err=0;

  SDDEBUG(">> %s()\n", __FUNCTION__);
  SDINDENT;

  /* Init driver list */
  SDDEBUG("Init decoder driver list\n");
  err = driver_list_init_all();
  if (err < 0) {
    goto error;
  }

  /* Load the default drivers from romdisk */
  /* VP : No more necessary, load drivers from dcplayarc.lua instead ! */
#if 0
  {
    const char **p, *paths[] = {
      "/pc" DREAMMP3_HOME "plugins/obj",
      "/pc" DREAMMP3_HOME "plugins/vis/lpo",
      "/pc" DREAMMP3_HOME "plugins/vis/fftvlr",
      "/pc" DREAMMP3_HOME "plugins/inp/xing",
/*      "/pc" DREAMMP3_HOME "plugins/inp/ogg",
      "/pc" DREAMMP3_HOME "plugins/inp/sc68",
*/
//      "/pc" DREAMMP3_HOME "plugins/inp/sidplay",
//     "/pc" DREAMMP3_HOME "plugins/inp/spc",
      0
    };

    for (p=paths; *p; ++p) {
      SDDEBUG("[%s] : Load default drivers in [%s]\n", __FUNCTION__, *p);
      err |= plugin_path_load(*p, 1);
    }
  }
#endif
  /* $$$ Ignore errors  */

  err = load_builtin_driver();

 error:
  SDUNINDENT;
  SDDEBUG("<< %s := %d\n", __FUNCTION__, err);
  return err;
}

static int no_mt_init(void)
{
  int err = 0;
  SDDEBUG(">> %s\n", __FUNCTION__);

  /* ramdisk init */
  if (fs_ramdisk_init(0) < 0) {
    SDERROR("ramdisk init failure");
    err = __LINE__;
    goto error;
  }

  /* Viewport init */
  viewport_set(&viewport, 0, 0, SCREEN_W, SCREEN_H, 1.0f);

  /* Projection init */
  MtxProjection(projection, 70*2.0*3.14159/360,
		0.01, (float)SCREEN_W/SCREEN_H,
		1000);

  /* Texture manager init */
  if (texture_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Driver list */
  if (driver_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Start sound */
  if (sound_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Setup background display */
  if (bkg_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Setup border poly */
  if (border_setup() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Init VMU stuff */
  if (vmu68_init() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Init font 16x16 */
  if (text_setup() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Init song menu */
#if 0
  if (songmenu_init() < 0) {
    err = __LINE__;
    goto error;
  }
#endif
  
  /* Init info */
  if (info_setup() < 0) {
    err = __LINE__;
    goto error;
  }
  
  /* Init option (must be done after visual plugin load)  */
  if (option_setup() < 0) {
    err = __LINE__;
    goto error;
  }

  /* Remanens FX */
  if (remanens_setup() < 0) {
    err = __LINE__;
    goto error;
  }
  
  /* Start controller */
  if (controler_init(frame_counter68) < 0) {
    err = __LINE__;
    goto error;
  }

  /* Start display_list */
  dl_open();


  /* Call "dcplayarc.lua" */
  shell_command("dofile (home..[[dcplayarc.lua]])");
  //shell_wait();


 error:
  SDDEBUG("<< %s : error line [%d]\n",__FUNCTION__, err);
  return err;
}

static void update_lcd(void)
{
  int *buf, nb, cnt, frq;

  playa_get_buffer(&buf, &nb, &cnt, &frq);
  vupeek_adddata(buf, nb, cnt, frq);
  vmu_lcd_update(buf, nb, cnt);
}

static void update_fft(void)
{
  int *buf, nb, cnt, frq;
  static int scnt = -1;

  playa_get_buffer(&buf, &nb, &cnt, &frq);
  if (cnt == scnt) return;
  scnt = cnt;
  fft(buf, nb, cnt, frq);

}


static void process_visual(unsigned int elapsed_frames)
{
  const float ms = elapsed_frames * (1000.0f / 60.0f);

  curvis = option_visual();
  if (curvis) {
    curvis->process(&viewport, projection, ms);
  }
}

static void render_visual_opaque(void)
{
  if (curvis) {
    curvis->opaque_render();
  }
}

static void render_visual_translucent(void)
{
  if (curvis) {
    curvis->translucent_render();
  }
}

void main_thread(void *cookie)
{
  const int exit_buttons = CONT_START|CONT_A|CONT_Y;
  const int shot_buttons = CONT_START|CONT_X|CONT_B;
  int err = 0;

  SDDEBUG(">> %s()\n", __FUNCTION__);

  //  vid_border_color(0,0,0);

#if 0
  if (songmenu_start() < 0) {
    err = __LINE__;
    goto error;
  }
#endif

  /* Load default disk from ROM */
  /*  if (dreamcast68_loaddisk("/rd/test.mp3", 1) < 0) {
      err = __LINE__;
      goto error;
      }*/
  playa_start("/rd/01 Intro.spc", -1, 1);
  thd_pass(); // $$$ Don't ask me why !!! It removes a bug in intro sound !!!

  fade68    = 0.0f;
  fade_step = 0.02f;
#ifdef SKIP_INTRO
  if (0)
#endif  
    {
      int end = 0;
      while (!end) {
	uint32 elapsed_frames;

	ta_begin_render();
	pipo_poly(0);

	/* Update FFT */
	update_fft();

	/* Update the VMU LCD */
	update_lcd();

	elapsed_frames = frame_counter68;
	frame_counter68 = ta_state.frame_counter;
	elapsed_frames = frame_counter68 - elapsed_frames;
	fade(elapsed_frames);
	controler_read(&controler68, frame_counter68);

	/* Update shell */
	shell_update(elapsed_frames * 1.0f/60.0f);

	/* Display opaque render list */
	dl_render_opaque();
      
	ta_commit_eol();

	end = render_intro(elapsed_frames);

	/* Display transparent render list */
	dl_render_transparent();
      
	ta_commit_eol();

	/* Finish the frame *******************************/
/*	extern kthread_t * playa_thread;
	if (playa_thread)
	  thd_set_prio(playa_thread, PRIO_DEFAULT);
	ta_finish_frame();
	if (playa_thread)
	  thd_set_prio(playa_thread, PRIO_DEFAULT-1);*/

	ta_finish_frame();

	// playa_decoderupdate();

      }
    }
  
  fade68    = 0.0f;
  fade_step = 0.005f;  
  while ( (controler68.buttons & exit_buttons) != exit_buttons) {
    uint32 elapsed_frames;
    int is_playing = playa_isplaying();

    SDDEBUG("%x \n",controler68.buttons & shot_buttons);
    if ((controler68.buttons & shot_buttons) == shot_buttons) {
      controler68.buttons &= ~shot_buttons;
      screen_shot("shot/shot");
    }

    //    my_vid_border_color(0,0,0);
    ta_begin_render();

    pipo_poly(0);
    
    elapsed_frames = frame_counter68;
    frame_counter68 = ta_state.frame_counter;
    elapsed_frames = frame_counter68 - elapsed_frames;
    fade(elapsed_frames);
    controler_read(&controler68, frame_counter68);

    
    /* Update shell */
    shell_update(elapsed_frames * 1.0f/60.0f);

    /* Update consoles */
    csl_update_all(elapsed_frames * 1.0f/60.0f);

    /* Update FFT */
    update_fft();

    /* Update the VMU LCD and calulate vupeek data */
    update_lcd();

    /* Make visual FX calculation */
    process_visual(elapsed_frames);

    /* Opaque list *************************************/
    //my_vid_border_color(255,0,0);
    bkg_render(fade68, info_is_help() || !is_playing);

    /* Visual opaque list */
    render_visual_opaque();
    
    /* Display opaque render list */
    dl_render_opaque();
      
    /* End of opaque list */
    ta_commit_eol();

    /* Translucent list ********************************/

    pipo_poly(1);

    /* Visual translucent list */
    render_visual_translucent();

    info_render(elapsed_frames, is_playing);
#if 0
    songmenu_render(elapsed_frames);
#endif
    option_render(elapsed_frames);
    //    my_vid_border_color(255,0,255);

    /* Render translucent consoles */
    csl_window_transparent_render_all();

    /* Display transparent render list */
    dl_render_transparent();

    /* End of translucent list */
    ta_commit_eol();

    /* Finish the frame *******************************/

/*    extern kthread_t * playa_thread;
    if (playa_thread)
      thd_set_prio(playa_thread, PRIO_DEFAULT);
    ta_finish_frame();
    if (playa_thread)
      thd_set_prio(playa_thread, PRIO_DEFAULT-1);*/
    
    ta_finish_frame();
    
    //    my_vid_border_color(0,0,0);    
  }

  SDDEBUG("Start exit procedure\n");

  /* Stop sc68 from playing */
  playa_stop(1);
  vmu_lcd_title();

  /* */
  stream_shutdown();

  /* Stop the sound */
  SDDEBUG("Disable SPU\n");
  spu_disable();
  SDDEBUG("SPU disabled\n");

  /* Stop songmenu */
#if 0
  songmenu_kill();
#endif
  err = 0;

  /* Stop controller thread */
  controler_shutdown();

  /* Stop display_list */
  dl_shutdown();

 error:
  if (cookie) {
    *(int *)cookie = err;
  }
  if (err) {
    SDERROR("Error line [%d]\n", err);
  } else {
    SDDEBUG("Exit procedure success\n");
  }
}

extern uint8 romdisk[];


/* Program entry :
 *
 * - Init some static variables
 * - KOS init
 * - Init all no thread safe
 * - Launch main thread and wait ...
 */
int dreammp3_main(int argc, char **argv)
{
  int err = 0;

  curlogo = 0; //& mine_3;
  fade68 = 0.0f;
  fade_step = 0.01f;
  memset(&animdata,0,sizeof(animdata));

#ifdef RELEASE
  dbglog_set_level(0);
#elif DEBUG_LEVEL > 1
  dbglog_set_level(DBG_KDEBUG);
#else
  dbglog_set_level(DBG_DEBUG);
#endif


  /* Do basic setup */

  kos_init_all(IRQ_ENABLE | THD_ENABLE, romdisk);
  //  vid_border_color(0,0,0);

  /* Initialize exceptions handling */
  expt_init();

  /* Initialize shell and LUA */
  SDDEBUG("SHELL init\n");
  if (shell_init()) {
    STHROW_ERROR(error);
  }

  /* Initialize the console debugging log facility */
  SDDEBUG("CONSOLE init\n");
  csl_init_main_console();
  csl_printf(csl_main_console, "TOTO !\n");

  ta_init(TA_LIST_OPAQUE_POLYS | TA_LIST_TRANS_POLYS, TA_POLYBUF_32, 1024*1024);
/*  ta_set_buffer_config(TA_LIST_OPAQUE_POLYS | TA_LIST_TRANS_POLYS, TA_POLYBUF_32, 1024*1024);
  ta_hw_init();*/

  frame_counter68 = ta_state.frame_counter;

  /* Run no multi-thread setup (malloc/free rules !) */
  if (no_mt_init() < 0) {
    err = __LINE__;
    goto error;
  }
  //return 0;

  //  texture_init();

  /* change main console render mode */
  csl_disable_render_mode(csl_main_console, CSL_RENDER_BASIC);
  csl_enable_render_mode(csl_main_console, CSL_RENDER_WINDOW);
  
  /* WARNING MESSAGE */
  SDDEBUG("Starting WARNING screen\n");
  warning_splash();
  SDDEBUG("End of WARNING screen\n");

  /* MAIN */
  main_thread(0);

  /* Close the console debugging log facility */
  csl_close_main_console();

error:
  dbglog_set_level(DBG_DEBUG);
  if (err) {
    SDERROR("Error line [%d]\n", err);
  } else {
    SDDEBUG("Clean exit\n");
  }

  BREAKPOINT(0x00DEAD00);

  /* Shutting down exceptions handling */
  expt_shutdown();

  return 0;
}

static int warning_splash(void)
{
  int end = 0;
  unsigned int end_frame = 0;

  fade68 = 0.0f;
  fade_step = 0.01f;

  vmu_lcd_title();

#ifdef SKIP_INTRO
  end = 1;
#endif
	
  while (!end) {
    uint32 elapsed_frames;

    ta_begin_render();
    pipo_poly(0);
    elapsed_frames = frame_counter68;
    frame_counter68 = ta_state.frame_counter;
    elapsed_frames = frame_counter68 - elapsed_frames;
    fade(elapsed_frames);
    controler_read(&controler68, frame_counter68);

    /* Update shell */
    shell_update(elapsed_frames * 1.0f/60.0f);

    if (controler_released(&controler68, CONT_START)) {
      end_frame = frame_counter68;
    }

    if (fade68 == 1.0f) {
      if (!end_frame) {
	end_frame = frame_counter68 + 60 * 30;
      } else if (frame_counter68 > end_frame) {
	fade_step = -0.01f;
      }
    }

    end = end_frame && fade68 == 0.0f;

    /* Opaque list ************************************ */
    /* End of opaque list */
    ta_commit_eol();

    /* Translucent list ******************************* */
    warning_render();

    /* End of translucent list */
    ta_commit_eol();

    /* Finish the frame ****************************** */
    ta_finish_frame();

  }
  return 0;
}
