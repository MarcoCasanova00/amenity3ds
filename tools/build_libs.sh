#!/usr/bin/env bash
# Build libctru and citro3d against the ARM GNU toolchain, without devkitPro's
# base_rules/3ds_rules make includes. Installs into $DEVKITPRO (libctru, citro3d).
set -euo pipefail

export DEVKITPRO="${DEVKITPRO:-$HOME/devkitpro}"
export DEVKITARM="${DEVKITARM:-$HOME/toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi}"
PREFIX="$DEVKITARM/bin/arm-none-eabi-"
CC="${PREFIX}gcc"
CXX="${PREFIX}g++"
AS="${PREFIX}gcc"
AR="${PREFIX}ar"
RANLIB="${PREFIX}ranlib"

ARCH="-march=armv6k -mtune=mpcore -mfloat-abi=hard -mfpu=vfp -mtp=soft"
CFLAGS_BASE="-g -O2 -Wall -fomit-frame-pointer -ffunction-sections -fdata-sections ${ARCH} -D__3DS__"
STUBINC=/tmp/opencode/stubinc
# -mword-relocations may not exist in vanilla arm-gnu-toolchain; conditionally add.
if ${CC} -mword-relocations -E -x c /dev/null >/dev/null 2>&1; then
  CFLAGS_BASE="${CFLAGS_BASE} -mword-relocations"
fi

# ---------------- libctru ----------------
run_ctru() {
  local SRC=source
  local OUT=out
  local EXCLUDE="romfs_dev.c archive_dev.c gdbhio_dev.c gdbhio.c 3dslink.c sslc.c"
  rm -rf "$OUT"; mkdir -p "$OUT"
  local i
  # All C sources (flat; libctru sources are in subdirs, compile by full path)
  local SRCS; SRCS=$(find "$SRC" -name '*.c' | grep -v '/soc/' | sort)
  for i in $SRCS; do
    local base; base=$(basename "$i")
    if echo " $EXCLUDE " | grep -q " $base "; then continue; fi
    local obj="$OUT/$(echo "$i" | sed 's#/#_#g; s#\.c$#.o#')"
    ${CC} ${CFLAGS_BASE} -I"$STUBINC" -Iinclude -I. -c "$i" -o "$obj"
  done
  # Assembly
  local ASMS; ASMS=$(find "$SRC" -name '*.s' | sort)
  for i in $ASMS; do
    local obj="$OUT/$(echo "$i" | sed 's#/#_#g; s#\.s$#.o#')"
    ${CC} ${CFLAGS_BASE} -I"$STUBINC" -Iinclude -I. -x assembler-with-cpp -c "$i" -o "$obj"
  done
  # C++ sources (allocator)
  local CPPS; CPPS=$(find "$SRC" -name '*.cpp' | sort)
  for i in $CPPS; do
    local obj="$OUT/$(echo "$i" | sed 's#/#_#g; s#\.cpp$#.o#')"
    ${CXX} ${CFLAGS_BASE} -fno-rtti -fno-exceptions -std=gnu++11 -I"$STUBINC" -Iinclude -I. -c "$i" -o "$obj"
  done
  # Embed default_font.bin as 'default_font_bin'
  cat > "$OUT/font_bin.c" <<EOF
__asm__(".section .rodata\n\t.align 4\n\t.global default_font_bin\n\t.type default_font_bin, %object\ndefault_font_bin:\n\t.incbin \"$PWD/data/default_font.bin\"\n\t.size default_font_bin, .-default_font_bin\n");
EOF
  ${CC} ${CFLAGS_BASE} -I"$STUBINC" -Iinclude -I. -c "$OUT/font_bin.c" -o "$OUT/font_bin.o"
  # devoptab_list stub
  ${CC} ${CFLAGS_BASE} -I"$STUBINC" -Iinclude -I. -c "$STUBINC/devoptab_stub.c" -o "$OUT/devoptab_stub.o"
  rm -f lib/libctru.a
  mkdir -p lib include
  ${AR} rcs lib/libctru.a "$OUT"/*.o
  ${RANLIB} lib/libctru.a
  echo "libctru.a built: $(du -h lib/libctru.a | cut -f1)"
}

( cd /tmp/opencode/libs/libctru/libctru && run_ctru )

# ---------------- citro3d ----------------
run_citro() {
  local SRC=source
  local OUT=out
  rm -rf "$OUT"; mkdir -p "$OUT"
  local i
  local SRCS; SRCS=$(find "$SRC" -name '*.c' | sort)
  for i in $SRCS; do
    local obj="$OUT/$(echo "$i" | sed 's#/#_#g; s#\.c$#.o#')"
    ${CC} ${CFLAGS_BASE} -DCITRO3D_BUILD -Iinclude -I/tmp/opencode/libs/libctru/libctru/include -c "$i" -o "$obj"
  done
  rm -f lib/libcitro3d.a
  mkdir -p lib
  ${AR} rcs lib/libcitro3d.a "$OUT"/*.o
  ${RANLIB} lib/libcitro3d.a
  echo "libcitro3d.a built: $(du -h lib/libcitro3d.a | cut -f1)"
}

( cd /tmp/opencode/libs/citro3d && run_citro )

# ---------------- install ----------------
mkdir -p "$DEVKITPRO/libctru/include" "$DEVKITPRO/libctru/lib" \
         "$DEVKITPRO/citro3d/include" "$DEVKITPRO/citro3d/lib"
cp -r /tmp/opencode/libs/libctru/libctru/include/* "$DEVKITPRO/libctru/include/"
cp /tmp/opencode/libs/libctru/libctru/lib/libctru.a "$DEVKITPRO/libctru/lib/"
cp -r /tmp/opencode/libs/citro3d/include/* "$DEVKITPRO/citro3d/include/"
cp /tmp/opencode/libs/citro3d/lib/libcitro3d.a "$DEVKITPRO/citro3d/lib/"

echo "Installed to $DEVKITPRO:"
ls -la "$DEVKITPRO/libctru/lib" "$DEVKITPRO/citro3d/lib"
