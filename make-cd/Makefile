#
# Makefile for burning dcplaya master CD.
#
# by Ben(jamin) Gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.1 2003-02-01 20:03:01 ben Exp $
#

CDRECORD	= echo #cdrecord dev=0,0,0 speed=4
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
	date > $@

README.txt: ../README
	cp $< $@

$(ELF_TARGET):
	@make --quiet -C .. all

$(1ST_READ): $(ELF_TARGET)
	$(OBJCOPY) -R .stack -S -O binary $< $@ || ( rm -f $(1ST_READ) && test -z 1 )

audio.raw:
	$(DD) if=/dev/zero of=audio.raw bs=2352 count=300

burn-audio: audio.raw
	@echo "CDRECORD audio"
	@$(CDRECORD) -multi -audio audio.raw && touch burn-audio

.PHONY: IP.txt
IP.txt: IP.m4
	@m4 -DVERSION=$(DCPLAYA_VERSION) $< > $@

IP.BIN: IP.txt
	@$(MAKEIP) IP.txt IP.BIN

1st_read.bin: $(1ST_READ)
	$(SCRAMBLE) $(1ST_READ) 1st_read.bin

data.raw: tmp.iso IP.BIN
	@echo "[$@]"
	( cat IP.BIN ; dd if=tmp.iso bs=2048 skip=16 ) > data.raw

tmp.iso: id.txt RELEASE.txt README.txt 1st_read.bin burn-audio
	@echo "[ISO image]"
#	$(MKISOFS) -C `$(CDRECORD) -msinfo` -o tmp.iso 1st_read.bin README.txt id.txt RELEASE.txt

.PHONY: burn-data
burn-data: data.raw
	@echo "CDRECORD data"
	@$(CDRECORD) -xa1 data.raw && rm -f audio.raw data.raw burn-audio tmp.iso

.PHONY : clean

clean_audio:
	@echo "[clean audio]"
	@if [ -e burn-audio ]; \
		then echo -e "Existing [burn-audio] file.\nPrevious burn has not been successfully complete.\nRun make force_clean to remove it."; test -z 1; \
	else rm -f audio.raw; fi

.PHONY : clean_file
clean_file:
	@echo "[clean files]"
	@rm -f tmp.iso *.txt $(1ST_READ) IP.BIN data.raw 1st_read.bin

.PHONY : clean
clean: clean_audio clean_file

.PHONY: force_clean
force_clean: clean_file
	@echo "[force clean]"
	@rm -f audio.raw burn-audio

# .PHONY: distclean
# distclean: clean
# 	@echo "[distclean]"
# 	@rm -f *~

include ../Makefile.inc
