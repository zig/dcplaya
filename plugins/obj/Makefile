#
# Make 3D objects plugin for dreammp3
#
# (C) COPYRIGHT Ben(jamin) Gerard <ben@sasahipa.com>
#
# $Id: Makefile,v 1.1 2002-08-26 14:14:50 ben Exp $
#

UTILS=../../utils
ASE2C=$(UTILS)/ase2c.sh

LIBRARY=yes

ASE := $(sort $(wildcard ase/*.ase))
SRC := $(ASE:ase/%.ase=src/%.c)
#LEF := $(SRC:src/%.c=%.lef)
OBJS:= $(ASE:ase/%.ase=%.o)
NUDE:= $(ASE:ase/%.ase=%)
NEXT:= $(filter-out $(firstword $(NUDE)), $(NUDE)) 1
MIXED:=$(join $(addsuffix :,$(NUDE)),$(NEXT))
FIRST:=$(firstword $(MIXED))

LEFS = $(shell basename `pwd`).lef

ifdef NEXT_DRIVER
KOS_LOCAL_CFLAGS := -DNXT_DRIVER="$(NEXT_DRIVER)" -DFIRST_DRIVER="$(FIRST_DRIVER)"
endif

VPATH = src

OPTIMIZE = $(OPTIMIZE_FOR_SIZE)
KOS_LOCAL_INCS := -I../../include
CLEAN_LOCAL := *.lef src/*


myall : all

TARGETS= special $(LEFS) 

export FIRST_DRIVER NEXT_DRIVER

special: $(SRC)
	for dir in $(MIXED); do \
		file="`expr "$$dir" : '\(.*\):.*'`"; \
		next="`expr "$$dir" : '.*:\(.*\)'`"; \
		FIRST_DRIVER=`expr "$(MIXED) " : "$$dir .*"` NEXT_DRIVER="$$next" $(MAKE) "$$file.o"; \
	done;

.SECONDARY: $(SRC)
src/%.c : ase/%.ase $(ASE2C) $(UTILS)/mkobjlinks.c
	@CC=gcc CFLAGS="-I. -I../../include" $(ASE2C) $< >$@

include ../../Makefile.inc
# DO NOT DELETE
