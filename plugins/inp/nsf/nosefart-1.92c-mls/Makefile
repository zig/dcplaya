################################
# Configuration

CC = gcc
CFLAGS = 
LDFLAGS =
PREFIX = /usr/local
# WANT_DEBUG = TRUE

# nothing below here should need to be changed

################################
# Acting on the configuration

NAME = nosefart
VERSION = 1.92c-mls

BUILDDIR = nsfobj
SRCDIR = src


ifeq "$(WANT_DEBUG)" "TRUE"
	CFLAGS += -ggdb -DNSF_PLAYER
else
	CFLAGS += -O2 -fomit-frame-pointer -ffast-math -funroll-loops -DNSF_PLAYER
	DEBUG_OBJECTS =
endif

CFLAGS += -I$(SRCDIR)\
	-I$(SRCDIR)/linux\
	-I$(SRCDIR)/sndhrdw\
	-I$(SRCDIR)/machine\
	-I$(SRCDIR)/cpu/nes6502\
	-I$(BUILDDIR)\
	-I/usr/local/include/\
	-lm

################################
# Here's where the directory tree gets ugly

FILES = log\
	memguard\
	cpu/nes6502/nes6502\
	cpu/nes6502/dis6502\
	machine/nsf\
	sndhrdw/nes_apu\
	sndhrdw/vrcvisnd\
	sndhrdw/fmopl\
	sndhrdw/vrc7_snd\
	sndhrdw/mmc5_snd\
	sndhrdw/fds_snd\
	linux/main_linux

SRCS = $(addsuffix .c, $(FILES))
SOURCES = $(addprefix $(SRCDIR)/, $(SRCS))
OBJS = $(addsuffix .o, $(FILES))
OBJECTS = $(addprefix $(BUILDDIR)/, $(OBJS))

################################
# Rules

all: $(BUILDDIR)/$(NAME)

################################
# Support

$(BUILDDIR):
	-mkdir -p $(BUILDDIR)/cpu/nes6502 $(BUILDDIR)/machine $(BUILDDIR)/sndhrdw $(BUILDDIR)/linux

$(BUILDDIR)/config.h: $(BUILDDIR) Makefile
	echo "#define VERSION \"$(VERSION)\"" > $(BUILDDIR)/config.h
	echo "#define NAME \"$(NAME)\"" >> $(BUILDDIR)/config.h

$(BUILDDIR)/dep: $(BUILDDIR)/config.h
	$(CC) $(CFLAGS) -M $(SOURCES) > $@

include $(BUILDDIR)/dep/

install: $(BUILDDIR)/$(NAME)
	mkdir -p $(PREFIX)/bin
	cp $(BUILDDIR)/$(NAME) $(PREFIX)/bin
	@echo "-----------------------------------------------"
	@echo "Be sure to run chmod +s $(PREFIX)/bin/$(NAME) if you want ordinary users"
	@echo "to be able to use /dev/dsp.  SUID isn't necessary, though, if you want to"
	@echo "run $(NAME) with a wrapper, like artsdsp from arts or esddsp from esound."
	@echo "-----------------------------------------------"
	@echo "Also, make sure that $(PREFIX)/bin is in your PATH."
	
uninstall:
	rm -f $(PREFIX)/bin/$(NAME)
clean: 
	rm -rf nsfobj


################################
# The real heavy lifting

$(BUILDDIR)/$(NAME): $(OBJECTS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $(OBJECTS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<
