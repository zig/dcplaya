# Makefile - dcplaya - data
#
# $Id: Makefile,v 1.6 2002-12-16 23:39:35 ben Exp $
#

TARGETS=
CLEAN_LOCAL=tmp/* romdisk/* romdisk.* vmu_header.bin

REQUIRED_LOCAL=req_local

my_all: vmuimg romdisk

# Images
fntimg : romdisk/font16x16.tga romdisk/font8x14.tga
bdrimg : romdisk/bordertile.tga romdisk/dcpsprites.tga romdisk/dcpbkg.jpg

vmuimg : ../include/vmu_sc68.h ../include/vmu_font.h
	@echo "Build [$@]"

.INTERMEDIATE: romimg fntimg bdrimg
romimg: fntimg bdrimg

romdisk/%.tga : img/%.tga
	cp $< $@

romdisk/%.jpg : img/%.jpg
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

# VMU header file
# romdisk/vmu_header.gz : vmu_header.bin
# 	gzip -c --best -n $< > $@
romdisk/vmu_header.bin : vmu_header.bin
	cp -fv $< $@

vmu_header.bin:
	 $(UTILS_DIR)/mkvmuheader \
		"dcplaya "$(DCPLAYA_VERSION) \
		"dcplaya user data (playlist...)" \
		 "dcplaya "$(DCPLAYA_VERSION) \
		img/vmu_icons.tga 6 $@

.PHONY: vmuheader
vmuheader: romdisk/vmu_header.bin

# ROM-disk fixed data
.PHONY: romdisk_data
romdisk_data:
	@echo "Building [$@]"
	@find romdisk_data -type f -exec cp -vf {} romdisk \;

romdisk.img: romdisk_data vmuheader romimg romdisk/stream.drv
	@$(KOS_GENROMFS) -f $@ -d romdisk -v 2> /dev/null

.PHONY: romdisk
romdisk: romdisk.o

.PHONY: req_local
req_local:
	mkdir -p tmp romdisk romdisk_data

# ----------------------------------------------------------------------

include ../Makefile.inc
