# Main application Makefile for Dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard
#
# $Id: Makefile,v 1.4 2002-09-20 00:22:14 benjihan Exp $

LIBNAME=libdreammp3.a
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TOPDIR:=$(dir $(shell pwd))

KOS_LOCAL_INCS := -I../include -I.. -I../libs/z

KOS_LOCAL_CFLAGS += -DDREAMMP3_HOME="\"$(TOPDIR)\""

include ../Makefile.inc
# DO NOT DELETE
