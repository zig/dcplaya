# Makefile - dcplaya - data
#
# $Id: Makefile,v 1.13 2003-07-30 00:41:31 benjihan Exp $
#

TARGETS=
CLEAN_LOCAL=tmp/* romdisk/* romdisk.* vmu_header.bin $(HEADERS)
REQUIRED_LOCAL=req_local

HEADERS= ../include/vmu_dcplaya.h ../include/vmu_dcplaya4.h ../include/vmu_font.h

MKVMUHEADER=$(UTILS_DIR)/mkvmuheader


my_all: vmuimg romdisk

# Images
fntimg : romdisk/font16x16.tga romdisk/font8x14.tga
	@echo "[$@] from [$^]"

bdrimg : romdisk/bordertile.tga romdisk/dcpsprites.tga romdisk/dcpbkg2.jpg
	@echo "[$@] from [$^]"

vmuimg : $(HEADERS)
	@echo "[$@]"

.INTERMEDIATE: romimg fntimg bdrimg
romimg: fntimg bdrimg
	@echo "[$@]"

romdisk/%.tga : img/%.tga
	@echo "[$@] from [$^]"
	@cp $< $@

romdisk/%.jpg : img/%.jpg
	@echo "[$@] from [$^]"
	@cp $< $@

# ----------------------------------------------------------------------
# ROMDISK
# ----------------------------------------------------------------------

# Copy ARM stream driver into romdisk
romdisk/stream.drv: ../arm/stream.drv
	@echo "[$@]"
	@cp -f $< $@

# VMU header file
# romdisk/vmu_header.gz : vmu_header.bin
# 	gzip -c --best -n $< > $@
romdisk/vmu_header.bin : vmu_header.bin
	@echo "[$@]"
	@cp -f $< $@

$(MKVMUHEADER):
	$(MAKE) -C ../utils/ mkvmuheader

vmu_header.bin: $(MKVMUHEADER)
	@echo "[$@]"
	@$(MKVMUHEADER) \
	"dcplaya "$(DCPLAYA_VERSION) \
	"dcplaya user data (playlist...)" \
	"dcplaya "$(DCPLAYA_VERSION) \
	img/vmu_icons.tga 6 $@

.PHONY: vmuheader
vmuheader: romdisk/vmu_header.bin
	@echo "[$@]"


# ROM-disk fixed data
.PHONY: romdisk_data
romdisk_data:
	@echo "[$@]"
	@find romdisk_data -maxdepth 1 -type f -exec cp -f {} romdisk \;

.PHONY: romdisk.img
romdisk.img: romdisk_data vmuheader romimg romdisk/stream.drv
	@echo "[$@]"
	@$(KOS_GENROMFS) -f $@ -d romdisk -v 2> /dev/null

.PHONY: romdisk
romdisk: romdisk.o
	@echo "[$@]"


.PHONY: req_local
req_local:
	mkdir -p tmp romdisk romdisk_data

# ----------------------------------------------------------------------
include ../Makefile.inc
# ----------------------------------------------------------------------

# Must be done after include of Makefile.inc because of conditionnal..

ifeq 'x$(PPMTOXPM)' 'xecho'

../include/%.h : xpm/%.h
	@echo "Copying default VMU pixmap [$(@F)]"
	@cp -f $< $@
else

../include/%.h : img/%.ppm
	@echo "Creating VMU pixmap '$@' with $(PPMTOXPM)"
	@$(PPMTOXPM) -name $(<:img/%.ppm=%) $<  2> /dev/null |\
	 egrep -v '^"[0-9].*$$|^". c #.*$$'  > $@
endif

