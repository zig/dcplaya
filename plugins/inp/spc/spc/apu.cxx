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

#ifdef __DJGPP
#include <allegro.h>
#undef TRUE
#endif

#include "snes9x.h"
#include "spc700.h"
#include "apu.h"
#include "soundux.h"
// #include "cpuexec.h"

#ifdef DEBUGGER
FILE *apu_trace = NULL;

static char *S9xMnemonics [256] = {
"NOP", "TCALL0", "SET0 $%02X", "BBS0 $%02X,$%04X",
"OR A,$%02X", "OR A,$%04X", "OR A,(X)", "OR A,($%02X+X)",
"OR A,#$%02X", "OR $%02X,$%02X", "OR1 C,$%04X,%d", "ASL $%02X",
"ASL $%04X", "PUSH PSW", "TSET1 $%04X", "BRK",
"BPL $%04X", "TCALL1", "CLR0 $%02X", "BBC0 $%02X,$%04X",
"OR A,$%02X+X", "OR A,$%04X+X", "OR A,$%04X+Y", "OR A,($%02X)+Y",
"OR $%02X,#$%02X", "OR (X),(Y)", "DECW $%02X", "ASL $%02X+X",
"ASL A", "DEC X", "CMP X,$%04X", "JMP ($%04X+X)",
"CLRP", "TCALL2", "SET1 $%02X", "BBS1 $%02X,$%04X",
"AND A,$%02X", "AND A,$%04X", "AND A,(X)", "AND A,($%02X+X)",
"AND A,$%02X", "AND $%02X,$%02X", "OR1 C,$%04X, not %d", "ROL $%02X",
"ROL $%04X", "PUSH A", "CBNE $%02X,$%04X", "BRA $%04X",
"BMI $%04X", "TCALL3", "CLR1 $%02X", "BBC1 $%02X,$%04X",
"AND A,$%02X+X", "AND A,$%04X+X", "AND A,$%04X+Y", "AND A,($%02X)+Y",
"AND $%02X,#$%02X", "AND (X),(Y)", "INCW $%02X", "ROL $%02X+X",
"ROL A", "INC X", "CMP X,$%02X", "CALL $%04X",
"SETP", "TCALL4", "SET2 $%02X", "BBS2 $%02X,$%04X",
"EOR A,$%02X", "EOR A,$%04X", "EOR A,(X)", "EOR A,($%02X+X)",
"EOR A,#$%02X", "EOR $%02X,$%02X", "AND1 C,$%04X,%d", "LSR $%02X",
"LSR $%04X", "PUSH X", "TCLR1 $%04X", "PCALL $%02X",
"BVC $%04X", "TCALL5", "CLR2 $%02X", "BBC2 $%02X,$%04X",
"EOR A,$%02X+X", "EOR A,$%04X+X", "EOR A,$%04X+Y", "EOR A,($%02X)+Y",
"EOR $%02X,#$%02X", "EOR (X),(Y)", "CMPW YA,$%02X", "LSR $%02X+X",
"LSR A", "MOV X,A", "CMP Y,$%04X", "JMP $%04X",
"CLRC", "TCALL6", "SET3 $%02X", "BBS3 $%02X,$%04X",
"CMP A,$%02X", "CMP A,$%04X", "CMP A,(X)", "CMP A,($%02X+X)",
"CMP A,#$%02X", "CMP $%02X,$%02X", "AND1 C, $%04X, not %d", "ROR $%02X",
"ROR $%04X", "PUSH Y", "DBNZ $%02X,$%04X", "RET",
"BVS $%04X", "TCALL7", "CLR3 $%02X", "BBC3 $%02X,$%04X",
"CMP A,$%02X+X", "CMP A,$%04X+X", "CMP A,$%04X+Y", "CMP A,($%02X)+Y",
"CMP $%02X,#$%02X", "CMP (X),(Y)", "ADDW YA,$%02X", "ROR $%02X+X",
"ROR A", "MOV A,X", "CMP Y,$%02X", "RETI",
"SETC", "TCALL8", "SET4 $%02X", "BBS4 $%02X,$%04X",
"ADC A,$%02X", "ADC A,$%04X", "ADC A,(X)", "ADC A,($%02X+X)",
"ADC A,#$%02X", "ADC $%02X,$%02X", "EOR1 C,%04,%d", "DEC $%02X",
"DEC $%04X", "MOV Y,#$%02X", "POP PSW", "MOV $%02X,#$%02X",
"BCC $%04X", "TCALL9", "CLR4 $%02X", "BBC4 $%02X,$%04X",
"ADC A,$%02X+X", "ADC A,$%04X+X", "ADC A,$%04X+Y", "ADC A,($%02X)+Y",
"ADC $%02X,#$%02X", "ADC (X),(Y)", "SUBW YA,$%02X", "DEC $%02X+X",
"DEC A", "MOV X,SP", "DIV YA,X", "XCN A",
"EI", "TCALL10", "SET5 $%02X", "BBS5 $%02X,$%04X",
"SBC A,$%02X", "SBC A,$%04X", "SBC A,(X)", "SBC A,($%02X+X)",
"SBC A,#$%02X", "SBC $%02X,$%02X", "MOV1 C,$%04X,%d", "INC $%02X",
"INC $%04X", "CMP Y,#$%02X", "POP A", "MOV (X)+,A",
"BCS $%04X", "TCALL11", "CLR5 $%02X", "BBC5 $%02X,$%04X",
"SBC A,$%02X+X", "SBC A,$%04X+X", "SBC A,$%04X+Y", "SBC A,($%02X)+Y",
"SBC $%02X,#$%02X", "SBC (X),(Y)", "MOVW YA,$%02X", "INC $%02X+X",
"INC A", "MOV SP,X", "DAS", "MOV A,(X)+",
"DI", "TCALL12", "SET6 $%02X", "BBS6 $%02X,$%04X",
"MOV $%02X,A", "MOV $%04X,A", "MOV (X),A", "MOV ($%02X+X),A",
"CMP X,#$%02X", "MOV $%04X,X", "MOV1 $%04X,%d,C", "MOV $%02X,Y",
"MOV $%04X,Y", "MOV X,#$%02X", "POP X", "MUL YA",
"BNE $%04X", "TCALL13", "CLR6 $%02X", "BBC6 $%02X,$%04X",
"MOV $%02X+X,A", "MOV $%04X+X,A", "MOV $%04X+Y,A", "MOV ($%02X)+Y,A",
"MOV $%02X,X", "MOV $%02X+Y,X", "MOVW $%02X,YA", "MOV $%02X+X,Y",
"DEC Y", "MOV A,Y", "CBNE $%02X+X,$%04X", "DAA",
"CLRV", "TCALL14", "SET7 $%02X", "BBS7 $%02X,$%04X",
"MOV A,$%02X", "MOV A,$%04X", "MOV A,(X)", "MOV A,($%02X+X)",
"MOV A,#$%02X", "MOV X,$%04X", "NOT1 $%04X,%d", "MOV Y,$%02X",
"MOV Y,$%04X", "NOTC", "POP Y", "SLEEP",
"BEQ $%04X", "TCALL15", "CLR7 $%02X", "BBC7 $%02X,$%04X",
"MOV A,$%02X+X", "MOV A,$%04X+X", "MOV A,$%04X+Y", "MOV A,($%02X)+Y",
"MOV X,$%02X", "MOV X,$%02X+Y", "MOV $%02X,$%02X", "MOV Y,$%02X+X",
"INC Y", "MOV Y,A", "DBNZ Y,$%04X", "STOP"
};

