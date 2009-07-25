
LIBRARY=yes

include ../../../../Makefile.inc

NODEP_RULES := clean distclean

LIBNAME := $(shell basename "`pwd`")
TARGET := lib$(LIBNAME).a

SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))
DEPS := $(addprefix .deps/,$(SRCS:%.c=%.d))

KOS_LOCAL_CFLAGS += -I.. -O -g -Wall
LDFLAGS = -L. -l$(LIBNAME) -lm

all : $(TARGET)

#$(TARGET) : $(OBJS)
#	ranlib $@

#clean:
#	rm -f $(TARGET) makedepend.mkf *.o
#	rm -rf .deps

#.deps:
#	@mkdir -p .deps

#.deps/%.d: %.c .deps
#	$(CC) $(CFLAGS) $< -M -MP > $@  2> /dev/null || touch $@

#makedepend.mkf: $(DEPS)
#	cat -s /dev/null $^ > $@

#depend: makedepend.mkf

backup:
	(cd .. && tar czvf $(LIBNAME)-`date +%Y%m%d`.tgz  --backup=t \
	`find $(LIBNAME) -type f -name '*.[ch]' -o -name Makefile`);

#ifeq "$(filter $(NODEP_RULES),$(MAKECMDGOALS))" ""
#-include makedepend.mkf
#endif
