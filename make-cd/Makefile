#
# Makefile for burning dcplaya master CD.
#
# by Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.19 2003-04-21 16:26:36 vincentp Exp $
#
# This makefile is pretty ugly. Sorry :( I'll try to make a clean one
# when I get some time.
#
# Add a minimal target to use extra_data_minimal instead of normal extra_data
# as extra-data directory
#

CDRECORD	= cdrecord dev=0,0,0 speed=4
MAKEIP		= /opt/sh-elf/bin/makeip
SCRAMBLE	= /opt/sh-elf/bin/scramble
DD		= dd
CP		= cp
MKISOFS		= mkisofs -r -f
CAT		= cat
TOUCH		= touch
1ST_READ	= dcplaya.bin
ELF_TARGET	= ../dreammp3.elf
OBJCOPY		= /opt/sh-elf/bin/sh-elf-objcopy
DATE		:= $(shell date +%Y/%m/%d)
VERSION         := $(shell cat ../VERSION)

ifeq "$(strip $(filter minimal,$(MAKECMDGOALS)))" "minimal"
EXTRA_DATA_DIR  := extra_data_minimal
else
EXTRA_DATA_DIR  := extra_data
endif

all: success

.PHONY: minimal
minimal:
	@echo "Make a minimal CD extra_data [$(EXTRA_DATA_DIR)]"

success: $(1ST_READ) IP.BIN burn
	@echo "Successfully burned DCPLAYA !!!"

.PHONY: burn
burn: burn-audio burn-data

.PHONY: RELEASE.txt
RELEASE.txt:
	@echo "[$@]"
	@(\
	echo -n   "version  : dcplaya ";\
	cat ../VERSION;\
	echo -e "\nbuild by : `id -un` @ `uname --nodename`";\
	echo      "date     : $(DATE)";\
	) > $@

.PHONY: README.txt
README.txt: ../README ../VERSION
	@echo "[$@]"
	@sed -e "s#@BUILT-DATE@#${DATE}#" -e "s#@VERSION@#${VERSION}#" $< > $@

$(ELF_TARGET):
	@echo "[$@]"
	@make --quiet -C .. all

$(1ST_READ): $(ELF_TARGET)
	@echo "[$@]"
	@$(OBJCOPY) -R .stack -S -O binary $< $@ \
	|| ( rm -f $(1ST_READ) && test -z 1 )

AUDIOFROM := "$(EXTRA_DATA_DIR)/Jess Lysen/Jess Lysen - B1 - Revival.mp3"
zero:
	echo "Making audio from " $(AUDIOFROM);
	if [ -f $(AUDIOFROM) -a ! -z "`which mpg123`" ] ; then\
	echo "Making audio from " $(AUDIOFROM);\
	mpg123 -t -s $(AUDIOFROM) > zero;\
	else \
	echo "Make audio from /dev/zero";\
	$(DD) if=/dev/zero of=zero bs=2352 count=300;\
	fi

audio.raw: zero
	@echo "[$@]"
	@$(DD) if=zero of=audio.raw bs=2352 count=300

burn-audio: audio.raw
	@echo "[$@]"
	@$(CDRECORD) -multi -audio audio.raw && touch burn-audio

.PHONY: IP.txt
IP.txt: IP.m4
	@echo "[$@]"
	@m4 -DVERSION=$(DCPLAYA_VERSION) $< > $@

IP.BIN: IP.txt
	@echo "[$@]"
# This one seems to be ok !
	@cp IP-dcload.bin $@
# Does not work :`(
#	$(MAKEIP) $< $@ 

1ST_READ.BIN: $(1ST_READ)
	@echo "[$@]"
	@$(SCRAMBLE) $< $@

data.raw: tmp.iso IP.BIN
	@echo "[$@]"
	@( cat IP.BIN ; dd if=tmp.iso bs=2048 skip=16 ) > $@

tmp.iso: dcplaya-data
	@echo "[ISO image]"
