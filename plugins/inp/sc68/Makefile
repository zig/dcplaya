#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.5 2003-03-07 20:50:33 ben Exp $
#

SC68HOME=/home/ben/sc68-from-cvs/sc68
SC68PREFIX=/opt/dreamcast

SC68INCS=.
SC68LIB=sc68app io68 emu68 file68 

SUBDIRS=

MKROM=$(UTILS_DIR)/mkromimg2

CLEAN_LOCAL=sc68disk.img sc68newdisk.img

LEZ=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)\
 -L$(SC68PREFIX)/lib $(foreach dir,$(SC68LIB),-l$(dir))

KOS_LOCAL_INCS := $(foreach dir, $(SC68INCS), -I"$(SC68HOME)/$(dir)")\
 -I../../../include -I.  -I../../../libs/z
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

OBJS =  sc68_driver.o fs_rz.o sc68newdisk.o

myall : all

sc68disk.img:
	$(KOS_GENROMFS) -f $@ -d romdisk -V sc68disk

$(MKROM):
	@$(MAKE) -C $(dir $(MKROM)) $(notdir $@)

sc68newdisk.img: $(MKROM)
	 $(MKROM) $@ `find romdisk -maxdepth 1 -type f`

include ../../../Makefile.inc
