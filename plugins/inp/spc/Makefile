#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2002-09-06 23:12:40 ben Exp $
#

SUBDIRS=spc

LEFS=$(shell basename `pwd`).lef
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 

KOS_LOCAL_INCS := -Ispc -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
