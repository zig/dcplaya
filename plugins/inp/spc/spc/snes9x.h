/*
 * Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 *
 * (c) Copyright 1996 - 2000 Gary Henderson (gary@daniver.demon.co.uk) and
 *                           Jerremy Koot (jkoot@snes9x.com)
 *
 * Super FX C emulator code 
 * (c) Copyright 1997 - 1999 Ivar (Ivar@snes9x.com) and
 *                           Gary Henderson.
 * Super FX assembler emulator code (c) Copyright 1998 zsKnight and _Demo_.
 *
 * DSP1 emulator code (c) Copyright 1998 Ivar, _Demo_ and Gary Henderson.
 * DOS port code contains the works of other authors. See headers in
 * individual files.
 *
 * Snes9x homepage: www.snes9x.com
 *
 * Permission to use, copy, modify and distribute Snes9x in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Snes9x is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Snes9x or software derived from Snes9x.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so everyone can benefit from the modifications
 * in future versions.
 *
 * Super NES and Super Nintendo Entertainment System are trademarks of
 * Nintendo Co., Limited and its subsidiary companies.
 */

#ifndef _SNES9X_H_
#define _SNES9X_H_

#define VERSION "1.29"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
}

#ifdef _WIN32
#include "..\Snes9X.h"
#include "..\zlib\zlib.h"
#endif

#include "port.h"
/* #include "65c816.h" */
/* #include "messages.h" */


#define SNES_MAX_NTSC_VCOUNTER  262
#define SNES_MAX_PAL_VCOUNTER   312
#define SNES_HCOUNTER_MAX	341
#define SPC700_TO_65C816_RATIO	2
#define AUTO_FRAMERATE		200
#define SNES_SCANLINE_TIME (63.49e-6)

#ifdef VAR_CYCLES
#define SNES_CYCLES_PER_SCANLINE ((uint16) (63.5e-6 / (1 / 3580000.0)) * 6)
#else
#define SNES_CYCLES_PER_SCANLINE ((uint16) (63.5e-6 / (1 / 3580000.0)))
#endif

#define SCANLINE_FREQUENCY (15748.0315)

#ifdef VAR_CYCLES
#define ONE_CYCLE 6
#define TWO_CYCLES 12
#else
#define ONE_CYCLE 1
#define TWO_CYCLES 2
#endif

struct SCPUState{
    uint32  Flags;
    bool8   BranchSkip;
    bool8   NMIActive;
    bool8   IRQActive;
    bool8   WaitingForInterrupt;
    bool8   InDMA;
    uint8   WhichEvent;
    uint8   *PC;
    uint8   *PCBase;
    uint8   *PCAtOpcodeStart;
    uint8   *WaitAddress;
    uint32  WaitCounter;
    long   Cycles;
    long   NextEvent;
    long   V_Counter;
    long   MemSpeed;
    long   MemSpeedx2;
    long   FastROMSpeed;
    uint32 AutoSaveTimer;
    bool8  SRAMModified;
};

#define HBLANK_START_EVENT 0
#define HBLANK_END_EVENT 1
#define HTIMER_EVENT 2
#define NO_EVENT 3

struct SSettings{
    // CPU options
    bool8  APUEnabled;
    bool8  Shutdown;
    uint8  SoundSkipMethod;

    long   H_Max;
    long   HBlankStart;
    uint32 SPCTo65c816Ratio;
    bool8  DisableIRQ;
    uint8  Paused;

    // ROM timing options (see also H_Max above)
    bool8  ForcePAL;
    bool8  ForceNTSC;
    bool8  PAL;
    uint32 FrameTimePAL;
    uint32 FrameTimeNTSC;
    uint32 FrameTime;
    uint32 SkipFrames;

    // Sound options
  //    bool8  TraceSoundDSP;
    uint8  SoundPlaybackRate;
    bool8  Stereo;
    int    SoundBufferSize;
    bool8  SoundEnvelopeHeightReading;
    bool8  DisableSoundEcho;
  //    bool8  DisableSampleCaching;
    bool8  DisableMasterVolume;
    bool8  SoundSync;
    bool8  InterpolatedSound;
  //    bool8  ThreadSound;
    
};

START_EXTERN_C

extern struct SSettings Settings;
extern struct SCPUState CPU;
extern char String [513];

void S9xExit ();
void S9xMessage (int type, int number, const char *message);

END_EXTERN_C

#endif
