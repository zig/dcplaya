SRCS := crt0.s main.c aica.c
OBJS := crt0.o main.o aica.o

all: stream.drv

aica_fw.h: stream.drv
	../../../utils/bin2c/bin2c stream.drv aica_fw.h.tmp
	echo 'unsigned char aica_fw[] = {' > aica_fw.h
	cat aica_fw.h.tmp >> aica_fw.h
	echo '};' >> aica_fw.h
	-rm aica_fw.h.tmp

%.drv: %.elf
	$(DC_ARM7OBJCOPY) -O binary $< $@

.INTERMEDIATE: stream.elf

stream.elf: $(OBJS)
	$(DC_ARM7CC) -Wl,-Ttext,0x00000000 -nostartfiles -nostdlib -e reset -o $@ $^ -lgcc

%.o: %.c
	$(DC_ARM7CC) -O2 $(DC_ARM7INCS) -c $< -o $@

%.o: %.s
	$(DC_ARM7AS) $< -o $@

clean:
	-rm -f *.o *.srec *.elf 1ST_READ.BIN prog.bin *.bck stream.drv aica_fw.h

depend-clean:
	@rm -f .depends
	@touch .depends

.PHONY: .depends
.depends:
	@makedepend -f$@ $(DC_ARM7INCS) $(filter %.c,$(SRCS)) && rm -f $@.bak

dep depend: .depends


required:
	@touch .depends
