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
/*      NIMP(localtime) */
/*      NIMP(gmtime) */
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
/*      NIMP(put_flush_packet) */
/*      NIMP(put_buffer) */
#endif
     NIMP(lseek)
/*      NIMP(localtime_r) */
     NIMP(lsb2full)

#ifndef AVCODEC_CVS
     NIMP(get_bit_count)
     NIMP(align_put_bits)
#endif

         NIMP(sinh)
         NIMP(cosh)
         NIMP(tanh)


void dummyf()
{
}



#if 0

/* looking for division by zero ... */

#include "lef.h"
static int symb(uint32 PC)
{
  symbol_t *symb = lef_closest_symbol(PC);
  if (symb) {
    printf("STACK 0x%8x (%s + 0x%x)\n", 
	   PC, symb->name, PC - ((uint32)symb->addr));
  }
  return 0;
}
static int stack_cb(void *ctx, void *dummy)
{
  uint32 PC = _Unwind_GetIP(ctx);
  symb(PC);
  return 0;
}

int __udivsi3_i4i(unsigned int a, int b)
{
    if (!b) {
        printf("udiv %d %d\n");
        symb(__builtin_return_address(0));
        //_Unwind_Backtrace(stack_cb, NULL);
    }
  return (float) a / (float) b;
}

int __sdivsi3_i4i(int a, int b)
{
    if (!b) {
        printf("sdiv %d %d\n");
        symb(__builtin_return_address(0));
        //_Unwind_Backtrace(stack_cb, NULL);
    }
  return (float) a / (float) b;
}

#endif
