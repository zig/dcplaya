#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2004-07-09 21:20:17 vincentp Exp $
#

#LEZ=yes

FFMPEG_DIR	:= ../ffmpeg
FFMPEG_INCS	:= . $(FFMPEG_DIR)/libavformat $(FFMPEG_DIR)/libavcodec
FFMPEG_LIB	:= $(FFMPEG_DIR)/libavformat/libavformat.a $(FFMPEG_DIR)/libavcodec/libavcodec.a

#LEF_TARGET_EXTRA_DEP := $(FFMPEG_LIB)
#LEF_EXTRA := -L$(FFMPEG_DIR) -R ../ffmpeg.lef -lavformat -lavcodec
#LEF_EXTRA := -R ../ffmpeg.lef $(FFMPEG_LIB)
LEF_EXTRA := -L$(FFMPEG_DIR)/libavcodec -L$(FFMPEG_DIR)/libavformat -lavcodec_misc
#LEF_EXTRA := ../ffmpeg.lef $(FFMPEG_LIB)

KOS_LOCAL_CFLAGS += -DHOST_LITTLE_ENDIAN -DDCPLAYA=1

KOS_LOCAL_INCS += -I.. -I. -I../../../../include -I../../../../libs\
 $(foreach i,$(FFMPEG_INCS),-I$(FFMPEG_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TARGETS = codec_misc.codez

#test:
#	echo $(OBJS)

include ../../../../Makefile.inc



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

.PRECIOUS : %.codec

%.codez : %.codec
	@echo "Build [$@] from [$^]"
	@gzip --best -n -c -v $< >$@

%.codec : $(OBJS) #$(filter %.a %.o %.elf %.lef, $(LEF_EXTRA)) $(LEF_TARGET_EXTRA_DEP)
	@echo "Build [$@] from [$^]"
	$(KOS_LD) -b elf32-shl -d -O 3 -EL -r --cref -M -Map $*.map -x -O -S $^ $(LEF_EXTRA) -o $@
	$(KOS_STRIP) --strip-unneeded $@
	$(KOS_OBJDUMP) -t $@ | grep '*COM*' || echo ":_) No *COM* found."

