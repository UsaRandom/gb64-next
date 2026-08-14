#!/usr/bin/env python3
"""Stamp CRC1/CRC2 in an N64 ROM header: the part of `makemask` a boot needs.

CIC-6102 algorithm over the megabyte at 0x1000, seed 0xF8CA4DDC. Verified
against the shipped gb64 template, whose known-good pair is
17a392d8/d20c12be. IPL3 recomputes this at boot on a real cart; flashcart
menus skip the check, but a stamped header is correct everywhere.

Usage: n64crc.py ROM.z64   (modifies in place, prints old -> new)
"""

import struct
import sys

M = 0xFFFFFFFF


def rol(v, n):
    n &= 31
    return ((v << n) | (v >> (32 - n))) & M


def crc6102(rom: bytes):
    seed = 0xF8CA4DDC
    t1 = t2 = t3 = t4 = t5 = t6 = seed
    for (d,) in struct.iter_unpack(">I", rom[0x1000:0x101000]):
        if (t6 + d) & M < t6:
            t4 = (t4 + 1) & M
        t6 = (t6 + d) & M
        t3 ^= d
        r = rol(d, d & 0x1F)
        t5 = (t5 + r) & M
        if t2 > d:
            t2 ^= r
        else:
            t2 ^= t6 ^ d
        t1 = (t1 + (t5 ^ d)) & M
    return (t6 ^ t4 ^ t3) & M, (t5 ^ t2 ^ t1) & M


if __name__ == "__main__":
    path = sys.argv[1]
    rom = bytearray(open(path, "rb").read())
    if len(rom) < 0x101000:
        # The checksum window runs to 0x101000; makemask's other job was
        # filling the ROM out so that window reads deterministic bytes.
        rom.extend(b"\xFF" * (0x101000 - len(rom)))
    old = rom[0x10:0x18].hex()
    c1, c2 = crc6102(bytes(rom))
    rom[0x10:0x18] = struct.pack(">II", c1, c2)
    open(path, "wb").write(bytes(rom))
    print(f"{path}: CRC {old} -> {c1:08x}{c2:08x}")
