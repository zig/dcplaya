TARGETS = dreammp3.elf

SH_LDFLAGS=-ml -m4-single-only -nostartfiles -nostdlib -static  -Wl,-Ttext=0x8c010000

SUBDIRS += arm plugins src

#ELF_EXTRA +=  plugins/inp/xing/xing.lef -L./src -ldreammp3 -los

#ELF_EXTRA += -L./src -ldreammp3
#ELF_EXTRA += -L$(KOS_BASE)/lib -los -ldcutils -lkallisti -lm -lgcc

WHOLE_LIBS=-ldreammp3,-ldcutils,-lkallisti
OPT_LIBS= -los -lgcc -lm 
ELF_EXTRA += -L./src -L$(KOS_BASE)/lib \
-Wl,--whole-archive,$(WHOLE_LIBS),--no-whole-archive \
$(OPT_LIBS)

#ELF_EXTRA +=  -shared -L./src -ldreammp3 -los
#ELF_EXTRA +=  `find src -type f -name  '*.o'` -los

KOS_INCS+= -I.
ALL_INCS= -I$(KOS_INCS) -I$(KOS_BASE)/kernel/arch/$(KOS_ARCH)/include
#ALL_DEFINES			= -D_arch_$(KOS_ARCH) $(filter -D%, $(KOS_CFLAGS) $(KOS_LOCAL_CFLAGS))

CLEAN_LOCAL=symtab.h tmp_symtab.h romdisk.*

my_all: all vmuimg

.PHONY: libmp3 arm

romdisk/stream.drv: arm/stream.drv
	cp -fv $< $@

# IMAGES
# ------
bckimg : romdisk/dream68.red romdisk/dream68.blu romdisk/dream68.grn
fntimg : romdisk/font16x16.ppm
bdrimg : romdisk/bordertile.ppm
vmuimg: include/vmu_sc68.h include/vmu_font.h

.INTERMEDIATE: romimg bckimg fntimg bdrimg
romimg: bckimg fntimg bdrimg

img/dream68.pnm : img/dream68.png
	pngtopnm $< > $@

romdisk/%.ppm : img/%.tga
	tgatoppm $< > img/$*.ppm && \
	ppmtorgb3 img/$*.ppm && \
	mv img/$*.red $@ && \
	rm -f img/$*.blu img/$*.grn img/$*.ppm

romdisk/%.red romdisk/%.blu romdisk/%.grn: img/%.pnm
	ppmtorgb3 $< && mv img/$*.[rgb][erl][dnu] romdisk

include/%.h : img/%.ppm
	@echo "Creating VMU pixmap '$@'"
	@ppmtoxpm -name $(<:img/%.ppm=%) $<  2> /dev/null | egrep -v '^"[0-9].*$$|^". c #.*$$'  > $@

mp3lib:
	$(MAKE) -C libmp3


# ROMDISK
# -------
romdisk.img: romimg romdisk/stream.drv
	@$(KOS_GENROMFS) -f romdisk.img -d romdisk -v 2> /dev/null

romdisk.o: romdisk.img
	@echo "Build [$@]"
	KOS_ARCH=$(KOS_ARCH) KOS_LD=$(KOS_LD) $(KOS_BASE)/utils/bin2o/bin2o romdisk.img romdisk romdisk.o

symtab.o: symtab.h 
.PHONY: symtab.h
symtab.h :
	@echo "Build [$@]" 
	@[ -e $@ ] || touch $@
	@utils/makesymb.sh $(TARGETS) > tmp_$@
	@diff tmp_$@ $@ || mv -fv tmp_$@ $@
	@rm -f tmp_$@

#$(sort $(OBJS) $^)

force_$(TARGETS): symtab.o main.o force_math.o romdisk.o
	@echo "Build [$@]"
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o $(TARGETS) $(KOS_START) \
		$^ $(ELF_EXTRA)

.PRECIOUS: $(TARGETS)

$(TARGETS): force_$(TARGETS) $(OBJS)
	@echo "Build [$@]"
	@echo "--------------------------------------------------"
	@( \
		pass=1; \
		rm -f tmp_$@; \
		[ -z "0" ]; \
		while [ $$? -ne 0 ]; do \
			cp -fv $@ tmp_$@; \
			echo "** [$@] PASS $$pass ";\
			pass=`expr $$pass + 1`;\
			$(MAKE) force_$@; \
			diff tmp_$@ $@; \
		done; \
		rm -f tmp_$@; \
	)
	cp -fv $@ full-symb-$@
	@sh-elf-strip -v $@		

send:
	@clear
	@dc-tool -x $(TARGETS) | tee debug.log

r: send

run: my_all send

DEPEND_EXTRA=depend_extra
depend_extra:
	touch "symtab.h"

include Makefile.inc
# DO NOT DELETE

force_math.o: /home/ben/kos/./include/math.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/math.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/sys/reent.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/_ansi.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/sys/config.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/time.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/machine/time.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/machine/types.h
force_math.o: /home/ben/kos/./include/newlib-libm-sh4/machine/ieeefp.h
symtab.o: include/lef.h
symtab.o: /home/ben/kos/./kernel/arch/dreamcast/include/arch/types.h
symtab.o: /home/ben/kos/./include/sys/queue.h symtab.h
