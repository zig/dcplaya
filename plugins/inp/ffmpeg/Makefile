#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.4 2004-07-13 09:24:19 vincentp Exp $
#

LEZ=yes

FFMPEG_DIR		:= ffmpeg
FFMPEG_INCS 	:= .
FFMPEG_LIB		:= $(FFMPEG_DIR)/libavformat.a $(FFMPEG_DIR)/libavcodec.a

LEF_TARGET_EXTRA_DEP := $(FFMPEG_LIB) sha123/libsha123.a istream/libistream.a
#LEF_EXTRA := -L$(FFMPEG_DIR) -Lsha123 -Listream -lavformat -lavcodec -lsha123 -listream
LEF_EXTRA = -L$(KOS_BASE)/../kos-ports/lib/dreamcast -loggvorbisplay

KOS_LOCAL_CFLAGS += -DHOST_LITTLE_ENDIAN -DDCPLAYA=1 -falign-functions=32 -falign-loops=32 -falign-jumps=32

KOS_LOCAL_INCS += -I. -I../../../include -I../../../libs\
 $(foreach i,$(FFMPEG_INCS),-I$(FFMPEG_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)


SUBDIRS = sha123 istream


include ../../../Makefile.inc


# SH-4 compile tools
KOS_CC=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-gcc
KOS_CXX=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-g++
KOS_AS=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-as
KOS_LD=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-ld
#export KOS_CC KOS_CXX KOS_AS KOS_LD

# SH-4 bin utils
KOS_AR=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-ar
KOS_RANLIB=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-ranlib
KOS_OBJCOPY=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-objcopy
KOS_OBJDUMP=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-objdump
KOS_NM=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-nm
KOS_STRIP=/usr/local/cross-tools/sh-dc-elf-3.4.0/bin/sh-dc-elf-strip


#.PHONY: $(FFMPEG_LIB)
#$(FFMPEG_LIB):
#	$(MAKE) -C $(FFMPEG_DIR) -f Makefile.dcp

#clean : clean_libffmpeg
#clean_libffmpeg:
#	$(MAKE) -C $(FFMPEG_DIR) -f Makefile.dcp clean

#depend : depend_libffmpeg
#depend_libffmpeg:
#	$(MAKE) -C $(FFMPEG_DIR) -f Makefile.dcp depend
