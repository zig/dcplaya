################################
# Configuration

CC = gcc
CFLAGS = 
LDFLAGS = -lm
PREFIX = /usr/local
WANT_DEBUG=TRUE

# nothing below here should need to be changed

################################
# Acting on the configuration

NAME = nosefart
VERSION = 1.92c-mls

BUILDTOP = nsfobj
BUILDDIR = $(BUILDTOP)/build
SRCDIR = src

CFLAGS += -DNSF_PLAYER

ifeq "$(WANT_DEBUG)" "TRUE"
	CFLAGS += -ggdb
else
	CFLAGS += -O2 -fomit-frame-pointer -ffast-math -funroll-loops
	DEBUG_OBJECTS =
endif

CFLAGS +=\
 -I$(SRCDIR)\
 -I$(SRCDIR)/linux\
 -I$(SRCDIR)/sndhrdw\
 -I$(SRCDIR)/machine\
 -I$(SRCDIR)/cpu/nes6502\
 -I$(BUILDTOP)\
 -I/usr/local/include/

NSFINFO_CFLAGS = $(CFLAGS) -DNES6502_MEM_ACCESS_CTRL

################################
# Here's where the directory tree gets ugly

FILES =\
 log\
 memguard\
 cpu/nes6502/nes6502\
 cpu/nes6502/dis6502\
 machine/nsf\
 sndhrdw/nes_apu\
 sndhrdw/vrcvisnd\
 sndhrdw/fmopl\
 sndhrdw/vrc7_snd\
 sndhrdw/mmc5_snd\
 sndhrdw/fds_snd

SRCS = $(addsuffix .c, $(FILES) linux/main_linux)
SOURCES = $(addprefix $(SRCDIR)/, $(SRCS))
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))

NSFINFO_SRCS = $(addsuffix .c, $(FILES) nsfinfo/nsfinfo)
NSFINFO_SOURCES = $(addprefix $(SRCDIR)/, $(NSFINFO_SRCS))
NSFINFO_OBJECTS = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%-acc.o,$(NSFINFO_SOURCES))

ALL_OBJECTS = $(OBJECTS) $(NSFINFO_OBJECTS)

ALL_TARGETS = $(BUILDTOP)/$(NAME) $(BUILDTOP)/nsfinfo

################################
# Rules

all: $(ALL_TARGETS)

################################
# Support

$(BUILDDIR):
	mkdir -p $(sort $(dir $(ALL_OBJECTS)))
#	-mkdir -p $(BUILDDIR)/cpu/nes6502 $(BUILDDIR)/machine $(BUILDDIR)/sndhrdw $(BUILDDIR)/linux  $(BUILDDIR)/nsfinfo

$(BUILDTOP)/config.h: $(BUILDDIR) Makefile
	@echo "[$@]"
	@echo "#define VERSION \"$(VERSION)\"" > $@
	@echo "#define NAME \"$(NAME)\"" >> $@

$(BUILDDIR)/dep: $(BUILDTOP)/config.h
	$(CC) $(CFLAGS) -M $(SOURCES) > $@

-include $(BUILDDIR)/dep/

install: all
	mkdir -p $(PREFIX)/bin
	cp $(ALL_TARGETS) $(PREFIX)/bin
	@echo "-----------------------------------------------"
	@echo "Be sure to run chmod +s $(PREFIX)/bin/$(NAME) if you want ordinary users"
	@echo "to be able to use /dev/dsp.  SUID isn't necessaryx, though, if you want to"
	@echo "run $(NAME) with a wrapper, like artsdsp from arts or esddsp from esound."
	@echo "-----------------------------------------------"
	@echo "Also, make sure that $(PREFIX)/bin is in your PATH."

uninstall:
	rm -f $(PREFIX)/bin/$(NAME)
clean: 
	rm -rf nsfobj


################################
# The real heavy lifting

$(BUILDTOP)/$(NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILDTOP)/nsfinfo: $(NSFINFO_OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

$(BUILDDIR)/%-acc.o: $(SRCDIR)/%.c
	$(CC) $(NSFINFO_CFLAGS) -o $@ -c $<
