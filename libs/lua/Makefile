# Main application Makefile for Dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard
#
# $Id: Makefile,v 1.1 2002-09-13 16:02:36 zig Exp $

LIBNAME=liblua.a
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TOPDIR:=$(dir $(shell pwd))

KOS_LOCAL_INCS := -I../include -I..

KOS_LOCAL_CFLAGS += -DDREAMMP3_HOME="\"$(TOPDIR)\""

#include config

include ../Makefile.inc
# DO NOT DELETE
