# Makefile — Amenity3DS (native 3DS homebrew, CIA output).

DEVKITPRO ?= $(HOME)/devkitpro
DEVKITARM ?= $(HOME)/toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi
PREFIX     := $(DEVKITARM)/bin/arm-none-eabi-

CC   := $(PREFIX)gcc
AR   := $(PREFIX)ar
LD   := $(PREFIX)gcc

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -mtp=soft

CFLAGS := -g -O2 -Wall -ffunction-sections -fdata-sections -std=gnu11 \
          $(ARCH) \
          -I$(DEVKITPRO)/libctru/include \
          -I$(DEVKITPRO)/citro3d/include \
          -Isource

LIBDIRS := -L$(DEVKITPRO)/libctru/lib -L$(DEVKITPRO)/citro3d/lib
LIBS    := -Wl,--start-group -lcitro3d -lctru -lstdc++ -lc -lm -lgcc -Wl,--end-group

OBJS := build/crt0.o \
        build/main.o \
        build/engine.o \
        build/ui.o \
        build/gfx.o \
        build/startup.o

MAKEROM := /tmp/opencode/libs/Project_CTR/makerom/bin/makerom
BANNERTOOL := /tmp/opencode/libs/bannertool/output/linux-x86_64/bannertool

TARGET := build/amenity3ds

all: $(TARGET).cia

build:
	@mkdir -p build

build/%.o: source/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build/%.o: source/%.s | build
	$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@

$(TARGET).elf: $(OBJS) source/app.ld
	$(LD) $(ARCH) -nostartfiles -T source/app.ld -Wl,--gc-sections \
		-Wl,-Map,$(TARGET).map -o $@ $(OBJS) $(LIBDIRS) $(LIBS)

build/banner.bin build/icon.bin: tools/make_assets.py
	python3 tools/make_assets.py
	$(BANNERTOOL) makebanner -i build/banner.png -a build/silence.wav -o build/banner.bin
	$(BANNERTOOL) makesmdh -s "Amenity3DS" -l "Amenity3DS" -p "Inception" -i build/icon.png -o build/icon.bin

$(TARGET).cia: $(TARGET).elf build/banner.bin build/icon.bin
	$(MAKEROM) -f cia -o $@ \
		-DAPP_TITLE="Amenity3DS" \
		-DAPP_PRODUCT_CODE=AMEN3DS \
		-DAPP_UNIQUE_ID=0xA3D5 \
		-rsf tools/rsf.cia \
		-target t -major 1 -minor 0 -micro 0 \
		-banner build/banner.bin -icon build/icon.bin \
		-elf $(TARGET).elf
	@echo "=== built: $@ ==="

clean:
	rm -rf build

.PHONY: all clean