#undef ABS

#define DP 0
#define ABS 1
#define IM 2
#define DP2DP 3
#define DPIM 4
#define DPREL 5
#define ABSBIT 6
#define REL 7

static uint8 Modes [256] = {
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, ABS, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, ABS, ABS,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, DPREL, REL,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, DP, ABS,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, ABS, DP,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, ABS, ABS,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, IM, DPREL, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, DP, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, DP, IM, DPIM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, IM, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, DP2DP, ABSBIT, DP,
    ABS, DP, IM, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DPIM, IM, DP, DP,
    IM, IM, IM, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, ABS, ABSBIT, DP,
    ABS, DP, IM, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DP, DP, DP, DP,
    IM, IM, DPREL, IM,
    IM, IM, DP, DPREL,
    DP, ABS, IM, DP,
    DP, ABS, ABS, DP,
    ABS, IM, IM, IM,
    REL, IM, DP, DPREL,
    DP, ABS, ABS, DP,
    DP, DP, DP2DP, DP,
    IM, IM, REL, IM
};

static uint8 ModesToBytes [] = {
    2, 3, 1, 3, 3, 3, 3, 2
};

#endif

extern int NoiseFreq [32];

#ifdef DEBUGGER
static FILE *SoundTracing = NULL;

void S9xOpenCloseSoundTracingFile (bool8 open)
{
    if (open && !SoundTracing)
	SoundTracing = fopen ("sound_trace.log", "w");
    else
    if (!open && SoundTracing)
    {
	fclose (SoundTracing);
	SoundTracing = NULL;
    }
}

void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
		    int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0)
{
    fprintf (SoundTracing, s, i1, i2, i3, i4, i5, i6, i7);
}

#endif

