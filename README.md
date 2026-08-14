# gb64-next

A Game Boy / Game Boy Color emulator that runs on N64 hardware, forked from
lambertjamesd's gb64. This fork exists to make the emulator behave on the
ModRetro M64 with a SummerCart64, and it changes three things:

* **Controls work on the M64.** Upstream never started the controller read
  when every port enrolled a controller, which no stock N64 with fewer than
  four pads can hit -- so on consoles whose PIF answers on all four channels,
  every button read as zero forever. The read is now started unconditionally.
* **In-game saving works on a SummerCart64.** Saves use banked SRAM
  (SaveTypeSRAM3X) instead of flash, and the emulator commits cart RAM to the
  cartridge the moment the game closes its SRAM write window -- the commit at
  the end of every battery save -- so saving in-game just works, with no
  save-button ritual. The save button and its save states remain.
* **Real-time clock support.** On a SummerCart64, games with an MBC3 timer
  keep time from the cart's battery-backed RTC: the emulated clock advances
  by exactly the time the console spent powered off.
  The counter itself stays game-owned -- clock writes the game makes are
  honored -- and on any other cartridge or emulator the original stored-timer
  behaviour is untouched.

## Building on macOS

`tools/macbuild.sh` builds `bin/gb.z64` natively -- no Linux, no container.
It expects:

* the libdragon toolchain's `mips64-elf-` binaries (default
  `$HOME/n64inst-preview/bin/`, override with `TOOLCHAIN=`)
* a sparse clone of the `ModernN64SDKArchives/n64sdkmod` repository as
  `../sdkmod-mirror` with `packages/n64sdk` and `packages/n64sdk-common`
  checked out (libultra headers and library, F3DEX2/rspboot ucode objects)
* `rsp2dwarf` built from lambertjamesd's repository of the same name:
  `GOBIN=$(pwd)/../tools-n64 go install github.com/lambertjamesd/rsp2dwarf@latest`
* `brew install rgbds` (assembles the placeholder boot ROM stubs)
* `prebuilt/boot.6102`: the 0xFC0-byte CIC-6102 IPL3, copied from offset 0x40
  of any 6102 ROM or from the SDK's `PR/bootcode/boot.6102` (they are
  byte-identical). Not committed; it is Nintendo's code.

The RSP ucode is linked from `prebuilt/ppu` + `prebuilt/ppu.dat`, carved from
the shipped upstream build -- `rspasm` exists only as a Linux binary. If you
change `rsp/ppu.s` the build fails on purpose until you reassemble those with
a real rspasm and refresh the carve.

## Packaging a ROM

The build output is a placeholder image. Splice a Game Boy ROM over the
placeholder with the wrapper page in `romwrapper/` (open
`romwrapper.html` locally) or with MainMenu's `tools/mkgb64.js`, then set the
save type with `tools/set-savetype.py` (`sram3x` for the SummerCart64 setup
this fork targets) and restamp the header checksum with `tools/n64crc.py`.

## Building the original way (Linux, crashsdk)

The stock `Makefile` still works against an installed modern SDK
(`make FINAL=YES`). The optional GB/GBC boot ROMs go in `data/dmg_boot.bin`
and `data/cgb_bios.bin`; without them the placeholder stubs are used.
