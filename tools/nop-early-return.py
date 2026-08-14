#!/usr/bin/env python3
"""Apply the controller.c early-return fix to an already-built gb64 image.

controller.c's initControllers() used to return before osContStartReadData()
whenever every port enrolled a controller, which leaves ReadController()'s
non-blocking receive with no message ever arriving -- input reads as all
zeroes, forever. The source fix is in this tree; this script is the interim
for images we cannot rebuild (no libultra toolchain on this machine): it NOPs
the early-return branch so the loop falls through to the read start.

Offsets are for the romwrapper gb.n64 template as of upstream a5ce8da
(big-endian image, md5 bfb3a9fdaea330c898a468eda6891f09 after mkgb64
conversion): file 0x1788 holds `beq t3,s2,0x17d4`, encoded 11 72 00 12.
The assert makes any other build refuse loudly rather than patch garbage.

Recompute CRC1/CRC2 afterwards (romwrapper/crc.js recalcCRC); the byte lands
inside the CIC-6102 checksum span. A menu-side boot never verifies it, but a
console boot of the bare image would.

Usage: nop-early-return.py IN.z64 OUT.z64
"""

import sys

OFFSET = 0x1788
EXPECT = bytes.fromhex("11720012")  # beq t3,s2,+0x12 words

src, dst = sys.argv[1], sys.argv[2]
d = bytearray(open(src, "rb").read())
found = d[OFFSET:OFFSET + 4]
if found != EXPECT:
    raise SystemExit(f"{src}: expected {EXPECT.hex()} at {OFFSET:#x}, "
                     f"found {found.hex()} -- different build, not patching")
d[OFFSET:OFFSET + 4] = b"\x00\x00\x00\x00"
open(dst, "wb").write(bytes(d))
print(f"{dst}: early-return beq at {OFFSET:#x} -> nop; now fix CRC1/CRC2")