#ifdef DEBUGGER
int S9xTraceAPU ()
{
    char buffer [200];
    
    uint8 b = S9xAPUOPrint (buffer, IAPU.PC - IAPU.RAM);
    if (apu_trace == NULL)
	apu_trace = fopen ("apu_trace.log", "wb");

    fprintf (apu_trace, "%s\n", buffer);
    return (b);
}

int S9xAPUOPrint (char *buffer, uint16 Address)
{
    char mnem [100];
    uint8 *p = IAPU.RAM + Address;
    int mode = Modes [*p];
    int bytes = ModesToBytes [mode];
    
    switch (bytes)
    {
    case 1:
	sprintf (buffer, "%04X %02X       ", p - IAPU.RAM, *p);
	break;
    case 2:
	sprintf (buffer, "%04X %02X %02X    ", p - IAPU.RAM, *p,
		 *(p + 1));
	break;
    case 3:
	sprintf (buffer, "%04X %02X %02X %02X ", p - IAPU.RAM, *p,
		 *(p + 1), *(p + 2));
	break;
    }

    switch (mode)
    {
    case DP:
	sprintf (mnem, S9xMnemonics [*p], *(p + 1));
	break;
    case ABS:
	sprintf (mnem, S9xMnemonics [*p], *(p + 1) + (*(p + 2) << 8));
	break;
    case IM:
	sprintf (mnem, S9xMnemonics [*p]);
	break;
    case DP2DP:
	sprintf (mnem, S9xMnemonics [*p], *(p + 2), *(p + 1));;
	break;
    case DPIM:
	sprintf (mnem, S9xMnemonics [*p], *(p + 2), *(p + 1));;
	break;
    case DPREL:
	sprintf (mnem, S9xMnemonics [*p], *(p + 1),
		(int) (p + 3 - IAPU.RAM) + (signed char) *(p + 2));
	break;
    case ABSBIT:
	sprintf (mnem, S9xMnemonics [*p], (*(p + 1) + (*(p + 2) << 8)) & 0x1fff,
		*(p + 2) >> 5);
	break;
    case REL:
	sprintf (mnem, S9xMnemonics [*p],
		(int) (p + 2 - IAPU.RAM) + (signed char) *(p + 1));
	break;
    }

    sprintf (buffer, "%s %-20s A:%02X X:%02X Y:%02X S:%02X P:%c%c%c%c%c%c%c%c",
	     buffer, mnem,
	     APURegisters.YA.B.A, APURegisters.X, APURegisters.YA.B.Y,
	     APURegisters.S,
	     APUCheckNegative () ? 'N' : 'n',
	     APUCheckOverflow () ? 'V' : 'v',
	     APUCheckDirectPage () ? 'P' : 'p',
	     APUCheckBreak () ? 'B' : 'b',
	     APUCheckHalfCarry () ? 'H' : 'h',
	     APUCheckInterrupt () ? 'I' : 'i',
	     APUCheckZero () ? 'Z' : 'z',
	     APUCheckCarry () ? 'C' : 'c');
		
    return (bytes);
}

const char *as_binary (uint8 data)
{
    static char buf [9];

    for (int i = 7; i >= 0; i--)
	buf [7 - i] = ((data & (1 << i)) != 0) + '0';

    buf [8] = 0;
    return (buf);
}

void S9xPrintAPUState ()
{
    printf ("Master volume left: %d, right: %d\n",
	    SoundData.master_volume_left, SoundData.master_volume_right);
    printf ("Echo: %s %s, Delay: %d Feedback: %d Left: %d Right: %d\n",
	    SoundData.echo_write_enabled ? "on" : "off",
	    as_binary (SoundData.echo_enable),
	    SoundData.echo_buffer_size >> 9,
	    SoundData.echo_feedback, SoundData.echo_volume_left,
	    SoundData.echo_volume_right);

    printf ("Noise: %s, Frequency: %d\n", as_binary (APU.DSP [APU_NON]),
	    NoiseFreq [APU.DSP [APU_FLG] & 0x1f]);
    extern int FilterTaps [8];

    printf ("Filter: ");
    for (int i = 0; i < 8; i++)
	printf ("%03d, ", FilterTaps [i]);
    printf ("\n");
    for (int J = 0; J < 8; J++)
    {
	register Channel *ch = &SoundData.channels[J];

	printf ("%d: ", J);
	if (ch->state == SOUND_SILENT)
	{
	    printf ("off\n");
	}
	else
	if (!(so.sound_switch & (1 << J)))
	    printf ("muted by user using channel on/off toggle\n");
	else
	{
	    int freq = ch->hertz;
	    if (APU.DSP [APU_NON] & (1 << J)) //ch->type == SOUND_NOISE)
	    {
		freq = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
		printf ("noise, ");
	    }
	    else
		printf ("sample %d, ", APU.DSP [APU_SRCN + J * 0x10]);

	    printf ("freq: %d", freq);
	    if (J > 0 && (SoundData.pitch_mod & (1 << J)) &&
		ch->type != SOUND_NOISE)
	    {
		printf ("(mod), ");
	    }
	    else
		printf (", ");

	    printf ("left: %d, right: %d, ",
		    ch->volume_left, ch->volume_right);

	    static char* envelope [] = 
	    {
		"silent", "attack", "decay", "sustain", "release", "gain",
		"inc_lin", "inc_bent", "dec_lin", "dec_exp"
	    };
	    printf ("%s envx: %d, target: %d, %d", ch->state > 9 ? "???" : envelope [ch->state],
		    ch->envx, ch->envx_target, ch->erate);
	    printf ("\n");
	}
    }
}
#endif

