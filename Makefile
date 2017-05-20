export PATH := /u/wbcowan/gnuarm-4.0.2/libexec/gcc/arm-elf/4.0.2:$(PATH)
export PATH := /u/wbcowan/gnuarm-4.0.2/arm-elf/bin:$(PATH)

XCC     = gcc
AS	= as
LD      = ld
CFLAGS  := -c -fPIC -Wall -Werror -mcpu=arm920t -msoft-float --std=gnu99
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fPIC: emit position-independent code
# -Wall: report all warnings

ifeq ($(STRACE_ENABLED),1)
CFLAGS := $(CFLAGS) -DSTRACE_ENABLED
endif

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -init main -Map build/kernel.map -N -T orex.ld -L/u/wbcowan/gnuarm-4.0.2/lib/gcc/arm-elf/4.0.2

SRC = $(wildcard src/*/*.c)
ASM = $(wildcard src/*/*.s)
OBJ = $(patsubst src/%.c, build/%.o, $(SRC)) $(patsubst src/%.s, build/%.o, $(ASM))

.PHONY: all builddir clean upload

# Prevent make from deleting intermediate files.
.PRECIOUS: build/%.o build/%.s build/kernel/%.s

all: build/kernel.elf

# Kernel code includes headers from ./include
build/kernel/%.s: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(XCC) -S $(CFLAGS) -I./include -o $@ $<

# User code includes headers from ./include/user
build/%.s: src/%.c
	@mkdir -p $(dir $@)
	$(XCC) -S $(CFLAGS) -I./include/user -o $@ $<

build/%.o: build/%.s
	$(AS) $(ASFLAGS) -o $@ $<

build/%.o: src/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

build/kernel.elf: $(OBJ) orex.ld
	$(XCC) -S $(CFLAGS) -o build/kernel/buildstr.s src/kernel/buildstr.c
	$(AS) $(ASFLAGS) -o build/kernel/buildstr.o build/kernel/buildstr.s
	$(LD) $(LDFLAGS) -o build/kernel.elf $(OBJ) -lgcc

clean:
	-rm -rf build/*

upload: build/kernel.elf
	cp build/kernel.elf /u/cs452/tftp/ARM/$(USER)/coldwell.elf
