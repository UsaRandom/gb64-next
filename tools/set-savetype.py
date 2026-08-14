#!/usr/bin/env python3
"""Stamp a built gb64 image's save type: SVID setting plus ED header byte.

gb64 picks its save access path at boot from the SaveTypeSetting block the rom
wrapper embeds ('SVID' header, then a u32: 0 flash, 1 sram, 2 sram3x --
src/save.h). Flashcarts and ares provision the save hardware from the ED64
homebrew header byte at 0x3F instead, so the two must agree or the emulator
DMAs into hardware that is not there.

We stamp SaveTypeSRAM3X (96 KB banked SRAM, ED code 0x40): on the
M64 + EverGenesis64, the flash path read back nothing -- a valid save booted
as absent, and saves reported success and were gone on relaunch -- while SRAM
is a plain PI DMA with no osFlashInit chip-id handshake to go wrong, and it
skips the whole-chip erase every flash save pays for.

Recompute CRC1/CRC2 afterwards (romwrapper/crc.js recalcCRC); both patches
land inside the checksummed span.

Usage: set-savetype.py IN.z64 OUT.z64 [flash|sram|sram3x]   (default sram3x)
"""

import sys

SVID = b"SVID"
TYPES = {"flash": (0, 0x50), "sram": (1, 0x30), "sram3x": (2, 0x40)}

src, dst = sys.argv[1], sys.argv[2]
save_type, ed_byte = TYPES[sys.argv[3] if len(sys.argv) > 3 else "sram3x"]

d = bytearray(open(src, "rb").read())
i = d.find(SVID)
if i < 0:
    raise SystemExit(f"{src}: no SVID block; not a gb64 image")
if d.find(SVID, i + 4) != -1:
    raise SystemExit(f"{src}: multiple SVID blocks; refusing to guess")
if d[0x3C:0x3E] != b"ED":
    raise SystemExit(f"{src}: no ED homebrew header at 0x3C")
old = int.from_bytes(d[i + 4:i + 8], "big")
d[i + 4:i + 8] = save_type.to_bytes(4, "big")
old_ed = d[0x3F]
d[0x3F] = ed_byte
open(dst, "wb").write(bytes(d))
print(f"{dst}: SVID saveType {old} -> {save_type} at {i + 4:#x}, "
      f"ED 0x3F {old_ed:#04x} -> {ed_byte:#04x}; now fix CRC1/CRC2")