bool8 S9xInitAPU ()
{
    IAPU.RAM = (uint8 *) malloc(0x10000);
    IAPU.ShadowRAM = (uint8 *) malloc(0x10000);
    IAPU.CachedSamples = (uint8 *) malloc(0x40000);
    
    if (!IAPU.RAM || !IAPU.ShadowRAM || !IAPU.CachedSamples)
	return (FALSE);

    return (TRUE);
}

void S9xDeinitAPU ()
{
  free(IAPU.RAM);
  free(IAPU.ShadowRAM);
  free(IAPU.CachedSamples);
}

EXTERN_C uint8 APUROM [64];

void S9xResetAPU ()
{
    memset (IAPU.RAM, 0xff, 0x10000);
    ZeroMemory (IAPU.ShadowRAM, 0x10000);
    ZeroMemory (IAPU.CachedSamples, 0x40000);
    ZeroMemory (APU.OutPorts, 4);
    IAPU.DirectPage = IAPU.RAM;
    memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
    memmove (APU.ExtraRAM, APUROM, sizeof (APUROM));
    IAPU.PC = IAPU.RAM + IAPU.RAM [0xfffe] + (IAPU.RAM [0xffff] << 8);
    APU.Cycles = 0;
    APURegisters.YA.W = 0;
    APURegisters.X = 0;
    APURegisters.S = 0xff;
    APURegisters.P = 0;
    S9xAPUUnpackStatus ();
    APURegisters.PC = 0;
    IAPU.APUExecuting = Settings.APUEnabled;
#ifdef SPC700_SHUTDOWN
    IAPU.WaitAddress1 = NULL;
    IAPU.WaitAddress2 = NULL;
    IAPU.WaitCounter = 0;
#endif
    APU.ShowROM = TRUE;
    IAPU.RAM [0xf1] = 0x80;

    int i;

    for (i = 0; i < 3; i++)
    {
	APU.TimerEnabled [i] = FALSE;
	APU.TimerValueWritten [i] = 0;
	APU.TimerTarget [i] = 0;
	APU.Timer [i] = 0;
    }
    for (i = 0; i < 0x80; i++)
	APU.DSP [i] = 0;

    IAPU.TwoCycles = IAPU.OneCycle * 2;

    for (i = 0; i < 256; i++)
	S9xAPUCycles [i] = S9xAPUCycleLengths [i] * IAPU.OneCycle;

    APU.DSP [APU_ENDX] = 0;
    APU.DSP [APU_KOFF] = 0;
    APU.DSP [APU_KON] = 0;
    APU.DSP [APU_FLG] = APU_MUTE | APU_ECHO_DISABLED;
    APU.KeyedChannels = 0;

    S9xResetSound (TRUE);
    S9xSetEchoEnable (0);
}

