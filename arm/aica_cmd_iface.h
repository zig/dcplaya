#ifndef __ARM_AICA_CMD_IFACE_H
#define __ARM_AICA_CMD_IFACE_H

/* $Id: aica_cmd_iface.h,v 1.1.1.1 2002-08-26 14:15:01 ben Exp $ */

#define  _BEN_AICA 1

#ifndef __ARCH_TYPES_H
typedef unsigned long uint32;
#endif

/* Make this 8 dwords long for one aica bus queue */
typedef struct {
	uint32		cmd;		  /* Command ID		    */
	uint32		pos;		  /* Sample position  */
	uint32		length;		/* Sample length	  */
	uint32		freq;		  /* Frequency		    */
	uint32		vol;		  /* Volume 0-255		  */
	uint32		pan;		  /* Pan 0-255		    */
	uint32		dummy[2];	/* Pad values		    */
} aica_channel;

/* Command values */
#define AICA_CMD_KICK		  0x80000000
#define AICA_CMD_NONE		  0
#define AICA_CMD_START	  1
#define AICA_CMD_STOP		  2
#define AICA_CMD_VOL		  3
#define AICA_CMD_FRQ		  4
#define AICA_CMD_MONO	    5
#define AICA_CMD_STEREO	  (AICA_CMD_MONO+1)
#define AICA_CMD_INVERSE	(AICA_CMD_MONO+2)
#define AICA_CMD_PARM     8

/* Streaming parameters */
#define SPU_STREAM_MAX  32768
#define SPU_SPL1_ADDR   0x11000
#define SPU_TOP_RAM     (SPU_SPL1_ADDR+SPU_STREAM_MAX*4)

#endif	/* __ARM_AICA_CMD_IFACE_H */
