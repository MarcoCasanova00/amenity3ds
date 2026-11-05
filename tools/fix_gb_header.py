#!/usr/bin/env python3
"""Patch a Game Boy cartridge header: title + header checksum + global checksum.

Header checksum (0x14D) is chosen so that sum(0x134..0x14D) & 0xFF == 0.
Global checksum (0x14E..0x14F, little endian) is the 16-bit sum of every
byte in the ROM except the two checksum bytes themselves.

Usage: fix_gb_header.py <rom.gb> [title]
"""
import sys

path = sys.argv[1]
title = (sys.argv[2] if len(sys.argv) > 2 else "AMENITY-DNB").encode("ascii", "replace")[:16]

d = bytearray(open(path, "rb").read())
if len(d) < 0x150:
    raise SystemExit("ROM too small")

d[0x134:0x144] = title.ljust(16, b"\x00")

# Header checksum: sum(0x134..0x14D) must be 0 mod 256
d[0x14D] = (-sum(d[0x134:0x14D])) & 0xFF

# Global checksum: sum of all bytes except 0x14E and 0x14F
total = sum(d[:0x14E]) + sum(d[0x150:])
d[0x14E] = total & 0xFF
d[0x14F] = (total >> 8) & 0xFF

open(path, "wb").write(d)
print("fixed GB header: title=%r chk=%02X global=%04X" % (
    bytes(d[0x134:0x144]), d[0x14D], total & 0xFFFF))
