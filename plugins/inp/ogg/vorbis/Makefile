# KallistiOS Ogg/Vorbis Decoder Library
#
# Library Makefile
# (c)2001 Thorsten Titze
# Based on KOS Makefiles by Dan Potter

LIBRARY=yes
OBJS = mdct.o smallft.o block.o envelope.o window.o lsp.o lpc.o analysis.o synthesis.o psy.o info.o time0.o floor1.o floor0.o res0.o mapping0.o registry.o codebook.o sharedbook.o lookup.o bitbuffer.o 
OPTIMIZE =  $(OPTIMIZE_FOR_SPEED)
KOS_INCS = -I. -I../include
KOS_LOCAL_CFLAGS += -DLITTLE_ENDIAN=1

include ../../../../Makefile.inc
# DO NOT DELETE

mdct.o: /home/ben/kos/include/stdio.h /home/ben/kos/include/unistd.h
mdct.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
mdct.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/malloc.h
mdct.o: /home/ben/kos/include/string.h /home/ben/kos/include/math.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/math.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/time.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
mdct.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
mdct.o: ../include/vorbis/codec.h ../include/ogg/ogg.h
mdct.o: ../include/ogg/os_types.h ../include/ogg/config_types.h mdct.h os.h
mdct.o: misc.h kos_fake_func.h
smallft.o: /home/ben/kos/include/string.h /home/ben/kos/include/unistd.h
smallft.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
smallft.o: /home/ben/kos/include/math.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/math.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/time.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
smallft.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h smallft.h
smallft.o: ../include/vorbis/codec.h ../include/ogg/ogg.h
smallft.o: ../include/ogg/os_types.h ../include/ogg/config_types.h misc.h
smallft.o: kos_fake_func.h /home/ben/kos/include/malloc.h
block.o: /home/ben/kos/include/stdio.h /home/ben/kos/include/unistd.h
block.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
block.o: /home/ben/kos/include/string.h ../include/ogg/ogg.h
block.o: ../include/ogg/os_types.h ../include/ogg/config_types.h
block.o: ../include/vorbis/codec.h codec_internal.h envelope.h iir.h
block.o: smallft.h codebook.h psy.h backends.h window.h mdct.h lpc.h
block.o: registry.h misc.h kos_fake_func.h /home/ben/kos/include/malloc.h
block.o: os.h /home/ben/kos/include/math.h
block.o: /home/ben/kos/include/newlib-libm-sh4/math.h
block.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
block.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
block.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
block.o: /home/ben/kos/include/newlib-libm-sh4/time.h
block.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
block.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
block.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
envelope.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
envelope.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
envelope.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
envelope.o: /home/ben/kos/include/stdio.h /home/ben/kos/include/math.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/math.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/time.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
envelope.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
envelope.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
envelope.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
envelope.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
envelope.o: backends.h os.h scales.h misc.h kos_fake_func.h iir.c
window.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
window.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
window.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/math.h
window.o: /home/ben/kos/include/newlib-libm-sh4/math.h
window.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
window.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
window.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
window.o: /home/ben/kos/include/newlib-libm-sh4/time.h
window.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
window.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
window.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h os.h
window.o: ../include/ogg/os_types.h ../include/ogg/config_types.h misc.h
window.o: ../include/vorbis/codec.h ../include/ogg/ogg.h kos_fake_func.h
lsp.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
lsp.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
lsp.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/math.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/math.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/time.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
lsp.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
lsp.o: /home/ben/kos/include/string.h lsp.h os.h ../include/ogg/os_types.h
lsp.o: ../include/ogg/config_types.h misc.h ../include/vorbis/codec.h
lsp.o: ../include/ogg/ogg.h kos_fake_func.h lookup.h scales.h
lpc.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
lpc.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
lpc.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
lpc.o: /home/ben/kos/include/math.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/math.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/time.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
lpc.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h os.h
lpc.o: ../include/ogg/os_types.h ../include/ogg/config_types.h smallft.h
lpc.o: ../include/vorbis/codec.h ../include/ogg/ogg.h lpc.h scales.h misc.h
lpc.o: kos_fake_func.h
analysis.o: /home/ben/kos/include/stdio.h /home/ben/kos/include/unistd.h
analysis.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
analysis.o: /home/ben/kos/include/string.h /home/ben/kos/include/math.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/math.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/time.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
analysis.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
analysis.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
analysis.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
analysis.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
analysis.o: backends.h registry.h scales.h os.h kos_fake_func.h
synthesis.o: /home/ben/kos/include/stdio.h /home/ben/kos/include/unistd.h
synthesis.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
synthesis.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
synthesis.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
synthesis.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
synthesis.o: backends.h registry.h misc.h kos_fake_func.h
synthesis.o: /home/ben/kos/include/malloc.h os.h /home/ben/kos/include/math.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/math.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/time.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
synthesis.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
psy.o: /home/ben/kos/include/math.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/math.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/time.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
psy.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
psy.o: /home/ben/kos/include/string.h /home/ben/kos/include/unistd.h
psy.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
psy.o: ../include/vorbis/codec.h ../include/ogg/ogg.h
psy.o: ../include/ogg/os_types.h ../include/ogg/config_types.h
psy.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
psy.o: backends.h masking.h os.h lpc.h scales.h misc.h kos_fake_func.h
psy.o: /home/ben/kos/include/malloc.h
info.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
info.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
info.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
info.o: /home/ben/kos/include/ctype.h ../include/ogg/ogg.h
info.o: ../include/ogg/os_types.h ../include/ogg/config_types.h
info.o: ../include/vorbis/codec.h codec_internal.h envelope.h iir.h smallft.h
info.o: codebook.h psy.h backends.h registry.h window.h misc.h
info.o: kos_fake_func.h os.h /home/ben/kos/include/math.h
info.o: /home/ben/kos/include/newlib-libm-sh4/math.h
info.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
info.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
info.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
info.o: /home/ben/kos/include/newlib-libm-sh4/time.h
info.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
info.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
info.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
time0.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
time0.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
time0.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
time0.o: ../include/vorbis/codec.h ../include/ogg/ogg.h
time0.o: ../include/ogg/os_types.h ../include/ogg/config_types.h
time0.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
time0.o: backends.h registry.h misc.h kos_fake_func.h
floor1.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
floor1.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
floor1.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
floor1.o: /home/ben/kos/include/math.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/math.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/time.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
floor1.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
floor1.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
floor1.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
floor1.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
floor1.o: backends.h registry.h misc.h kos_fake_func.h scales.h os.h
floor1.o: /home/ben/kos/include/stdio.h
floor0.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
floor0.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
floor0.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
floor0.o: /home/ben/kos/include/math.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/math.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/time.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
floor0.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
floor0.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
floor0.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
floor0.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
floor0.o: backends.h registry.h lpc.h lsp.h scales.h os.h misc.h
floor0.o: kos_fake_func.h /home/ben/kos/include/stdio.h
res0.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
res0.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
res0.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
res0.o: /home/ben/kos/include/math.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/math.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/time.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
res0.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
res0.o: /home/ben/kos/include/stdio.h ../include/ogg/ogg.h
res0.o: ../include/ogg/os_types.h ../include/ogg/config_types.h
res0.o: ../include/vorbis/codec.h codec_internal.h envelope.h iir.h smallft.h
res0.o: codebook.h psy.h backends.h registry.h misc.h kos_fake_func.h os.h
mapping0.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
mapping0.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
mapping0.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/stdio.h
mapping0.o: /home/ben/kos/include/string.h /home/ben/kos/include/math.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/math.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/time.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
mapping0.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
mapping0.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
mapping0.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
mapping0.o: codec_internal.h envelope.h iir.h smallft.h codebook.h psy.h
mapping0.o: backends.h bitbuffer.h registry.h misc.h kos_fake_func.h os.h
mapping0.o: lpc.h lsp.h mdct.h scales.h
registry.o: ../include/vorbis/codec.h ../include/ogg/ogg.h
registry.o: ../include/ogg/os_types.h
registry.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
registry.o: ../include/ogg/config_types.h codec_internal.h envelope.h iir.h
registry.o: smallft.h codebook.h psy.h backends.h registry.h misc.h
registry.o: kos_fake_func.h /home/ben/kos/include/malloc.h
codebook.o: /home/ben/kos/include/stdlib.h /home/ben/kos/include/unistd.h
codebook.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
codebook.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/string.h
codebook.o: /home/ben/kos/include/math.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/math.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/time.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
codebook.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
codebook.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
codebook.o: ../include/ogg/config_types.h ../include/vorbis/codec.h
codebook.o: codebook.h scales.h os.h misc.h kos_fake_func.h
sharedbook.o: /home/ben/kos/include/malloc.h /home/ben/kos/include/math.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/math.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/time.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
sharedbook.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h
sharedbook.o: /home/ben/kos/include/string.h /home/ben/kos/include/unistd.h
sharedbook.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
sharedbook.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
sharedbook.o: ../include/ogg/config_types.h os.h ../include/vorbis/codec.h
sharedbook.o: codebook.h scales.h /home/ben/kos/include/stdio.h
lookup.o: /home/ben/kos/include/math.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/math.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/sys/reent.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/_ansi.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/sys/config.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/time.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/machine/time.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/machine/types.h
lookup.o: /home/ben/kos/include/newlib-libm-sh4/machine/ieeefp.h lookup.h
lookup.o: lookup_data.h os.h ../include/ogg/os_types.h
lookup.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
lookup.o: ../include/ogg/config_types.h
bitbuffer.o: /home/ben/kos/include/string.h /home/ben/kos/include/unistd.h
bitbuffer.o: /home/ben/kos/kernel/arch/dreamcast/include/arch/types.h
bitbuffer.o: ../include/ogg/ogg.h ../include/ogg/os_types.h
bitbuffer.o: ../include/ogg/config_types.h ../include/vorbis/codec.h misc.h
bitbuffer.o: kos_fake_func.h /home/ben/kos/include/malloc.h bitbuffer.h
bitbuffer.o: codebook.h
