#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.6 2002-09-20 01:24:36 vincentp Exp $
#

SUBDIRS=vorbis ogg

LEZ=yes

LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 
KOS_LOCAL_INCS := -I. -I../../../include -Iid3tag
KOS_INCS += -Iinclude
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc

