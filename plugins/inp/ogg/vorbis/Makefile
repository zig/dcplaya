# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Dan Potter
#
# $Id: Makefile,v 1.4 2002-09-12 17:57:31 ben Exp $

LIBRARY=yes
OBJS = mdct.o smallft.o block.o envelope.o window.o lsp.o lpc.o analysis.o synthesis.o psy.o info.o time0.o floor1.o floor0.o res0.o mapping0.o registry.o codebook.o sharedbook.o lookup.o bitbuffer.o 
OPTIMIZE =  $(OPTIMIZE_FOR_SPEED)
KOS_INCS += -I. -I../include
KOS_LOCAL_CFLAGS += -DLITTLE_ENDIAN=1

include ../../../../Makefile.inc