void S9xSetAPUDSP (uint8 byte)
{
    uint8 reg = IAPU.RAM [0xf2];
    int i;

    switch (reg)
    {
    case APU_FLG:
	if (byte & APU_SOFT_RESET)
	{
	    APU.DSP [reg] = APU_MUTE | APU_ECHO_DISABLED | (byte & 0x1f);
	    APU.DSP [APU_ENDX] = 0;
	    APU.DSP [APU_KOFF] = 0;
	    APU.DSP [APU_KON] = 0;
	    S9xSetEchoWriteEnable (FALSE);
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] DSP reset\n", ICPU.Scanline);
#endif
	    // Kill sound
	    S9xResetSound (FALSE);
	}
	else
	{
	    S9xSetEchoWriteEnable (!(byte & APU_ECHO_DISABLED));
	    if (byte & APU_MUTE)
	    {
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
		    S9xTraceSoundDSP ("[%d] Mute sound\n", ICPU.Scanline);
#endif
		S9xSetSoundMute (TRUE);
	    }
	    else
		S9xSetSoundMute (FALSE);

	    SoundData.noise_hertz = NoiseFreq [byte & 0x1f];
	    for (i = 0; i < 8; i++)
	    {
		if (SoundData.channels [i].type == SOUND_NOISE)
		    S9xSetSoundFrequency (i, SoundData.noise_hertz);
	    }
	}
	break;
    case APU_NON:
	if (byte != APU.DSP [APU_NON])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Noise:", ICPU.Scanline);
#endif
	    uint8 mask = 1;
	    int c;
	    for (c = 0; c < 8; c++, mask <<= 1)
	    {
		int type;
		if (byte & mask)
		{
		    type = SOUND_NOISE;
#ifdef DEBUGGER
		    if (Settings.TraceSoundDSP)
		    {
			if (APU.DSP [reg] & mask)
			    S9xTraceSoundDSP ("%d,", c);
			else
			    S9xTraceSoundDSP ("%d(on),", c);
		    }
#endif
		}
		else
		{
		    type = SOUND_SAMPLE;
#ifdef DEBUGGER
		    if (Settings.TraceSoundDSP)
		    {
			if (APU.DSP [reg] & mask)
			    S9xTraceSoundDSP ("%d(off),", c);
		    }
#endif
		}
		S9xSetSoundType (c, type);
	    }
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("\n");
#endif
	}
	break;
    case APU_MVOL_LEFT:
	if (byte != APU.DSP [APU_MVOL_LEFT])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Master volume left:%d\n", 
				  ICPU.Scanline, (signed char) byte);
#endif
		S9xSetMasterVolume ((signed char) byte,
				    (signed char) APU.DSP [APU_MVOL_RIGHT]);
	}
	break;
    case APU_MVOL_RIGHT:
	if (byte != APU.DSP [APU_MVOL_RIGHT])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Master volume right:%d\n",
				  ICPU.Scanline, (signed char) byte);
#endif
		S9xSetMasterVolume ((signed char) APU.DSP [APU_MVOL_LEFT],
				    (signed char) byte);
	}
	break;
    case APU_EVOL_LEFT:
	if (byte != APU.DSP [APU_EVOL_LEFT])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Echo volume left:%d\n",
				  ICPU.Scanline, (signed char) byte);
#endif
		S9xSetEchoVolume ((signed char) byte,
				  (signed char) APU.DSP [APU_EVOL_RIGHT]);
	}
	break;
    case APU_EVOL_RIGHT:
	if (byte != APU.DSP [APU_EVOL_RIGHT])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Echo volume right:%d\n",
				  ICPU.Scanline, (signed char) byte);
#endif
		S9xSetEchoVolume ((signed char) APU.DSP [APU_EVOL_LEFT],
				  (signed char) byte);
	}
	break;
    case APU_ENDX:
#ifdef DEBUGGER
	if (Settings.TraceSoundDSP)
	    S9xTraceSoundDSP ("[%d] S9xReset ENDX\n", ICPU.Scanline);
#endif
	byte = 0;
	break;

    case APU_KOFF:
	if (byte)
	{
	    uint8 mask = 1;
	    int c;
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Key off:", ICPU.Scanline);
#endif
	    for (c = 0; c < 8; c++, mask <<= 1)
	    {
		if ((byte & mask) != 0)
		{
#ifdef DEBUGGER

		    if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("%d,", c);
#endif		    
		    if (APU.KeyedChannels & mask)
		    {
			{
			    APU.KeyedChannels &= ~mask;
			    APU.DSP [APU_KON] &= ~mask;
			    //APU.DSP [APU_KOFF] |= mask;
			    S9xSetSoundKeyOff (c);
			}
		    }
		}
	    }
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("\n");
#endif
	}
	APU.DSP [APU_KOFF] = byte;
	return;
    case APU_KON:
	if (byte)
	{
	    uint8 mask = 1;
	    int c;
#ifdef DEBUGGER

	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] Key on:", ICPU.Scanline);
#endif
	    for (c = 0; c < 8; c++, mask <<= 1)
	    {
		if ((byte & mask) != 0)
		{
#ifdef DEBUGGER
		    if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("%d,", c);
#endif		    
		    // Pac-In-Time requires that channels can be key-on
		    // regardeless of their current state.
		    APU.KeyedChannels |= mask;
		    APU.DSP [APU_KON] |= mask;
		    APU.DSP [APU_KOFF] &= ~mask;
		    APU.DSP [APU_ENDX] &= ~mask;
		    S9xPlaySample (c);
		}
	    }
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("\n");
#endif
	}
	return;
	
    case APU_VOL_LEFT + 0x00:
    case APU_VOL_LEFT + 0x10:
    case APU_VOL_LEFT + 0x20:
    case APU_VOL_LEFT + 0x30:
    case APU_VOL_LEFT + 0x40:
    case APU_VOL_LEFT + 0x50:
    case APU_VOL_LEFT + 0x60:
    case APU_VOL_LEFT + 0x70:
	if (byte != APU.DSP [reg])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] %d volume left: %d\n", 
				  ICPU.Scanline, reg>>4, (signed char) byte);
