#
# dcplaya top level Makefile
#
# (C) COPYRIGHT 2002 benjamin gerard <ben@sashipa.com>
#
# $Id: Makefile,v 1.22 2003-03-09 01:00:14 ben Exp $ 
#
TARGETS=dreammp3.elf

SUBDIRS = arm plugins src data libs dynshell

BUILTIN_DRIVERS=./plugins/img/tga/tga.lef
WHOLE_LIBS=-lz,-ltranslator,-ldraw,-ldreammp3,-llua,-ldcutils,-lkallisti
OPT_LIBS= -los -lgcc -lm
ELF_EXTRA += -L./src -L./libs/z -L./libs/draw -L./libs/translator -L./libs/lua -L$(KOS_BASE)/lib\
 -Wl,--whole-archive,$(WHOLE_LIBS),--no-whole-archive\
 $(BUILTIN_DRIVERS) $(OPT_LIBS)

#ELF_EXTRA +=  -shared -L./src -ldreammp3 -los
#ELF_EXTRA +=  `find src -type f -name  '*.o'` -los

KOS_INCS+= -I. -Iinclude
ALL_INCS= -I$(KOS_INCS) -I$(KOS_BASE)/kernel/arch/$(KOS_ARCH)/include

CLEAN_LOCAL=symtab.h tmp_symtab.h full-symb-dreammp3.elf debug.log


my_all: all TODO

symtab.o: symtab.h 
.PHONY: symtab.h
symtab.h :
	@echo "Build [$@]" 
	@[ -e $@ ] || touch $@
	@utils/makesymb.sh $(TARGETS) > tmp_$@
	@diff tmp_$@ $@ > /dev/null || mv -fv tmp_$@ $@
	@rm -f tmp_$@

force_$(TARGETS): symtab.o main.o force_math.o data/romdisk.o
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
	@$(KOS_STRIP) -v $@		

send:
	$(MAKEDATA)
#	@clear
	@dc-tool -x $(TARGETS) | tee debug.log

r: send

run: my_all send

backup: maintainer-clean
	N="`basename $(TOP_DIR)`"; tar cjvf ../$${N}-"`date -I`".tar.bz2 -C .. "$${N}" 

.PHONY: doc
doc:
	@$(MAKE) -s -C doc	


.PHONY: TODO
TODO:
	@echo "[Build $@]"
	@utils/shasubst.sh -D "d=[`date '+%Y-%m-%d %r'`, `id -un`]"\
	<$@ >$@.$$$$ && \
	diff -q $@ $@.$$$$; \
	if test $$? -ne 0; then mv -f $@.$$$$ $@; else rm -f $@.$$$$; fi;

commit: TODO
	cvs commit 2>&1 | tee - cvs.log 

update: TODO
	cvs update -d 2>&1 | tee - cvs.log

DEPEND_EXTRA=depend_extra
depend_extra:
	@touch "symtab.h"

# ----------------------------------------------------------------------

include Makefile.inc

ifeq '$(MASTER)' '1'
MAKEDATA=$(MAKE) -C make-cd dcplaya-data
else
MAKEDATA=@echo "NO make data"
endif
