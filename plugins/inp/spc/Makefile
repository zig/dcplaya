#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.3 2002-09-20 01:24:36 vincentp Exp $
#

SUBDIRS=spc

LEZ=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 

KOS_LOCAL_INCS := -Ispc -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
