#
# dcplaya Makefile
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.2 2002-12-15 16:15:03 ben Exp $
#

SUBDIRS=jpeg

LEZ=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 

#ELFS=$(shell basename `pwd`).elf
#ELF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)

KOS_LOCAL_INCS := -I. -Ijpeg -I../../../include -I../../../libs -I../../../libs/translator
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
