/* The four DImode division helpers o32 code expects from libgcc.
 *
 * The libdragon toolchain's mips64-elf libgcc is built for its default 64-bit
 * ABI, where long long division is a native instruction and the packaged
 * helpers are the TImode set (__divti3 ...). Code compiled -mabi=32 -- ours,
 * and the crashsdk's prebuilt libultra_rom.a -- emits calls to the DImode
 * names instead, which that libgcc simply does not contain. tools/macbuild.sh
 * compiles this file and adds it to the code segment link.
 *
 * Shift-subtract, deliberately: u64 add/sub/shift/compare all inline on o32,
 * so nothing here can recurse into the very helpers it implements. 64
 * iterations of simple ALU work per divide is plenty for the callers (RTC
 * seconds-splitting and libultra's lldiv). Division by zero returns 0 rather
 * than trapping, which matches the don't-crash spirit of the callers. */

typedef unsigned long long u64;
typedef long long s64;

static u64 udivmod(u64 n, u64 d, u64 *rem)
{
    u64 q = 0, r = 0;
    int i;
    if (d != 0) {
        for (i = 63; i >= 0; --i) {
            r = (r << 1) | ((n >> i) & 1);
            if (r >= d) {
                r -= d;
                q |= (u64)1 << i;
            }
        }
    }
    if (rem) {
        *rem = r;
    }
    return q;
}

u64 __udivdi3(u64 n, u64 d) { return udivmod(n, d, 0); }

u64 __umoddi3(u64 n, u64 d)
{
    u64 r;
    udivmod(n, d, &r);
    return r;
}

s64 __divdi3(s64 n, s64 d)
{
    int neg = (n < 0) != (d < 0);
    u64 q = udivmod(n < 0 ? 0 - (u64)n : (u64)n, d < 0 ? 0 - (u64)d : (u64)d, 0);
    return neg ? -(s64)q : (s64)q;
}

s64 __moddi3(s64 n, s64 d)
{
    u64 r;
    udivmod(n < 0 ? 0 - (u64)n : (u64)n, d < 0 ? 0 - (u64)d : (u64)d, &r);
    return n < 0 ? -(s64)r : (s64)r;
}
