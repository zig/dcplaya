#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.2 2002-09-25 03:21:22 benjihan Exp $
#

LIBRARY=yes
SUBDIRS=loaders drivers playercode mmio
OBJS =\
 ./drivers/drv_dcplaya.o\
 ./drivers/drv_nos.o\
 ./loaders/load_669.o\
 ./loaders/load_amf.o\
 ./loaders/load_dsm.o\
 ./loaders/load_far.o\
 ./loaders/load_gdm.o\
 ./loaders/load_imf.o\
 ./loaders/load_it.o\
 ./loaders/load_m15.o\
 ./loaders/load_med.o\
 ./loaders/load_mod.o\
 ./loaders/load_mtm.o\
 ./loaders/load_okt.o\
 ./loaders/load_s3m.o\
 ./loaders/load_stm.o\
 ./loaders/load_stx.o\
 ./loaders/load_ult.o\
 ./loaders/load_uni.o\
 ./loaders/load_xm.o\
 ./mmio/mmalloc.o\
 ./mmio/mmerror.o\
 ./mmio/mmio.o\
 ./playercode/mdreg.o\
 ./playercode/mdriver.o\
 ./playercode/mdulaw.o\
 ./playercode/mloader.o\
 ./playercode/mlreg.o\
 ./playercode/mlutil.o\
 ./playercode/mplayer.o\
 ./playercode/munitrk.o\
 ./playercode/mwav.o\
 ./playercode/npertab.o\
 ./playercode/sloader.o\
 ./playercode/virtch2.o\
 ./playercode/virtch.o\
 ./playercode/virtch_common.o


OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../../Makefile.inc
