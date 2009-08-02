# Main application Makefile for Dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard
#
# $Id: Makefile,v 1.7 2003-02-12 12:31:57 ben Exp $

LIBNAME=libdreammp3.a
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TOPDIR:=$(dir $(shell pwd))

KOS_LOCAL_INCS := -I../include -I.. -I../libs -I../libs/z
# -I../libs/translator

#KOS_LOCAL_CFLAGS += -DDREAMMP3_HOME="\"$(TOPDIR)\""

include ../Makefile.inc

pullsymbols.o: pullsymbols.c
	@echo "CC [$(@F)] from [$(<F)]"
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS) $(KOS_INC_RULES) -D_arch_$(KOS_ARCH) -c $< -o $@ -fno-builtin

# DO NOT DELETE
