#
# Make exe plugins BBA driver
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1 2004/07/09 21:20:17 vincentp Exp $
#

LEZ=yes

LEF_EXTRA := 

KOS_LOCAL_CFLAGS += -DHOST_LITTLE_ENDIAN -DDCPLAYA=1

KOS_LOCAL_INCS += -I.. -I. -I../../../../include -I../../../../libs

OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

TARGETS = bba.lez


include ../../../../Makefile.inc

