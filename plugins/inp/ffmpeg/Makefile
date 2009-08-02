#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.6 2007-03-17 14:40:29 vincentp Exp $
#

LEZ=yes

FFMPEG_DIR	:= ffmpeg
FFMPEG_INCS 	:= . libavformat libavcodec
FFMPEG_LIB	:= $(FFMPEG_DIR)/libavformat/libavformat.a $(FFMPEG_DIR)/libavcodec/libavcodec.a

LEF_TARGET_EXTRA_DEP := $(FFMPEG_LIB) \
			sha123/libsha123.a istream/libistream.a \
			id3tag/libid3tag.a \
#LEF_EXTRA := -L$(FFMPEG_DIR) -Lsha123 -Listream -lavformat -lavcodec -lsha123 -listream
LEF_EXTRA = -L$(KOS_BASE)/addons/lib/dreamcast -loggvorbisplay

KOS_LOCAL_CFLAGS += -DHOST_LITTLE_ENDIAN -DDCPLAYA=1 -falign-functions=32 -falign-loops=32 -falign-jumps=32

KOS_LOCAL_INCS += -I. -I../../../include -I../../../libs\
 $(foreach i,$(FFMPEG_INCS),-I$(FFMPEG_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)


SUBDIRS = sha123 istream id3tag codec_misc codec_net
# these subdirs need ffmpeg.lef to be build already (so they can use symbol in it)
DEPENDENT_SUBDIRS = codec_misc codec_net

include ../../../Makefile.inc

$(patsubst %, _dir_%, $(DEPENDENT_SUBDIRS)): $(FFMPEG_LIB) ffmpeg/config.h ffmpeg/config.mak ffmpeg.lef

$(TARGET): $(FFMPEG_LIB)

ffmpeg/config.h ffmpeg/config.mak: ffmpeg/configure
	cd ffmpeg ; \
	./configure --cc=$(KOS_CC) --as=$(KOS_AS) --target=sh4-linux --prefix=$(KOS_CC_BASE) --extra-cflags="-O2 -ffast-math -fomit-frame-pointer -fno-strict-aliasing -Wno-multichar -Wall -finline-functions $(KOS_CFLAGS) $(KOS_INC_RULES) -D_arch_$(KOS_ARCH)" --cpu=sh4 --disable-ffserver --disable-encoders --disable-ffmpeg --disable-ffplay --disable-network --disable-opts ; \
	cd ..

.PHONY: $(FFMPEG_LIB)
$(FFMPEG_LIB): ffmpeg/config.h ffmpeg/config.mak
	$(MAKE) -C $(FFMPEG_DIR) -f Makefile 

clean : clean_libffmpeg
clean_libffmpeg: ffmpeg/config.h ffmpeg/config.mak
	$(MAKE) -C $(FFMPEG_DIR) -f Makefile clean
	rm -f ffmpeg/config.h ffmpeg/config.mak

depend : depend_libffmpeg
depend_libffmpeg: ffmpeg/config.h ffmpeg/config.mak
	$(MAKE) -C $(FFMPEG_DIR) -f Makefile depend
