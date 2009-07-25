#include "mpg123.h"
#include "libxmms/configfile.h"
#include "libxmms/titlestring.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define CPU_HAS_MMX() (cpu_fflags & 0x800000)
#define CPU_HAS_3DNOW() (cpu_efflags & 0x80000000)

static const long outscale = 32768;

static struct frame fr, temp_fr;

PlayerInfo *mpg123_info = NULL;
static pthread_t decode_thread;

static gboolean audio_error = FALSE, output_opened = FALSE, dopause = FALSE;
gint mpg123_bitrate, mpg123_frequency, mpg123_length, mpg123_layer, mpg123_lsf;
gchar *mpg123_title = NULL, *mpg123_filename = NULL;
static int disp_bitrate, skip_frames = 0;
static int cpu_fflags, cpu_efflags;
gboolean mpg123_stereo, mpg123_mpeg25;
int mpg123_mode;


real mpg123_compute_tpf(mpg123_frame_t * fr)
{
  const int bs[4] = {0, 384, 1152, 1152};
  real tpf;

  tpf = bs[fr->lay];
  tpf /= fr->info.sampling_rate << fr->lsf;
  return tpf;
}

void mpg123_config_default(mpg123_t * mpg123)
{
  mpg123->cfg.resolution = 16;
  mpg123->cfg.channels = 2;
  mpg123->cfg.downsample = 0;

  mpg123->cfg.http.buffer_size = 128;
  mpg123->cfg.http.prebuffer = 25;

  mpg123->cfg.proxy.port = 8080;
  mpg123->cfg.proxy.use_auth = 0;
  mpg123->cfg.proxy.user = 0;
  mpg123->cfg.proxy.pass = 0;

/*   mpg123->cfg.cast_title_streaming = FALSE; */
/*   mpg123->cfg.use_udp_channel = TRUE; */
/*   mpg123->cfg.title_override = FALSE; */
/*   mpg123->cfg.disable_id3v2 = FALSE; */
/*   mpg123->cfg.detect_by = DETECT_EXTENSION; */
/*   mpg123->cfg.default_synth = SYNTH_AUTO; */

}


int mpg123_init(void)
{
  int err;
  err = mpg123_make_decode_tables(outscale);
/*   mpg123_getcpuflags(&cpu_fflags, &cpu_efflags); */
  return err;
}

static unsigned int convert_to_header(unsigned char * buf)
{
  return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}

static unsigned int convert_to_long(unsigned char * buf)
{
  return (buf[3] << 24) + (buf[2] << 16) + (buf[1] << 8) + buf[0];
}

static void play_frame(struct frame *fr)
{
  if (fr->error_protection) {
    bsi.wordpointer += 2;
  }
  if ( !fr->do_layer(fr) ) {
    skip_frames = 2;
    mpg123_info->output_audio = FALSE;
  } else {
    if (!skip_frames)
      mpg123_info->output_audio = TRUE;
    else
      skip_frames--;
  }
}

