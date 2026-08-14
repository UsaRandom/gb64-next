A gameboy emulator meant to run on N64 hardware.

If you just want to use the emulator go here
[https://lambertjamesd.github.io/gb64/romwrapper/romwrapper.html](https://lambertjamesd.github.io/gb64/romwrapper/romwrapper.html)

To build, you will need to locate the original gameboy's boot rom and save it to a file named `data/dmg_boot.bin`. As well as `data/cgb_bios.bin` for the gameboy colors.
You should be able to find both of these files here
[https://gbdev.gg8.se/files/roms/bootroms/](https://gbdev.gg8.se/files/roms/bootroms/)

You will also need to supply the gameboy rom you want to build into an N64 rom. You should put the gameboy rom into the `data` folder
and update the include path in [spec](./spec) under the `gbrom` section
## Building on macOS

`tools/macbuild.sh` builds `bin/gb.z64` natively -- no Linux, no container.
It expects:

* the [libdragon toolchain](https://github.com/DragonMinded/libdragon)'s
  `mips64-elf-` binaries (default `$HOME/n64inst-preview/bin/`, override with
  `TOOLCHAIN=`)
* a sparse clone of
  [ModernN64SDKArchives/n64sdkmod](https://github.com/ModernN64SDKArchives/n64sdkmod)
  as `../sdkmod-mirror` with `packages/n64sdk` and `packages/n64sdk-common`
  checked out (libultra headers and library, F3DEX2/rspboot ucode objects)
* `rsp2dwarf`: `GOBIN=$(pwd)/../tools-n64 go install github.com/lambertjamesd/rsp2dwarf@latest`
* `brew install rgbds` (assembles the placeholder boot ROM stubs)
* `prebuilt/boot.6102`: the 0xFC0-byte CIC-6102 IPL3, copied from offset 0x40
  of any 6102 ROM or from the SDK's `PR/bootcode/boot.6102` (they are
  byte-identical). Not committed; it is Nintendo's code.

The RSP ucode is linked from `prebuilt/ppu` + `prebuilt/ppu.dat`, carved from
the shipped romwrapper build -- `rspasm` exists only as a Linux binary. If you
change `rsp/ppu.s` the build fails on purpose until you reassemble those with
a real rspasm and refresh the carve.

The output is a placeholder image: run it through `romwrapper/romwrapper.html`
(or MainMenu's `tools/mkgb64.js`) to splice in a Game Boy ROM.
