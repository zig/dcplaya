#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2002-09-21 09:53:40 benjihan Exp $
#

SUBDIRS=mikmod
KOS_LOCAL_INCS = -I.  -I../../../include -Imikmod/include

LEZ=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
