#
# Make visual plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.1.1.1 2002-08-26 14:14:59 ben Exp $
#

LEFS=$(shell basename `pwd`).lef

KOS_LOCAL_INCS := -I../../../include
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
# DO NOT DELETE

fftvlr.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
fftvlr.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
fftvlr.o: /home/ben/kos/include/malloc.h ../../../include/fft.h
fftvlr.o: ../../../include/int_fft.h ../../../include/vis_driver.h
fftvlr.o: ../../../include/any_driver.h ../../../include/driver_option.h
fftvlr.o: ../../../include/matrix.h ../../../include/obj3d.h
