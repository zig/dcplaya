#include <stdio.h>

#define AVCODEC_CVS

#undef fprintf
#define NIMP(name) int name() { printf("ffmpeg : Missing "#name" !\n"); return 0; }
//NIMP(sscanf)
     //NIMP(snprintf)
//NIMP(fprintf)
     //     NIMP(vfprintf)
     //     NIMP(rnd_avg2)
     //     NIMP(no_rnd_avg2)
     //     NIMP(strtol)
/*      NIMP(time) */
     NIMP(localtime)
     NIMP(gmtime)
     NIMP(atof)
/*      NIMP(mktime) */
     //     NIMP(__ashldi3)
/*      NIMP(__ashrdi3) */
/*      NIMP(__lshldi3) */
/*      NIMP(__lshrdi3) */
     //     NIMP(__floatdisf)
     //     NIMP(__fixsfdi)
     NIMP(video_grab_init)
     NIMP(audio_init)
     NIMP(dv1394_init)
#ifdef AVCODEC_CVS
     NIMP(put_flush_packet)
     NIMP(put_buffer)
#endif
     NIMP(lseek)
/*      NIMP(localtime_r) */
     NIMP(lsb2full)

#ifndef AVCODEC_CVS
     NIMP(get_bit_count)
     NIMP(align_put_bits)
#endif


void dummyf()
{
}
