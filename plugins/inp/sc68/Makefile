#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.3 2002-09-20 00:22:14 benjihan Exp $
#

SC68HOME=/home/ben/sc68-from-cvs/sc68
SC68PREFIX=/opt/dreamcast

SC68INCS=.
SC68LIB=sc68app io68 emu68 file68 

SUBDIRS=

CLEAN_LOCAL=sc68disk.img

LEF=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)\
 -L$(SC68PREFIX)/lib $(foreach dir,$(SC68LIB),-l$(dir))

KOS_LOCAL_INCS := $(foreach dir, $(SC68INCS), -I"$(SC68HOME)/$(dir)")\
 -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

OBJS =  sc68_driver.o sc68disk.o

myall : all

#.PHONY: sc68disk.img
sc68disk.img:
	$(KOS_GENROMFS) -f $@ -d romdisk -V sc68disk

include ../../../Makefile.inc
