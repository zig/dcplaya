#
# Make input plugins for dreammp3
#
# (C) COPYRIGHT 2002 Ben(jamin) Gerard <ben@sasahipa.com>
#
#
# $Id: Makefile,v 1.1.1.1 2002-08-26 14:14:42 ben Exp $
#

SUBDIRS=id3tag mpglib xingmp3

LEFS=$(shell basename `pwd`).lef
LEF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a) 

ELFS=$(shell basename `pwd`).elf
ELF_EXTRA := $(foreach dir,$(SUBDIRS),$(dir)/lib$(dir).a)

KOS_LOCAL_INCS := -I../../../include -Iinclude -Iid3tag
OPTIMIZE = $(OPTIMIZE_FOR_SPEED)

include ../../../Makefile.inc
# DO NOT DELETE

id3.o: /home/ben/kos/./include/kos.h /home/ben/kos/./include/ctype.h
id3.o: /home/ben/kos/./include/malloc.h /home/ben/kos/./include/stdio.h
id3.o: /home/ben/kos/./include/unistd.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/types.h
id3.o: /home/ben/kos/./include/string.h /home/ben/kos/./include/kos/fs.h
id3.o: /home/ben/kos/./include/kos/limits.h
id3.o: /home/ben/kos/./include/kos/fs_builtin.h
id3.o: /home/ben/kos/./include/kos/fs_romdisk.h
id3.o: /home/ben/kos/./include/kos/pcx.h /home/ben/kos/./include/kos/thread.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/irq.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/arch.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/video.h
id3.o: /home/ben/kos/./include/sys/queue.h
id3.o: /home/ben/kos/./include/sys/iovec.h /home/ben/kos/./include/kos/sem.h
id3.o: /home/ben/kos/./include/kos/cond.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/cache.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/dbgio.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/spinlock.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/syscall.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/timer.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/biosfont.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/cdrom.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/controller.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fs_dcload.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fs_iso9660.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fs_vmu.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/g2.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/keyboard.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/maple.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/mouse.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/spu.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/ta.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/vmu.h
id3.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fmath.h
id3.o: /home/ben/kos/./include/os/process.h /home/ben/kos/./include/os/elf.h
id3.o: /home/ben/kos/./include/os/svcmpx.h
id3.o: /home/ben/kos/./include/dcutils/3dutils.h
id3.o: /home/ben/kos/./include/dcutils/matrix.h
id3.o: /home/ben/kos/./include/dcutils/bspline.h
id3.o: /home/ben/kos/./include/dcutils/pcx.h
id3.o: /home/ben/kos/./include/dcutils/precompiler.h
id3.o: /home/ben/kos/./include/dcutils/pvrutils.h include/id3.h
id3.o: ../../../include/file_wrapper.h id3tag/id3tag.h
id3.o: ../../../include/playa_info.h
xing_driver.o: /home/ben/kos/./include/kos.h /home/ben/kos/./include/ctype.h
xing_driver.o: /home/ben/kos/./include/malloc.h
xing_driver.o: /home/ben/kos/./include/stdio.h
xing_driver.o: /home/ben/kos/./include/unistd.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/types.h
xing_driver.o: /home/ben/kos/./include/string.h
xing_driver.o: /home/ben/kos/./include/kos/fs.h
xing_driver.o: /home/ben/kos/./include/kos/limits.h
xing_driver.o: /home/ben/kos/./include/kos/fs_builtin.h
xing_driver.o: /home/ben/kos/./include/kos/fs_romdisk.h
xing_driver.o: /home/ben/kos/./include/kos/pcx.h
xing_driver.o: /home/ben/kos/./include/kos/thread.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/irq.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/arch.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/video.h
xing_driver.o: /home/ben/kos/./include/sys/queue.h
xing_driver.o: /home/ben/kos/./include/sys/iovec.h
xing_driver.o: /home/ben/kos/./include/kos/sem.h
xing_driver.o: /home/ben/kos/./include/kos/cond.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/cache.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/dbgio.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/spinlock.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/syscall.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/timer.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/biosfont.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/cdrom.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/controller.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fs_dcload.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fs_iso9660.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fs_vmu.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/g2.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/keyboard.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/maple.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/mouse.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/spu.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/ta.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/vmu.h
xing_driver.o: /home/ben/kos/./kernel/arch/dreamcast/include/dc/fmath.h
xing_driver.o: /home/ben/kos/./include/os/process.h
xing_driver.o: /home/ben/kos/./include/os/elf.h
xing_driver.o: /home/ben/kos/./include/os/svcmpx.h
xing_driver.o: /home/ben/kos/./include/dcutils/3dutils.h
xing_driver.o: /home/ben/kos/./include/dcutils/matrix.h
xing_driver.o: /home/ben/kos/./include/dcutils/bspline.h
xing_driver.o: /home/ben/kos/./include/dcutils/pcx.h
xing_driver.o: /home/ben/kos/./include/dcutils/precompiler.h
xing_driver.o: /home/ben/kos/./include/dcutils/pvrutils.h
xing_driver.o: ../../../include/inp_driver.h ../../../include/playa_info.h
xing_driver.o: ../../../include/any_driver.h ../../../include/pcm_buffer.h
xing_driver.o: ../../../include/fifo.h include/id3.h
xing_driver.o: ../../../include/file_wrapper.h id3tag/id3tag.h
xing_driver.o: include/mhead.h include/L3.h include/itype.h include/port.h
