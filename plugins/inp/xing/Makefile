#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.4 2002-09-12 17:57:31 ben Exp $
#

SUBDIRS=id3tag mpglib xingmp3

LEF=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 

#ELFS=$(shell basename `pwd`).elf
#ELF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)

KOS_LOCAL_INCS := -I../../../include -Iinclude -Iid3tag
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
