#
# Make visual plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.1 2004-07-03 00:17:17 vincentp Exp $
#

LEZ=yes

KOS_LOCAL_INCS := -I. -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

SUBDIRS = lwip

LWIPDIR := lwip/src
LWIPARCH := kos
SRCDIR := lwip

KOS_LOCAL_CFLAGS := -DDCPLAYA=1 -DIPv4 -DLWIP_DEBUG
KOS_LOCAL_INCS := -I../../../include -I../../../libs \
	-I$(SRCDIR) -I$(SRCDIR)/include \
	-I$(LWIPDIR)/include -I$(LWIPDIR)/arch/$(LWIPARCH)/include \
	-I$(LWIPDIR)/include/ipv4 -I.

LEF_TARGET_EXTRA_DEP := $(SRCDIR)/liblwip.a
LEF_EXTRA := -Llwip -llwip

include ../../../Makefile.inc
