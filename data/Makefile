# Makefile - dcplaya - data
#
# $Id: Makefile,v 1.2 2002-09-02 20:01:28 ben Exp $
#

TARGETS=
CLEAN_LOCAL=tmp/* romdisk/* romdisk.*

REQUIRED_LOCAL=req_local

my_all: vmuimg romdisk

# Images
bckimg : romdisk/dream68.red romdisk/dream68.blu romdisk/dream68.grn
fntimg : romdisk/font16x16.ppm
bdrimg : romdisk/bordertile.ppm

vmuimg : ../include/vmu_sc68.h ../include/vmu_font.h
	@echo "Build [$@]"

.INTERMEDIATE: romimg bckimg fntimg bdrimg
romimg: bckimg fntimg bdrimg

tmp/dream68.pnm : img/dream68.png
	pngtopnm $< > $@

romdisk/%.ppm : img/%.tga
	tgatoppm $< > tmp/$*.ppm && \
	ppmtorgb3 tmp/$*.ppm && \
	mv tmp/$*.red $@ && \
	rm -f tmp/$**

romdisk/%.red romdisk/%.blu romdisk/%.grn: tmp/%.pnm
	ppmtorgb3 $< && mv tmp/$*.[rgb][erl][dnu] romdisk

../include/%.h : img/%.ppm
	@echo "Creating VMU pixmap '$@'"
	@ppmtoxpm -name $(<:img/%.ppm=%) $<  2> /dev/null |\
	 egrep -v '^"[0-9].*$$|^". c #.*$$'  > $@

# ----------------------------------------------------------------------
# ROMDISK
# ----------------------------------------------------------------------

# Copy ARM stream driver into romdisk
romdisk/stream.drv: ../arm/stream.drv
	cp -fv $< $@

# ROM-disk fixed data
.PHONY: romdisk_data
romdisk_data:
	@echo "Building [$@]"
	@find romdisk_data -type f -exec cp -vf {} romdisk \;

romdisk.img: romdisk_data romimg romdisk/stream.drv
	@$(KOS_GENROMFS) -f $@ -d romdisk -v 2> /dev/null

.PHONY: romdisk
romdisk: romdisk.o

.PHONY: req_local
req_local:
	mkdir -p tmp romdisk romdisk_data

# ----------------------------------------------------------------------

include ../Makefile.inc
