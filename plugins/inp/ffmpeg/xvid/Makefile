#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2004-07-09 21:20:17 vincentp Exp $
#

SUBDIRS = xvidcore-1.0.1

#LEZ=yes

FFMPEG_DIR	:= ../ffmpeg
FFMPEG_INCS	:= .

XVID_DIR := xvidcore-1.0.1

LEF_EXTRA := -L$(XVID_DIR) -lxvidcore

KOS_LOCAL_CFLAGS += -DHOST_LITTLE_ENDIAN -DDCPLAYA=1

KOS_LOCAL_INCS += -I.. -I. -I../../../../include -I../../../../libs\
	-I$(XVID_DIR)/src $(foreach i,$(FFMPEG_INCS),-I$(FFMPEG_DIR)/$(i))

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TARGETS = codec_xvid.codez

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

