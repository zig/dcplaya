#
# Make visual plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.2 2004-07-04 14:16:45 vincentp Exp $
#

LEZ=yes

KOS_LOCAL_INCS := -I. -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc

LWIPDIR = $(KOS_BASE)/../kos-ports/lwip/lwip/src
ARCHDIR = $(LWIPDIR)/../../kos

KOS_LOCAL_CFLAGS := -DDCPLAYA=1 -DIPv4 -DLWIP_DEBUG
KOS_INCS +=  \
	-DIPv4 \
	-I$(LWIPDIR)/include -I$(ARCHDIR)/include \
	-I$(LWIPDIR)/include/ipv4 

KOS_LOCAL_INCS += -I../../../include -I../../../libs -I.

LEF_EXTRA := -L$(KOS_BASE)/../kos-ports/lib/dreamcast -llwip4
