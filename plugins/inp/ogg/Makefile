#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.3 2002-09-06 01:14:50 zig Exp $
#

SUBDIRS=ogg vorbis

LEFS := $(shell basename `pwd`).lef

LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 
KOS_LOCAL_INCS := -I../../../include -Iid3tag
KOS_INCS += -Iinclude
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
# DO NOT DELETE
