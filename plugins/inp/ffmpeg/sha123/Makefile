
LIBRARY=yes

include ../../../../Makefile.inc

NODEP_RULES := clean distclean

LIBNAME := $(shell basename "`pwd`")
TARGET := lib$(LIBNAME).a

SRCS := $(filter-out test.c mpg123.c,$(wildcard *.c))
OBJS := $(patsubst %.c,%.o,$(SRCS))
DEPS := $(addprefix .deps/,$(SRCS:%.c=%.d))

#KOS_LOCAL_CFLAGS += -I.. -DSHA123_DEBUG=0 -DBSI_NO_INLINE -O2 -g -Wall
KOS_LOCAL_CFLAGS += -I.. -DBSI_NO_INLINE -O3 -g -Wall
#CFLAGS += -I.. -USHA123_DEBUG -march=i686 -O6 -funroll-loops -ffast-math -fomit-frame-pointer -finline-functions

EXTRALIBS := ../istream/libistream.a

LDFLAGS += -lm

all : $(TARGET)

#$(TARGET) : $(OBJS)
#	ranlib $@

$(EXTRALIBS) :

test: $(TARGET) $(EXTRALIBS)

MP2ENCODE=test -e "mp2/test-$$frq-$$br-$$mode.mp2"\
 || nice -n +10 toolame\
 -s $$frq\
 -m $$mode\
 -b $$br\
 "test.wav" "mp2/test-$$frq-$$br-$$mode.mp2"

MP3ENCODE=test -e "mp3/test-$$frq-$$br-$$mode$$brsuffix.mp3"\
 || nice -n +10 lame -h\
 -s $$frq\
 -m $$mode\
 $$brmode $$br\
 "test.wav" "mp3/test-$$frq-$$br-$$mode$$brsuffix.mp3"\

.PHONY: mp2
mp2:
	@mkdir -p $@
	for mode in s d j m; do\
		for frq in 16 22.05 24; do\
			for br in 8 16 24 32 40 48 56 64\
				 80 96 112 128 144 160; do\
				$(MP2ENCODE);\
			done;\
		done;\
		for frq in 32 44.1 48; do\
			for br in 32 48 56 64 80 96 112 128\
				 160 192 224 256 320 384; do\
				$(MP2ENCODE);\
			done;\
		done;\
	done

.PHONY: mp3
mp3:
	@mkdir -p $@
	for brmode in -b --abr ; do\
		brsuffix=""; test "$$brmode" = "--abr" && brsuffix="-vbr";\
		for mode in s j m; do\
			for frq in 32  48  44.1; do\
				for br in 32 40 48 56 64 80 96 112 128 160\
					192 224 256 320; do\
					$(MP3ENCODE);\
				done;\
			done;\
			for frq in 16 24 22.05; do\
				for br in 8 16 24 32 40 48 56 64 80 96 112\
					128 144 160; do\
					$(MP3ENCODE);\
				done;\
			done;\
			for frq in 8 12 11.025; do\
				for br in 8 16 24 32 40 48 56 64 80 96 112\
					128 144 160; do\
					$(MP3ENCODE);\
				done;\
			done;\
		done;\
	done

testfiles: mp2 mp3

#clean:
#	rm -f $(TARGET) test makedepend.mkf *.o $(LIBNAME)
#	rm -rf .deps

#.deps:
#	@mkdir -p .deps

#.deps/%.d: %.c .deps
#	$(CC) $(CFLAGS) $< -M -MP > $@ 2> /dev/null || touch $@

makedepend.mkf: $(DEPS)
	cat -s /dev/null $^ > $@

#depend: makedepend.mkf

backup:
	(cd .. && tar czvf $(LIBNAME)-`date +%Y%m%d`.tgz  --backup=t \
	`find $(LIBNAME) -type f -name '*.[ch]' -o -name Makefile`);

#ifeq "$(filter $(NODEP_RULES),$(MAKECMDGOALS))" ""
#-include makedepend.mkf
#endif