#endif
		S9xSetSoundVolume (reg >> 4, (signed char) byte,
				   (signed char) APU.DSP [reg + 1]);
	}
	break;
    case APU_VOL_RIGHT + 0x00:
    case APU_VOL_RIGHT + 0x10:
    case APU_VOL_RIGHT + 0x20:
    case APU_VOL_RIGHT + 0x30:
    case APU_VOL_RIGHT + 0x40:
    case APU_VOL_RIGHT + 0x50:
    case APU_VOL_RIGHT + 0x60:
    case APU_VOL_RIGHT + 0x70:
	if (byte != APU.DSP [reg])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] %d volume right: %d\n", 
				  ICPU.Scanline, reg >>4, (signed char) byte);
#endif
		S9xSetSoundVolume (reg >> 4, (signed char) APU.DSP [reg - 1],
				   (signed char) byte);
	}
	break;

    case APU_P_LOW + 0x00:
    case APU_P_LOW + 0x10:
    case APU_P_LOW + 0x20:
    case APU_P_LOW + 0x30:
    case APU_P_LOW + 0x40:
    case APU_P_LOW + 0x50:
    case APU_P_LOW + 0x60:
    case APU_P_LOW + 0x70:
#ifdef DEBUGGER
	if (Settings.TraceSoundDSP)
	    S9xTraceSoundDSP ("[%d] %d freq low: %d\n",
			      ICPU.Scanline, reg>>4, byte);
#endif
	    S9xSetSoundHertz (reg >> 4, ((byte + (APU.DSP [reg + 1] << 8)) & FREQUENCY_MASK) * 8);
	break;

    case APU_P_HIGH + 0x00:
    case APU_P_HIGH + 0x10:
    case APU_P_HIGH + 0x20:
    case APU_P_HIGH + 0x30:
    case APU_P_HIGH + 0x40:
    case APU_P_HIGH + 0x50:
    case APU_P_HIGH + 0x60:
    case APU_P_HIGH + 0x70:
#ifdef DEBUGGER
	if (Settings.TraceSoundDSP)
	    S9xTraceSoundDSP ("[%d] %d freq high: %d\n",
			      ICPU.Scanline, reg>>4, byte);
#endif
	    S9xSetSoundHertz (reg >> 4, 
			      (((byte << 8) + APU.DSP [reg - 1]) & FREQUENCY_MASK) * 8);
	break;

    case APU_SRCN + 0x00:
    case APU_SRCN + 0x10:
    case APU_SRCN + 0x20:
    case APU_SRCN + 0x30:
    case APU_SRCN + 0x40:
    case APU_SRCN + 0x50:
    case APU_SRCN + 0x60:
    case APU_SRCN + 0x70:
	if (byte != APU.DSP [reg])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] %d sample number: %d\n",
				  ICPU.Scanline, reg>>4, byte);
#endif
	    S9xSetSoundSample (reg >> 4, byte);
	}
	break;
	
    case APU_ADSR1 + 0x00:
    case APU_ADSR1 + 0x10:
    case APU_ADSR1 + 0x20:
    case APU_ADSR1 + 0x30:
    case APU_ADSR1 + 0x40:
    case APU_ADSR1 + 0x50:
    case APU_ADSR1 + 0x60:
    case APU_ADSR1 + 0x70:
	if (byte != APU.DSP [reg])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] %d adsr1: %02x\n",
				  ICPU.Scanline, reg>>4, byte);
#endif
	    {
		S9xFixEnvelope (reg >> 4, APU.DSP [reg + 2], byte, 
			     APU.DSP [reg + 1]);
	    }
	}
	break;

    case APU_ADSR2 + 0x00:
    case APU_ADSR2 + 0x10:
    case APU_ADSR2 + 0x20:
    case APU_ADSR2 + 0x30:
    case APU_ADSR2 + 0x40:
    case APU_ADSR2 + 0x50:
    case APU_ADSR2 + 0x60:
    case APU_ADSR2 + 0x70:
	if (byte != APU.DSP [reg])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] %d adsr2: %02x\n", 
				  ICPU.Scanline, reg>>4, byte);
