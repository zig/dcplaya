# Main application Makefile for Dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard
#
# $Id: Makefile,v 1.4 2002-09-18 12:04:38 zig Exp $

LIBRARY=yes
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TOPDIR:=$(dir $(shell pwd))

KOS_LOCAL_INCS := -I../../include -I.. -I.

KOS_LOCAL_CFLAGS += -DDREAMMP3_HOME="\"$(TOPDIR)\""

#include config

include ../../Makefile.inc
# DO NOT DELETE
