#!/bin/sh -e
# Build gb64 natively on macOS: libdragon's mips64-elf toolchain, the crashsdk
# headers/libraries out of the ModernN64SDKArchives mirror, and the carved
# prebuilt RSP ucode in prebuilt/ (see README "Building on macOS").
#
# Layout this defaults to, all overridable by environment:
#   ../sdkmod-mirror   sparse clone of ModernN64SDKArchives/n64sdkmod
#                      (packages/n64sdk + packages/n64sdk-common checked out)
#   ../tools-n64       rsp2dwarf binary (go install github.com/lambertjamesd/rsp2dwarf)
#   $HOME/n64inst-preview/bin/mips64-elf-   the cross toolchain

cd "$(dirname "$0")/.."

TOOLCHAIN=${TOOLCHAIN:-$HOME/n64inst-preview/bin/mips64-elf-}
MIRROR=${MIRROR:-../sdkmod-mirror/packages}
SDKINC=${SDKINC:-$MIRROR/n64sdk/usr/include/n64}
SDKLIB=${SDKLIB:-$MIRROR/n64sdk/usr/lib/n64}
SDKPR=${SDKPR:-$SDKLIB/PR}
RSP2DWARF=${RSP2DWARF:-../tools-n64/rsp2dwarf}
LIBGCCDIR=$(dirname "$("${TOOLCHAIN}gcc" -mabi=32 -print-libgcc-file-name)")

# The ucode is prebuilt: rspasm exists only as a Linux binary, and none of the
# fork's changes touch rsp/. Regenerate ppu.o from the carved images and mark
# everything newer than rsp/*.s so make never reaches for rspasm. If you edit
# rsp/ppu.s this goes red on purpose: delete prebuilt/ and bring a real rspasm.
mkdir -p bin/rsp build
cp prebuilt/ppu prebuilt/ppu.dat bin/rsp/
# rspasm would have written this; only its optional debug "line" records are
# read, so carved ucode gets an empty one.
: > bin/rsp/ppu.sym

# asm/data.s (the non-placeholder variant) is assembled by S_FILES even though
# a placeholder build never links it, and it incbins a dev-only GB rom no
# clone carries. An empty stand-in keeps the assembler satisfied.
[ -f data/PokemonYellow.gb ] || : > data/PokemonYellow.gb
# No -g variant: carved ucode has no line records for it, and nothing links
# ppu.debug.o -- it exists for the serial debugger workflow only.
"$RSP2DWARF" bin/rsp/ppu -o bin/rsp/ppu.o -n ppu
touch bin/rsp/ppu bin/rsp/ppu.dat bin/rsp/ppu.o

# DImode libgcc helpers this toolchain's default-ABI libgcc lacks; see the
# comment atop tools/di3helpers.c.
"${TOOLCHAIN}gcc" -c -mabi=32 -march=vr4300 -mfix4300 -ffreestanding -G 0 -O2 \
    -o build/di3helpers.o tools/di3helpers.c

make ROOT=tools/macsdk FINAL=YES \
    TOOLCHAIN="$TOOLCHAIN" \
    CPP_LD="${TOOLCHAIN}cpp -Umips" \
    RSP2DWARF="$RSP2DWARF" \
    BOOT=prebuilt/boot.6102 \
    MAKEMASK="python3 tools/n64crc.py" \
    LCINCS="-I. -I$SDKINC/PR -I$SDKINC" \
    LCDEFS="-D_FINALROM -DNDEBUG -DF3DEX_GBI_2 -DSDK_PR=$SDKPR -DUSE_PLACEHOLDER=1" \
    OPTIMIZER="-g -O2 -std=gnu99 -Werror=implicit-function-declaration" \
    LDFLAGS="-L$SDKLIB -lultra_rom -L$LIBGCCDIR -lgcc build/di3helpers.o" \
    "$@"
