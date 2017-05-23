export PATH := /u3/rj2olear/gcc-arm-eabi-6/bin:$(PATH)

XCC     := arm-none-eabi-g++
AS      := arm-none-eabi-as
LD      := arm-none-eabi-g++
CFLAGS  := -c -fPIC -Wall -Werror -mcpu=arm920t -msoft-float --std=gnu++14 -nostdlib -nostartfiles -ffreestanding -fno-exceptions -fno-unwind-tables -fno-rtti -fno-threadsafe-statics
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

LDFLAGS = -Wl,-init,main,-Map=build/kernel.map,-N -T orex.ld -nostdlib -nostartfiles -ffreestanding -L/u3/rj2olear/gcc-arm-eabi-6/lib/gcc/arm-none-eabi/6.3.1/

ifeq ($(OPT_ENABLED),1)
CFLAGS := $(CFLAGS) -O2 -flto
LDFLAGS := $(LDFLAGS) -O2 -flto
endif

SRC = $(wildcard src/*/*.c)
ASM = $(wildcard src/*/*.s)
OBJ = $(patsubst src/%.c, build/%.o, $(SRC)) $(patsubst src/%.s, build/%.o, $(ASM))

.PHONY: all builddir clean upload

# Prevent make from deleting intermediate files.
.PRECIOUS: build/%.o build/%.s build/kernel/%.s

all: build/kernel.elf

# Kernel code includes headers from ./include
build/kernel/%.o: src/kernel/%.c
	@mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) -I./include -o $@ $<

# User code includes headers from ./include/user
build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) -I./include/user -o $@ $<

build/%.o: src/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

build/kernel.elf: $(OBJ) orex.ld
	$(XCC) $(CFLAGS) -o build/kernel/buildstr.o src/kernel/buildstr.c
	$(LD) $(LDFLAGS) -o build/kernel.elf $(OBJ) -lgcc

clean:
	-rm -rf build/*

upload: build/kernel.elf
	cp build/kernel.elf /u/cs452/tftp/ARM/$(USER)/coldwell.elf