#endif
	    {
		S9xFixEnvelope (reg >> 4, APU.DSP [reg + 1], APU.DSP [reg - 1],
			     byte);
	    }
	}
	break;

    case APU_GAIN + 0x00:
    case APU_GAIN + 0x10:
    case APU_GAIN + 0x20:
    case APU_GAIN + 0x30:
    case APU_GAIN + 0x40:
    case APU_GAIN + 0x50:
    case APU_GAIN + 0x60:
    case APU_GAIN + 0x70:
	if (byte != APU.DSP [reg])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
		S9xTraceSoundDSP ("[%d] %d gain: %02x\n",
				  ICPU.Scanline, reg>>4, byte);
#endif
	    {
		S9xFixEnvelope (reg >> 4, byte, APU.DSP [reg - 2],
			     APU.DSP [reg - 1]);
	    }
	}
	break;

    case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
	break;

    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
	break;
    
    case APU_DIR:
#ifdef DEBUGGER
	if (Settings.TraceSoundDSP)
	    S9xTraceSoundDSP ("[%d] Sample directory to: %02x\n",
			      ICPU.Scanline, byte);
#endif
	break;

    case APU_PMON:
	if (byte != APU.DSP [APU_PMON])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
	    {
		S9xTraceSoundDSP ("[%d] FreqMod:", ICPU.Scanline);
		uint8 mask = 1;
		for (int c = 0; c < 8; c++, mask <<= 1)
		{
		    if (byte & mask)
		    {
			if (APU.DSP [reg] & mask)
			    S9xTraceSoundDSP ("%d", c);
			else
			    S9xTraceSoundDSP ("%d(on),", c);
		    }
		    else
		    {
			if (APU.DSP [reg] & mask)
			    S9xTraceSoundDSP ("%d(off),", c);
		    }
		}
		S9xTraceSoundDSP ("\n");
	    }
#endif
		S9xSetFrequencyModulationEnable (byte);
	}
	break;

    case APU_EON:
	if (byte != APU.DSP [APU_EON])
	{
#ifdef DEBUGGER
	    if (Settings.TraceSoundDSP)
	    {
		S9xTraceSoundDSP ("[%d] Echo:", ICPU.Scanline);
		uint8 mask = 1;
		for (int c = 0; c < 8; c++, mask <<= 1)
		{
		    if (byte & mask)
		    {
			if (APU.DSP [reg] & mask)
			    S9xTraceSoundDSP ("%d", c);
			else
			    S9xTraceSoundDSP ("%d(on),", c);
		    }
		    else
		    {
			if (APU.DSP [reg] & mask)
			    S9xTraceSoundDSP ("%d(off),", c);
		    }
		}
		S9xTraceSoundDSP ("\n");
	    }
#endif
		S9xSetEchoEnable (byte);
	}
	break;

    case APU_EFB:
	S9xSetEchoFeedback ((signed char) byte);
	break;

    case APU_ESA:
	break;

    case APU_EDL:
	S9xSetEchoDelay (byte & 0xf);
	break;

    case APU_C0:
    case APU_C1:
    case APU_C2:
    case APU_C3:
    case APU_C4:
    case APU_C5:
    case APU_C6:
    case APU_C7:
	S9xSetFilterCoefficient (reg >> 4, (signed char) byte);
	break;
    default:
// XXX
//printf ("Write %02x to unknown APU register %02x\n", byte, reg);
	break;
    }

    if (reg < 0x80)
	APU.DSP [reg] = byte;
}

