#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.3 2002-09-12 17:57:31 ben Exp $
#

SUBDIRS=sidplay
KOS_LOCAL_INCS = -I. -Isidplay  -I../../../include
KOS_LOCAL_CXXFLAGS = -USID_HAVE_EXCEPTIONS -DSID_NO_STDIN_LOADER -DSID_NO_FILE_ACCESS  -DPACKAGE=\"libsidplay\" -DVERSION=\"1.36.55\" -DSIZEOF_CHAR=1 -DSIZEOF_SHORT_INT=2 -DSIZEOF_INT=4 -DSIZEOF_LONG_INT=4 -DSTDC_HEADERS=1 -DHAVE_LONG_FILE_NAMES=1

LEF=yes
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
