# Main application Makefile for Dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard
#
# $Id: Makefile,v 1.5 2002-09-27 16:49:41 benjihan Exp $

LIBNAME=libdreammp3.a
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TOPDIR:=$(dir $(shell pwd))

KOS_LOCAL_INCS := -I../include -I.. -I../libs/z -I../libs/translator

KOS_LOCAL_CFLAGS += -DDREAMMP3_HOME="\"$(TOPDIR)\""

include ../Makefile.inc
# DO NOT DELETE
