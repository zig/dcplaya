typedef unsigned char u8_t;
typedef signed char s8_t;
typedef unsigned int u32_t;
typedef unsigned short u16_t;


//#define CONFIG_AC3 1
#define ARCH_SH4 1
#define TUNECPU generic
#define EMULATE_INTTYPES 1
#define EMULATE_FAST_INT 1
/*#define CONFIG_ENCODERS 1*/
#define CONFIG_DECODERS 1
#define CONFIG_MPEGAUDIO_HP 1
/* #define CONFIG_VIDEO4LINUX 1 */
/* #define CONFIG_DV1394 1 */
/* #define CONFIG_AUDIO_OSS 1 */
/* #define CONFIG_NETWORK 1 */
#undef  HAVE_MALLOC_H
#undef  HAVE_MEMALIGN
#define SIMPLE_IDCT 1
#define CONFIG_RISKY 1
#define restrict __restrict__

#include "avcodec.h"
#include "avformat.h"
#undef fifo_init
#undef fifo_free
#undef fifo_size
#undef fifo_read
#undef fifo_write
