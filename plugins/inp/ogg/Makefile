#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.2 2002-08-26 20:12:22 ben Exp $
#

SUBDIRS=ogg vorbis

LEFS := $(shell basename `pwd`).lef

LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 
KOS_LOCAL_INCS := -I../../../include -Iid3tag
KOS_INCS += -Iinclude
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
# DO NOT DELETE

ogg_driver.o: /home/ben/kos/include/kos.h /home/ben/kos/include/ctype.h
ogg_driver.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/stdio.h
ogg_driver.o: /home/ben/kos/include/unistd.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
ogg_driver.o: /home/ben/kos/include/string.h /home/ben/kos/include/kos/fs.h
ogg_driver.o: /home/ben/kos/include/kos/limits.h
ogg_driver.o: /home/ben/kos/include/kos/fs_builtin.h
ogg_driver.o: /home/ben/kos/include/kos/fs_romdisk.h
ogg_driver.o: /home/ben/kos/include/kos/pcx.h
ogg_driver.o: /home/ben/kos/include/kos/thread.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/irq.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/arch.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/video.h
ogg_driver.o: /home/ben/kos/include/sys/queue.h
ogg_driver.o: /home/ben/kos/include/sys/iovec.h
ogg_driver.o: /home/ben/kos/include/kos/sem.h
ogg_driver.o: /home/ben/kos/include/kos/cond.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/cache.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/dbgio.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/spinlock.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/syscall.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/timer.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/biosfont.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/cdrom.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/controller.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fs_dcload.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fs_iso9660.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fs_vmu.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/g2.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/keyboard.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/maple.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/mouse.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/spu.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/ta.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/vmu.h
ogg_driver.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fmath.h
ogg_driver.o: /home/ben/kos/include/os/process.h
ogg_driver.o: /home/ben/kos/include/os/elf.h
ogg_driver.o: /home/ben/kos/include/os/svcmpx.h
ogg_driver.o: /home/ben/kos/include/dcutils/3dutils.h
ogg_driver.o: /home/ben/kos/include/dcutils/matrix.h
ogg_driver.o: /home/ben/kos/include/dcutils/bspline.h
ogg_driver.o: /home/ben/kos/include/dcutils/pcx.h
ogg_driver.o: /home/ben/kos/include/dcutils/precompiler.h
ogg_driver.o: /home/ben/kos/include/dcutils/pvrutils.h include/ogg/os_types.h
ogg_driver.o: include/ogg/config_types.h sndvorbisfile.h
ogg_driver.o: ../../../include/inp_driver.h ../../../include/playa_info.h
ogg_driver.o: ../../../include/any_driver.h ../../../include/driver_option.h
ogg_driver.o: ../../../include/pcm_buffer.h ../../../include/fifo.h
sndvorbisfile.o: /home/ben/kos/include/kos.h /home/ben/kos/include/ctype.h
sndvorbisfile.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/stdio.h
sndvorbisfile.o: /home/ben/kos/include/unistd.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
sndvorbisfile.o: /home/ben/kos/include/string.h
sndvorbisfile.o: /home/ben/kos/include/kos/fs.h
sndvorbisfile.o: /home/ben/kos/include/kos/limits.h
sndvorbisfile.o: /home/ben/kos/include/kos/fs_builtin.h
sndvorbisfile.o: /home/ben/kos/include/kos/fs_romdisk.h
sndvorbisfile.o: /home/ben/kos/include/kos/pcx.h
sndvorbisfile.o: /home/ben/kos/include/kos/thread.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/irq.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/arch.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/video.h
sndvorbisfile.o: /home/ben/kos/include/sys/queue.h
sndvorbisfile.o: /home/ben/kos/include/sys/iovec.h
sndvorbisfile.o: /home/ben/kos/include/kos/sem.h
sndvorbisfile.o: /home/ben/kos/include/kos/cond.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/cache.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/dbgio.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/spinlock.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/syscall.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/timer.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/biosfont.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/cdrom.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/controller.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fs_dcload.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fs_iso9660.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fs_vmu.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/g2.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/keyboard.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/maple.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/mouse.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/spu.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/ta.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/vmu.h
sndvorbisfile.o: /home/ben/kos/kernel/arch/dreamcast/include/dc/fmath.h
sndvorbisfile.o: /home/ben/kos/include/os/process.h
sndvorbisfile.o: /home/ben/kos/include/os/elf.h
sndvorbisfile.o: /home/ben/kos/include/os/svcmpx.h
sndvorbisfile.o: /home/ben/kos/include/dcutils/3dutils.h
sndvorbisfile.o: /home/ben/kos/include/dcutils/matrix.h
sndvorbisfile.o: /home/ben/kos/include/dcutils/bspline.h
sndvorbisfile.o: /home/ben/kos/include/dcutils/pcx.h
sndvorbisfile.o: /home/ben/kos/include/dcutils/precompiler.h
sndvorbisfile.o: /home/ben/kos/include/dcutils/pvrutils.h
sndvorbisfile.o: include/vorbis/codec.h include/ogg/ogg.h
sndvorbisfile.o: include/ogg/os_types.h include/ogg/config_types.h
sndvorbisfile.o: sndvorbisfile.h
