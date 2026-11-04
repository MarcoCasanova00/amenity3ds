# Makefile — GBA DnB break player (amdnb.gba)
DEVKITARM ?= $(HOME)/toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi
PREFIX := $(DEVKITARM)/bin/arm-none-eabi-
CC := $(PREFIX)gcc
LD := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

ARCH := -marm -mcpu=arm7tdmi -O2 -Wall -fno-builtin -std=gnu11

BREAKS := ../assets/breaks/CLN086.wav ../assets/breaks/CLN105.wav \
          ../assets/breaks/PRC095.wav ../assets/breaks/PRC098.wav

OBJS := build/crt0.o build/main.o build/samples.o

all: amdnb.gba

build/samples.c build/samples.h: ../tools/make_gba_bank.py $(BREAKS)
	@mkdir -p build
	python3 ../tools/make_gba_bank.py ../assets/breaks build/samples

build:
	@mkdir -p build

build/crt0.o: src/crt0.s | build
	$(CC) $(ARCH) -x assembler-with-cpp -c $< -o $@

build/main.o: src/main.c build/samples.h | build
	$(CC) $(ARCH) -Isrc -Ibuild -c $< -o $@

build/samples.o: build/samples.c | build
	$(CC) $(ARCH) -Isrc -Ibuild -c $< -o $@

build/amdnb.elf: $(OBJS) app.ld
	$(LD) $(ARCH) -nostartfiles -T app.ld -Wl,-Map,build/amdnb.map -o $@ $(OBJS)

amdnb.gba: build/amdnb.elf
	$(OBJCOPY) -O binary $< $@
	@ls -la $@

clean:
	rm -rf build *.gba

.PHONY: all clean
