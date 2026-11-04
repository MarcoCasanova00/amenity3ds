#!/usr/bin/env python3
"""Convert an ARM ELF (as produced by our app link) into a .3dsx homebrew
file for 3DS emulators (Lime3DS / Citra) and loader-based testing.

3DSX format (from libctru's 3dsx.h):
  0x00 u32 magic '3DSX'
  0x04 u16 version, u16 flags
  0x08 u32 rom_size (0)
  0x0C u32 code_size
  0x10 u32 data_size
  0x14 u32 bss_size
  0x18 u32 total_size (header + code + data)
  0x1C u64 text_addr
  0x24 u64 data_addr
  0x2C u64 bss_addr
  then code, then data.
"""
import struct, sys, os

def read_elf(path):
    with open(path, 'rb') as f:
        data = f.read()
    if data[:4] != b'\x7fELF':
        raise SystemExit('not an ELF')
    endian = '<' if data[5] == 1 else '>'
    if data[4] == 2:  # ELF64
        (e_phoff,) = struct.unpack_from(endian + 'Q', data, 0x20)
        (e_phentsize,) = struct.unpack_from(endian + 'H', data, 0x36)
        (e_phnum,) = struct.unpack_from(endian + 'H', data, 0x38)
        def seg(off):
            p_type, p_flags = struct.unpack_from(endian + 'II', data, off)
            p_offset, p_vaddr = struct.unpack_from(endian + 'QQ', data, off + 8)
            p_filesz, p_memsz = struct.unpack_from(endian + 'QQ', data, off + 32)
            return p_type, p_flags, p_offset, p_vaddr, p_filesz, p_memsz
        esz, eoff, efld = e_phentsize, e_phoff, e_phnum
    else:  # ELF32
        (e_phoff,) = struct.unpack_from(endian + 'I', data, 0x1C)
        (e_phentsize,) = struct.unpack_from(endian + 'H', data, 0x2A)
        (e_phnum,) = struct.unpack_from(endian + 'H', data, 0x2C)
        def seg(off):
            p_type = struct.unpack_from(endian + 'I', data, off)[0]
            p_offset = struct.unpack_from(endian + 'I', data, off + 4)[0]
            p_vaddr = struct.unpack_from(endian + 'I', data, off + 8)[0]
            p_filesz = struct.unpack_from(endian + 'I', data, off + 16)[0]
            p_memsz = struct.unpack_from(endian + 'I', data, off + 20)[0]
            p_flags = struct.unpack_from(endian + 'I', data, off + 24)[0]
            return p_type, p_flags, p_offset, p_vaddr, p_filesz, p_memsz
        esz, eoff, efld = e_phentsize, e_phoff, e_phnum
    segs = []
    for i in range(efld):
        off = eoff + i * esz
        p_type, p_flags, p_offset, p_vaddr, p_filesz, p_memsz = seg(off)
        if p_type == 1:  # PT_LOAD
            segs.append((p_vaddr, p_flags, p_offset, p_filesz, p_memsz))
    segs.sort()
    return segs, data

def main():
    if len(sys.argv) < 3:
        print('usage: elf2_3dsx.py input.elf output.3dsx')
        sys.exit(1)
    segs, raw = read_elf(sys.argv[1])

    text = b''
    text_addr = 0
    data = b''
    data_addr = 0
    bss = 0
    bss_addr = 0

    for vaddr, flags, off, fsz, msz in segs:
        blob = raw[off:off + fsz]
        if flags & 1:            # executable -> code
            text += blob
            if not text_addr:
                text_addr = vaddr
        elif flags & 2:         # writable -> data + bss
            data += blob
            if not data_addr:
                data_addr = vaddr
            bss = msz - fsz
            bss_addr = vaddr + fsz
        else:                   # read-only (rodata) -> append to code
            text += blob

    header = struct.pack('<IHHIIIIIQQQ', 0x58534433, 0, 0, 0,
                         len(text), len(data), bss,
                         52 + len(text) + len(data),
                         text_addr, data_addr, bss_addr)
    with open(sys.argv[2], 'wb') as f:
        f.write(header)
        f.write(text)
        f.write(data)
    print('%s: text=%d data=%d bss=%d (text@0x%x data@0x%x bss@0x%x)'
          % (sys.argv[2], len(text), len(data), bss, text_addr, data_addr, bss_addr))

if __name__ == '__main__':
    main()