#	$(MKISOFS) -C `$(CDRECORD) -msinfo` -l -iso-level 2 -r -o tmp.iso iso
	$(MKISOFS) -C `$(CDRECORD) -msinfo` -o $@ 1ST_READ.BIN iso

test.iso: dcplaya-data
	@echo "[ISO test-image]"
	@$(MKISOFS) -o $@ 1ST_READ.BIN iso
	@rm -rf mnt
#	@mkdir mnt
#	@mount $@ mnt -o loop &&\
#	echo "CD image mounted on ./mnt" &&\
#	echo "type umount mnt to unmount"

test: test.iso
	@echo [$@]
	@dc-tool -x $(ELF_TARGET) -i test.iso

.PHONY: burn-data
burn-data: burn-audio data.raw 
	@echo "[$@]"
	$(CDRECORD) -xa1 data.raw \
	&& rm -f audio.raw data.raw burn-audio tmp.iso

.PHONY: dcplaya-docs
dcplaya-docs :
	@echo "[$@]"
	@$(MAKE) -C ../doc txt zml

extra-data:
	@echo "[$@] from [$(EXTRA_DATA_DIR)]"
	@mkdir -p iso
	@cp -RL "$(EXTRA_DATA_DIR)"/* iso/

lua-home:
	@echo "[$@]"
	@mkdir -p iso/dcplaya
	@cp -a ../autorun.lua ../dcplayarc.lua iso/dcplaya
	@sed -e "s#@BUILT-DATE@#${DATE}#" ../userconf-release.lua \
		> iso/dcplaya/userconf.lua

lua-libs:
	@echo "[$@]"
	@mkdir -p iso/dcplaya/lua
	@find ../lua -type f -name '*.lua' -exec cp {} iso/dcplaya/lua \;

lua-rsc:
	@echo "[$@]"
	@mkdir -p iso/dcplaya/lua/rsc/icons iso/dcplaya/lua/rsc/text
	@cp -f ../lua/rsc/icons/*.tga ../lua/rsc/icons/*.jpg \
		iso/dcplaya/lua/rsc/icons
	@cp -f ../lua/rsc/text/*.txt iso/dcplaya/lua/rsc/text

lua: lua-home lua-libs lua-rsc
	@echo "[$@]"

distrib-doc: README.txt RELEASE.txt
	@mkdir -p iso
	@cp -f $^ iso

distrib-devdoc: dcplaya-docs
	@mkdir -p iso/dcplaya
	@cp -a ../doc/doc iso/dcplaya

dynshell:
	@mkdir -p iso/dcplaya/dynshell
	@cp -a ../dynshell/dynshell.lez iso/dcplaya/dynshell

plugins: dynshell
	@(cd ..; find plugins -type f -name '*.lez') \
	| tar cf - -C .. -T - \
	| tar xf - -C iso/dcplaya

.PHONY: distrib
#distrib: 1ST_READ.BIN distrib-doc distrib-devdoc plugins lua
distrib: 1ST_READ.BIN plugins lua


.PHONY: dcplaya-data
dcplaya-data: clean_dir distrib extra-data
	@echo "[$@]"

.PHONY : clean

clean_audio:
	@echo "[clean audio]"
	@rm -f zero
	@if [ -e burn-audio ]; \
		then echo -e "Existing [burn-audio] file.\nPrevious burn has not been successfully complete.\nRun make force_clean to remove it."; test -z 1; \
	else rm -f audio.raw; fi

.PHONY : clean_dir
clean_dir:
	@echo "[$@]"
	@rm -rf iso/

.PHONY : clean_file
clean_file:
	@echo "[clean files]"
	@rm -f tmp.iso *.txt $(1ST_READ) IP.BIN data.raw 1ST_READ.BIN

.PHONY : clean
clean: clean_audio clean_file clean_dir

.PHONY: force_clean
force_clean: clean_file
	@echo "[force clean]"
	@rm -f zero audio.raw burn-audio

include ../Makefile.inc