static void *decode_loop(void *arg)
{
	gboolean have_xing_header = FALSE, vbr = FALSE;
	int disp_count = 0, temp_time;
	char *filename = arg;
	xing_header_t xing_header;

	/* This is used by fileinfo on http streams */
	mpg123_bitrate = 0;

	mpg123_pcm_sample = g_malloc0(32768);
	mpg123_pcm_point = 0;
	mpg123_filename = filename;

	mpg123_read_frame_init();

	mpg123_open_stream(filename, -1);
	if (mpg123_info->eof || !mpg123_read_frame(&fr))
		mpg123_info->eof = TRUE;
	if (!mpg123_info->eof && mpg123_info->going)
	{
		if (mpg123_cfg.channels == 2)
			fr.single = -1;
		else
			fr.single = 3;

		fr.down_sample = mpg123_cfg.downsample;
		fr.down_sample_sblimit = SBLIMIT >> mpg123_cfg.downsample;
		set_synth_functions(&fr);
		mpg123_init_layer3(fr.down_sample_sblimit);

		mpg123_info->tpf = mpg123_compute_tpf(&fr);
		if (strncasecmp(filename, "http://", 7))
		{
			if (mpg123_stream_check_for_xing_header(&fr, &xing_header))
			{
				mpg123_info->num_frames = xing_header.frames;
				have_xing_header = TRUE;
				mpg123_read_frame(&fr);
			}
		}

		for (;;)
		{
			memcpy(&temp_fr, &fr, sizeof(struct frame));
			if (!mpg123_read_frame(&temp_fr))
			{
				mpg123_info->eof = TRUE;
				break;
			}
			if (fr.lay != temp_fr.lay ||
			    fr.sampling_frequency != temp_fr.sampling_frequency ||
	        	    fr.stereo != temp_fr.stereo || fr.lsf != temp_fr.lsf)
				memcpy(&fr,&temp_fr,sizeof(struct frame));
			else
				break;
		}

		if (!have_xing_header && strncasecmp(filename, "http://", 7))
			mpg123_info->num_frames = mpg123_calc_numframes(&fr);

		memcpy(&fr, &temp_fr, sizeof(struct frame));
		mpg123_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
		disp_bitrate = mpg123_bitrate;
		mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
		mpg123_stereo = fr.stereo;
		mpg123_layer = fr.lay;
		mpg123_lsf = fr.lsf;
		mpg123_mpeg25 = fr.mpeg25;
		mpg123_mode = fr.mode;

		if (strncasecmp(filename, "http://", 7))
		{
			mpg123_length =
				mpg123_info->num_frames * mpg123_info->tpf * 1000;
			if (!mpg123_title)
				mpg123_title = get_song_title(NULL,filename);
		}
		else
		{
			if (!mpg123_title)
				mpg123_title = mpg123_http_get_title(filename);
			mpg123_length = -1;
		}
		mpg123_ip.set_info(mpg123_title, mpg123_length,
				   mpg123_bitrate * 1000,
				   mpg123_freqs[fr.sampling_frequency],
				   fr.stereo);
		output_opened = TRUE;
		if (!open_output())
		{
			audio_error = TRUE;
			mpg123_info->eof = TRUE;
		}
		else
			play_frame(&fr);
	}

	mpg123_info->first_frame = FALSE;
	while (mpg123_info->going)
	{
		if (mpg123_info->jump_to_time != -1)
		{
			void *xp = NULL;
			if (have_xing_header)
				xp = &xing_header;
			if (mpg123_seek(&fr, xp, vbr,
					mpg123_info->jump_to_time) > -1)
			{
				mpg123_ip.output->flush(mpg123_info->jump_to_time * 1000);
				mpg123_info->eof = FALSE;
			}
			mpg123_info->jump_to_time = -1;
		}
		if (!mpg123_info->eof)
		{
			if (mpg123_read_frame(&fr) != 0)
			{
				if(fr.lay != mpg123_layer || fr.lsf != mpg123_lsf)
				{
					memcpy(&temp_fr, &fr, sizeof(struct frame));
					if(mpg123_read_frame(&temp_fr) != 0)
					{
						if(fr.lay == temp_fr.lay && fr.lsf == temp_fr.lsf)
						{
							mpg123_layer = fr.lay;
							mpg123_lsf = fr.lsf;
							memcpy(&fr,&temp_fr,sizeof(struct frame));
						}
						else
						{
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							skip_frames = 2;
							mpg123_info->output_audio = FALSE;
							continue;
						}
						
					}
				}
				if(mpg123_freqs[fr.sampling_frequency] != mpg123_frequency || mpg123_stereo != fr.stereo)
				{
					memcpy(&temp_fr,&fr,sizeof(struct frame));
					if(mpg123_read_frame(&temp_fr) != 0)
					{
						if(fr.sampling_frequency == temp_fr.sampling_frequency && temp_fr.stereo == fr.stereo)
						{
							mpg123_ip.output->buffer_free();
							mpg123_ip.output->buffer_free();
							while(mpg123_ip.output->buffer_playing() && mpg123_info->going && mpg123_info->jump_to_time == -1)
								xmms_usleep(20000);
							if(!mpg123_info->going)
								break;
							temp_time = mpg123_ip.output->output_time();
							mpg123_ip.output->close_audio();
							mpg123_frequency = mpg123_freqs[fr.sampling_frequency];
							mpg123_stereo = fr.stereo;
							if (!mpg123_ip.output->open_audio(mpg123_cfg.resolution == 16 ? FMT_S16_NE : FMT_U8,
								mpg123_freqs[fr.sampling_frequency] >> mpg123_cfg.downsample,
				  				mpg123_cfg.channels == 2 ? fr.stereo : 1))
							{
								audio_error = TRUE;
								mpg123_info->eof = TRUE;
							}
							mpg123_ip.output->flush(temp_time);
							mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_frequency, mpg123_stereo);
							memcpy(&fr,&temp_fr,sizeof(struct frame));
						}
						else
						{
							memcpy(&fr,&temp_fr,sizeof(struct frame));
							skip_frames = 2;
							mpg123_info->output_audio = FALSE;
							continue;
						}
					}					
				}
				
				if (tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index] != mpg123_bitrate)
					mpg123_bitrate = tabsel_123[fr.lsf][fr.lay - 1][fr.bitrate_index];
				
				if (!disp_count)
				{
					disp_count = 20;
					if (mpg123_bitrate != disp_bitrate)
					{
						/* FIXME networks streams */
						disp_bitrate = mpg123_bitrate;
						if(!have_xing_header && strncasecmp(filename,"http://",7))
						{
							double rel = mpg123_relative_pos();
							if (rel)
							{
								mpg123_length = mpg123_ip.output->written_time() / rel;
								vbr = TRUE;
							}

							if (rel == 0 || !(mpg123_length > 0))
							{
								mpg123_info->num_frames = mpg123_calc_numframes(&fr);
								mpg123_info->tpf = mpg123_compute_tpf(&fr);
								mpg123_length = mpg123_info->num_frames * mpg123_info->tpf * 1000;
							}


						}
						mpg123_ip.set_info(mpg123_title, mpg123_length, mpg123_bitrate * 1000, mpg123_frequency, mpg123_stereo);
					}
				}
				else
					disp_count--;
				play_frame(&fr);
			}
			else
			{
				mpg123_ip.output->buffer_free();
				mpg123_ip.output->buffer_free();
				mpg123_info->eof = TRUE;
				xmms_usleep(10000);
			}
		}
		else
		{
			xmms_usleep(10000);
		}
	}
	g_free(mpg123_title);
	mpg123_title = NULL;
	mpg123_stream_close();
	if (output_opened && !audio_error)
		mpg123_ip.output->close_audio();
	g_free(mpg123_pcm_sample);
	mpg123_filename = NULL;
	g_free(filename);
	pthread_exit(NULL);
}

static void play_file(char *filename)
{
	memset(&fr, 0, sizeof (struct frame));
	memset(&temp_fr, 0, sizeof (struct frame));

	mpg123_info = g_malloc0(sizeof (PlayerInfo));
	mpg123_info->going = 1;
	mpg123_info->first_frame = TRUE;
	mpg123_info->output_audio = TRUE;
	mpg123_info->jump_to_time = -1;
	skip_frames = 0;
	audio_error = FALSE;
	output_opened = FALSE;
	dopause = FALSE;

	pthread_create(&decode_thread, NULL, decode_loop, g_strdup(filename));
}

