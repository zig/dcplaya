# Makefile - dcplaya - data
#
# $Id: Makefile,v 1.16 2004-07-04 14:16:44 vincentp Exp $
#

TARGETS=
CLEAN_LOCAL=romdisk.* vmu_header.bin $(HEADERS) -r romdisk/
REQUIRED_LOCAL=req_local

HEADERS= ../include/vmu_dcplaya.h ../include/vmu_dcplaya4.h ../include/vmu_font.h

MKVMUHEADER=$(UTILS_DIR)/mkvmuheader


my_all: vmuimg all-romdisk

# Images
fntimg : romdisk/font16x16.tga romdisk/font8x14.tga
	@echo "[$@] from [$^]"

bdrimg : romdisk/bordertile.tga romdisk/dcpsprites.tga romdisk/dcpbkg3.jpg
	@echo "[$@] from [$^]"

vmuimg : $(HEADERS)
	@echo "[$@]"

.INTERMEDIATE: romimg fntimg bdrimg
romimg: fntimg bdrimg
	@echo "[$@]"

romdisk/%.tga : img/%.tga
	@echo "[$@] from [$^]"
	@mkdir -p $(@D) && cp -f $< $@

romdisk/%.jpg : img/%.jpg
	@echo "[$@] from [$^]"
	@mkdir -p $(@D) && cp -f $< $@

# ----------------------------------------------------------------------
# ROMDISK
# ----------------------------------------------------------------------

# Copy ARM stream driver into romdisk
romdisk/stream.drv: ../arm/stream.drv
	@echo "[$@]"
	@mkdir -p $(@D) && cp -f $< $@

# VMU header file
# romdisk/vmu_header.gz : vmu_header.bin
# 	gzip -c --best -n $< > $@
romdisk/vmu_header.bin : vmu_header.bin
	@echo "[$@]"
	@mkdir -p $(@D) && cp -f $< $@

$(MKVMUHEADER):
	@$(MAKE) --quiet -C ../utils/ mkvmuheader

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
.PHONY: romdisk-data
romdisk-data:
	@echo "[$@]"
	if test -d romdisk_data; then \
		find romdisk_data/ -type f -exec cp -vf {} romdisk/ \; ; \
	else echo "No [romdisk_data] extra file"; fi;

.PHONY: romdisk.img
romdisk.img: romdisk-data vmuheader romimg romdisk/stream.drv
	@echo "[$@]"
	@$(KOS_GENROMFS) -f $@ -d romdisk -v 2> /dev/null

.PHONY: all-romdisk
all-romdisk: romdisk.o
	@echo "[$@]"

.PHONY: req_local
req_local:
	@echo "[$@]"

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

