export PATH := /u3/rj2olear/gcc-arm-eabi-6/bin:$(PATH)

XCC     := arm-none-eabi-g++
AS      := arm-none-eabi-as
LD      := arm-none-eabi-g++
OBJDUMP := arm-none-eabi-objdump
CFLAGS  := -c -fPIC -Wall -Werror -mcpu=arm920t -msoft-float --std=gnu++14 -nostdlib -nostartfiles -ffreestanding -fno-exceptions -fno-unwind-tables -fno-rtti -fno-threadsafe-statics
# -g: include hooks for gdb
# -c: only compile
# -mcpu=arm920t: generate code for the 920t architecture
# -fPIC: emit position-independent code
# -Wall: report all warnings

ifeq ($(STRACE_ENABLED),1)
CFLAGS := $(CFLAGS) -DSTRACE_ENABLED
endif

ifeq ($(PERF_TEST),1)
CFLAGS := $(CFLAGS) -DPERF_TEST
endif

ifeq ($(CACHE_ENABLED),1)
CFLAGS := $(CFLAGS) -DCACHE_ENABLED
endif

ASFLAGS	= -mcpu=arm920t -mapcs-32
# -mapcs: always generate a complete stack frame

LDFLAGS = -Wl,-init,main,-Map=build/kernel.map,-N -T orex.ld -nostdlib -nostartfiles -ffreestanding -L/u3/rj2olear/gcc-arm-eabi-6/lib/gcc/arm-none-eabi/6.3.1/

ifeq ($(OPT_ENABLED),1)
CFLAGS := $(CFLAGS) -O2 -flto -DOPT_ENABLED
LDFLAGS := $(LDFLAGS) -O2 -flto
endif

SRC = $(wildcard src/*/*.cpp)
ASM = $(wildcard src/*/*.s)
OBJ = $(patsubst src/%.cpp, build/%.o, $(SRC)) $(patsubst src/%.s, build/%.o, $(ASM))

.PHONY: all builddir clean upload objdump

# Prevent make from deleting intermediate files.
.PRECIOUS: build/%.o

all: build/kernel.elf

# Kernel code includes headers from ./include
build/kernel/%.o: src/kernel/%.cpp
	@mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) -I./include -o $@ $<

# User code includes headers from ./include/user
build/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(XCC) $(CFLAGS) -I./include/user -o $@ $<

build/%.o: src/%.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -o $@ $<

build/kernel.elf: $(OBJ) orex.ld
	$(XCC) $(CFLAGS) -o build/kernel/buildstr.o src/kernel/buildstr.cpp
	$(LD) $(LDFLAGS) -o build/kernel.elf $(OBJ) -lgcc

objdump: build/kernel.elf
	$(OBJDUMP) -dC build/kernel.elf | less

clean:
	-rm -rf build/*

upload: build/kernel.elf
	cp build/kernel.elf /u/cs452/tftp/ARM/$(USER)/coldwell.elf

cupload: clean build/kernel.elf
	cp build/kernel.elf /u/cs452/tftp/ARM/$(USER)/coldwell.elf
