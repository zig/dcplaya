# Makefile - dcplaya - data
#
# $Id: Makefile,v 1.4 2002-11-14 23:40:26 benjihan Exp $
#

TARGETS=
CLEAN_LOCAL=tmp/* romdisk/* romdisk.*

REQUIRED_LOCAL=req_local

my_all: vmuimg romdisk

# Images
fntimg : romdisk/font16x16.tga romdisk/font8x14.tga
bdrimg : romdisk/bordertile.tga romdisk/dcpsprites.tga

vmuimg : ../include/vmu_sc68.h ../include/vmu_font.h
	@echo "Build [$@]"

.INTERMEDIATE: romimg fntimg bdrimg
romimg: fntimg bdrimg

romdisk/%.tga : img/%.tga
	cp $< $@

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
