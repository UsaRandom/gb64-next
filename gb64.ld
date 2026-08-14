OUTPUT_ARCH (mips)

/* 0 embeds data/PokemonYellow.gb for the fast dev loop; 1 embeds the 16-byte
 * GB64_ROM magic block the romwrapper (and tools/mkgb64.js) splice a real ROM
 * over -- the shipped template is a placeholder build. Overridable because
 * the cpp step can now pass -DUSE_PLACEHOLDER=1 (tools/macbuild.sh does). */
#ifndef USE_PLACEHOLDER
#define USE_PLACEHOLDER 0
#endif

#define BEGIN_SEG(name, addr) \
    _##name##SegmentStart = ADDR(.name); \
    _##name##SegmentRomStart = __romPos; \
    .name addr : AT(__romPos)

#define END_SEG(name) \
    _##name##SegmentEnd = ADDR(.name) + SIZEOF(.name); \
    _##name##SegmentRomEnd = __romPos + SIZEOF(.name); \
    __romPos += SIZEOF(.name);

#define BEGIN_NOLOAD(name) \
    _##name##SegmentBssStart = ADDR(.name.noload); \
    .name.noload (NOLOAD) :

#define END_NOLOAD(name) \
    _##name##SegmentBssEnd = ADDR(.name.noload) + SIZEOF(.name.noload); \
    _##name##SegmentBssSize = SIZEOF(.name.noload);

#define DEFINE_LEVEL(humanName, name, theme, maxPlayers, flags) \
   BEGIN_SEG(name, 0x04000000) \
   { \
      build/data/levels/name/level.o(.data); \
      build/data/levels/name/level.o(.bss); \
   } \
   END_SEG(name) \
   BEGIN_SEG(name##_wireframe, 0x07000000) \
   { \
      build/data/levels/name/level.wire.o(.data); \
      build/data/levels/name/level.wire.o(.bss); \
   } \
   END_SEG(name##_wireframe) \

#define DEFINE_THEME(theme) \
   BEGIN_SEG(theme, 0x05000000) \
   { \
      build/data/level_themes/theme/the##me.o(.data); \
      build/data/level_themes/theme/the##me.o(.bss); \
   } \
   END_SEG(theme) \


/* Where the SDK's prebuilt RSP objects live. The default is the crashsdk
 * install path this script always assumed; a host without one (macOS) passes
 * -DSDK_PR=<path> through the cpp step instead. */
#ifndef SDK_PR
#define SDK_PR /usr/lib/n64/PR
#endif

SECTIONS
{
   __romPos = 0;

   BEGIN_SEG(boot, 0x04000000)
   {
      build/asm/rom_header.o(.text);
      build/boot.6102.o(.data);
   }
   END_SEG(boot)

   BEGIN_SEG(code, 0x80000400) SUBALIGN(16)
   {
      build/asm/entry.o(.text);
      CODE_SEGMENT(.text);
      SDK_PR/rspboot.o(.text);
      SDK_PR/gspF3DEX2.fifo.o(.text);
      bin/rsp/ppu.o(.text);

      /* data */
      CODE_SEGMENT(.data*);
      SDK_PR/rspboot.o(.data*);
      SDK_PR/gspF3DEX2.fifo.o(.data*);
      bin/rsp/ppu.o(.data*);

      /* rodata */
      CODE_SEGMENT(.rodata*);
   }
   END_SEG(code)
   BEGIN_NOLOAD(code)
   {
      CODE_SEGMENT(COMMON);
      CODE_SEGMENT(.scommon*);
      CODE_SEGMENT(.bss*);
      . = ALIGN(0x8);
   }
   END_NOLOAD(code)
   
   _codeSegmentBssEnd = .;
   
   _heapStart = .;

   . = 0x80200000;

#if USE_PLACEHOLDER
   BEGIN_SEG(sound_data, __romPos)
   {
      build/asm/data_placeholder.o(.data);
      build/asm/data_placeholder.o(.bss);
   }
   END_SEG(sound_data)
#else // USE_PLACEHOLDER
   BEGIN_SEG(sound_data, __romPos)
   {
      build/asm/data.o(.data);
      build/asm/data.o(.bss);
   }
   END_SEG(sound_data)
#endif

   /* Discard everything not specifically mentioned above. */
   /DISCARD/ :
   {
      *(.eh_frame)
      *(.MIPS.abiflags)
   }
}