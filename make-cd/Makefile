#
# Makefile for burning dcplaya master CD.
#
# by Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.7 2003-02-12 19:02:17 ben Exp $
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

all: success

success: $(1ST_READ) IP.BIN burn
	@[ -e id.txt ] && \
	expr `cat id.txt` + 1 > $@.tmp && \
	mv -f $@.tmp id.txt && \
	echo "Successfully burned DCPLAYA ID #`cat id.txt`"

.PHONY: burn
burn: burn-audio burn-data

.PHONY:id.txt
id.txt:	
	@[ -e $@ ] || echo "0" > $@
	@touch $@

.PHONY: RELEASE
RELEASE.txt:
	@(echo "dcplaya alpha version"; date) > $@

README.txt: ../README
	cp $< $@

$(ELF_TARGET):
	@make --quiet -C .. all

$(1ST_READ): $(ELF_TARGET)
	@echo "[$@]"
	$(OBJCOPY) -R .stack -S -O binary $< $@ \
	|| ( rm -f $(1ST_READ) && test -z 1 )

audio.raw:
	@echo "[$@]"
	@$(DD) if=/dev/zero of=audio.raw bs=2352 count=300

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
	cp IP-dcload.bin $@
# Does not work :`(
#	$(MAKEIP) $< $@ 

1ST_READ.BIN: $(1ST_READ)
	@echo "[$@]"
	$(SCRAMBLE) $< $@

data.raw: tmp.iso IP.BIN
	@echo "[$@]"
	( cat IP.BIN ; dd if=tmp.iso bs=2048 skip=16 ) > $@

tmp.iso: dcplaya-data
	@echo "[ISO image]"
#	$(MKISOFS) -C `$(CDRECORD) -msinfo` -l -iso-level 2 -r -o tmp.iso iso
	$(MKISOFS) -C `$(CDRECORD) -msinfo` -o $@ 1ST_READ.BIN iso

test.iso: dcplaya-data
	@echo "[ISO test-image]"
	@$(MKISOFS) -o $@ 1ST_READ.BIN iso
	@rm -rf mnt
	@mkdir mnt
	@mount $@ mnt -o loop
	@echo "CD image mounted on ./mnt"
	@echo "type umount mnt to unmount"

.PHONY: burn-data
burn-data: burn-audio data.raw 
	@echo "[$@]"
	$(CDRECORD) -xa1 data.raw \
	&& rm -f audio.raw data.raw burn-audio tmp.iso

.PHONY: dcplaya-data
dcplaya-data: id.txt RELEASE.txt README.txt 1ST_READ.BIN
	@echo "[$@]"
	@rm -rf dcplaya iso
	@mkdir -p iso dcplaya/dynshell
	@cp -a ../autorun.lua ../dcplayarc.lua ../userconf.lua dcplaya
	@cp -a ../dynshell/dynshell.lez dcplaya/dynshell
	@cp -a ../lua dcplaya
	@(cd ..; find plugins -type f -name '*.lez') \
	| tar cf - -C .. -T - \
	| tar xf - -C dcplaya
	@rm -rf `find dcplaya/ -name CVS -o -name '*~' -o -name '.#*'`
	@cp -RL README.txt id.txt RELEASE.txt extra_data/* iso/
	@mv dcplaya iso

.PHONY : clean

clean_audio:
	@echo "[clean audio]"
	@if [ -e burn-audio ]; \
		then echo -e "Existing [burn-audio] file.\nPrevious burn has not been successfully complete.\nRun make force_clean to remove it."; test -z 1; \
	else rm -f audio.raw; fi

.PHONY : clean_dir
clean_dir:
	@echo "[clean dirs]"
	@rm -rf  dcplaya/ iso/

.PHONY : clean_file
clean_file:
	@echo "[clean files]"
	@rm -f tmp.iso *.txt $(1ST_READ) IP.BIN data.raw 1ST_READ.BIN

.PHONY : clean
clean: clean_audio clean_file clean_dir

.PHONY: force_clean
force_clean: clean_file
	@echo "[force clean]"
	@rm -f audio.raw burn-audio

include ../Makefile.inc
