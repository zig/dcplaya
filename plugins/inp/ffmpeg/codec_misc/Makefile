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

#LEF_EXTRA := $(FFMPEG_DIR)/libavcodec/libavcodec_misc.a -R ../ffmpeg.lef
LEF_EXTRA := -L$(FFMPEG_DIR)/libavcodec/ -lavcodec_misc

KOS_LOCAL_CFLAGS += -DHOST_LITTLE_ENDIAN -DDCPLAYA=1

KOS_LOCAL_INCS += -I.. -I. -I../../../../include -I../../../../libs\
 $(foreach i,$(FFMPEG_INCS),-I$(FFMPEG_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TARGETS = codec_misc.codez

#test:
#	echo $(OBJS)

include ../../../../Makefile.inc

.PRECIOUS : %.codec

%.codez : %.codec
	@echo "Build [$@] from [$^]"
	@gzip --best -n -c -v $< >$@

%.codec : $(OBJS) $(filter %.a %.o %.elf %.lef, $(LEF_EXTRA)) $(LEF_TARGET_EXTRA_DEP)
	@echo "Build [$@] from [$^]"
	$(KOS_LD) -b elf32-shl -d -O 3 -EL -r --cref -M -Map $*.map -x -O -S $^ $(LEF_EXTRA) -o $@
	$(KOS_STRIP) --strip-unneeded $@
	$(KOS_OBJDUMP) -t $@ | grep '*COM*' || echo ":_) No *COM* found."

