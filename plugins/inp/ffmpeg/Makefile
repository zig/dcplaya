#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2004-06-22 20:21:35 vincentp Exp $
#

LEZ=yes

FFMPEG_DIR		:= ffmpeg
FFMPEG_INCS 	:= .
FFMPEG_LIB		:= $(FFMPEG_DIR)/libavcodec.a $(FFMPEG_DIR)/libavformat.a

LEF_TARGET_EXTRA_DEP := $(FFMPEG_LIB)
LEF_EXTRA := -L$(FFMPEG_DIR) -lavcodec -lavformat

KOS_LOCAL_CFLAGS := -DHOST_LITTLE_ENDIAN -DDCPLAYA=1

KOS_LOCAL_INCS := -I../../../include -I../../../libs\
 $(foreach i,$(FFMPEG_INCS),-I$(FFMPEG_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)


include ../../../Makefile.inc

#.PHONY: $(FFMPEG_LIB)
#$(FFMPEG_LIB):
#	$(MAKE) -C $(FFMPEG_DIR) -f Makefile.dcp

#clean : clean_libffmpeg
#clean_libffmpeg:
#	$(MAKE) -C $(FFMPEG_DIR) -f Makefile.dcp clean

#depend : depend_libffmpeg
#depend_libffmpeg:
#	$(MAKE) -C $(FFMPEG_DIR) -f Makefile.dcp depend