void S9xFixEnvelope (int channel, uint8 gain, uint8 adsr1, uint8 adsr2)
{
    if (adsr1 & 0x80)
    {
	// ADSR mode
	static unsigned long AttackRate [16] = {
	    4100, 2600, 1500, 1000, 640, 380, 260, 160,
	    96, 64, 40, 24, 16, 10, 6, 1
	};
	static unsigned long DecayRate [8] = {
	    1200, 740, 440, 290, 180, 110, 74, 37
	};
	static unsigned long SustainRate [32] = {
	    ~0, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
	    7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
	    1200, 880, 740, 590, 440, 370, 290, 220,
	    180, 150, 110, 92, 74, 55, 37, 18
	};
	// XXX: can DSP be switched to ADSR mode directly from GAIN/INCREASE/
	// DECREASE mode? And if so, what stage of the sequence does it start
	// at?
	if (S9xSetSoundMode (channel, MODE_ADSR))
	{
	    // Hack for ROMs that use a very short attack rate, key on a 
	    // channel, then switch to decay mode. e.g. Final Fantasy II.

	    int attack = AttackRate [adsr1 & 0xf];

	    if (attack == 1 && !Settings.SoundSync)
		attack = 0;

	    S9xSetSoundADSR (channel, attack,
			     DecayRate [(adsr1 >> 4) & 7],
			     SustainRate [adsr2 & 0x1f],
			     (adsr2 >> 5) & 7, 8);
	}
    }
    else
    {
	// Gain mode
	if ((gain & 0x80) == 0)
	{
	    if (S9xSetSoundMode (channel, MODE_GAIN))
	    {
		S9xSetEnvelopeRate (channel, 0, 0, gain & 0x7f);
		S9xSetEnvelopeHeight (channel, gain & 0x7f);
	    }
	}
	else
	{
	    static unsigned long IncreaseRate [32] = {
		~0, 4100, 3100, 2600, 2000, 1500, 1300, 1000,
		770, 640, 510, 380, 320, 260, 190, 160,
		130, 96, 80, 64, 48, 40, 32, 24,
		20, 16, 12, 10, 8, 6, 4, 2
	    };
	    static unsigned long DecreaseRateExp [32] = {
		~0, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
		7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
		1200, 880, 740, 590, 440, 370, 290, 220,
		180, 150, 110, 92, 74, 55, 37, 18
	    };
	    if (gain & 0x40)
	    {
		// Increase mode
		if (S9xSetSoundMode (channel, (gain & 0x20) ?
					  MODE_INCREASE_BENT_LINE :
					  MODE_INCREASE_LINEAR))
		{
		    S9xSetEnvelopeRate (channel, IncreaseRate [gain & 0x1f],
					1, 127);
		}
	    }
	    else
	    {
		uint32 rate = (gain & 0x20) ? DecreaseRateExp [gain & 0x1f] / 2 :
					      IncreaseRate [gain & 0x1f];
		int mode = (gain & 0x20) ? MODE_DECREASE_EXPONENTIAL
					 : MODE_DECREASE_LINEAR;

		if (S9xSetSoundMode (channel, mode))
		    S9xSetEnvelopeRate (channel, rate, -1, 0);
	    }
	}
    }
}

void S9xSetAPUControl (uint8 byte)
{
    if ((byte & 1) != 0 && !APU.TimerEnabled [0])
    {
	APU.Timer [0] = 0;
	IAPU.RAM [0xfd] = 0;
	if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
	    APU.TimerTarget [0] = 0x100;
    }
    if ((byte & 2) != 0 && !APU.TimerEnabled [1])
    {
	APU.Timer [1] = 0;
	IAPU.RAM [0xfe] = 0;
	if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
	    APU.TimerTarget [1] = 0x100;
    }
    if ((byte & 4) != 0 && !APU.TimerEnabled [2])
    {
	APU.Timer [2] = 0;
	IAPU.RAM [0xff] = 0;
	if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
	    APU.TimerTarget [2] = 0x100;
    }
    APU.TimerEnabled [0] = byte & 1;
    APU.TimerEnabled [1] = (byte & 2) >> 1;
    APU.TimerEnabled [2] = (byte & 4) >> 2;

    if (byte & 0x10)
	IAPU.RAM [0xF4] = IAPU.RAM [0xF5] = 0;

    if (byte & 0x20)
	IAPU.RAM [0xF6] = IAPU.RAM [0xF7] = 0;

    if (byte & 0x80)
    {
	if (!APU.ShowROM)
	{
	    memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
	    APU.ShowROM = TRUE;
	}
    }
    else
    {
	if (APU.ShowROM)
	{
	    APU.ShowROM = FALSE;
	    memmove (&IAPU.RAM [0xffc0], APU.ExtraRAM, sizeof (APUROM));
	}
    }
    IAPU.RAM [0xf1] = byte;
}

void S9xSetAPUTimer (uint16 Address, uint8 byte)
{
    IAPU.RAM [Address] = byte;

    switch (Address)
    {
    case 0xfa:
//	if (!APU.TimerEnabled [0] || !APU.TimerValueWritten [0])
	{
	    if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
		APU.TimerTarget [0] = 0x100;
	    APU.TimerValueWritten [0] = TRUE;
	}
	break;
    case 0xfb:
//	if (!APU.TimerEnabled [1] || !APU.TimerValueWritten [1])
	{
	    if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
		APU.TimerTarget [1] = 0x100;
	    APU.TimerValueWritten [1] = TRUE;
	}
	break;
    case 0xfc:
//	if (!APU.TimerEnabled [2] || !APU.TimerValueWritten [2])
	{
	    if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
		APU.TimerTarget [2] = 0x100;
	    APU.TimerValueWritten [2] = TRUE;
	}
	break;
    }
}

uint8 S9xGetAPUDSP ()
{
    uint8 reg = IAPU.RAM [0xf2] & 0x7f;
    uint8 byte = APU.DSP [reg];

    switch (reg)
    {
    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
	return (byte);

    case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
	return ((uint8) S9xGetEnvelopeHeight (reg >> 4));

    case APU_ENDX:
	APU.DSP [APU_ENDX] = 0;
	break;
    default:
	break;
    }
    return (byte);
}

